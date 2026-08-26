#include "DriverBackupManager.h"
#include <windows.h>
#include <shlobj.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <algorithm>
#include "core/driver/PnpUtilRunner.h"
#include "core/safety/RestorePointManager.h"
#include "core/safety/OperationLogger.h"
#include "utils/JsonUtil.h"
#include "utils/FormatUtil.h"

namespace IceClean::Core::Driver {

namespace {

std::wstring GetAppDataRoaming() {
    wchar_t path[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        return path;
    }
    wchar_t local[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, local))) {
        return local;
    }
    return L".";
}

std::wstring CurrentTimestamp() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buf[32] = {};
    swprintf_s(buf, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay,
               st.wHour, st.wMinute, st.wSecond);
    return buf;
}

std::wstring GetSystemVersion() {
    // 读取当前 Windows 产品名与版本（仅做备份元信息，不依赖此执行逻辑）
    HKEY hKey = nullptr;
    std::wstring ver;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0,
                      KEY_READ, &hKey) == ERROR_SUCCESS) {
        auto read = [&](const wchar_t* name) -> std::wstring {
            wchar_t buf[256] = {};
            DWORD size = sizeof(buf);
            if (RegQueryValueExW(hKey, name, nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(buf), &size) == ERROR_SUCCESS) {
                return buf;
            }
            return {};
        };
        ver = read(L"ProductName");
        const std::wstring build = read(L"CurrentBuild");
        if (!build.empty()) ver += L" (Build " + build + L")";
        RegCloseKey(hKey);
    }
    return ver.empty() ? L"Unknown" : ver;
}

} // namespace

std::wstring DriverBackupManager::GetDefaultBackupRoot() {
    return GetAppDataRoaming() + L"\\IceClean\\Backups\\Drivers";
}

DriverBackupManager::BackupResult DriverBackupManager::BackupAll(
    const std::wstring& backupRoot,
    std::function<void(int, int, const std::wstring&)> progress) {
    BackupResult result;
    namespace fs = std::filesystem;

    try {
        fs::create_directories(backupRoot);
    } catch (...) {
        return result;
    }

    const std::wstring stamp = CurrentTimestamp();
    const std::wstring backupDir = backupRoot + L"\\" + stamp;
    try {
        fs::create_directories(backupDir);
    } catch (...) {
        return result;
    }

    const bool ok = PnpUtilRunner::ExportAll(backupDir, progress);
    result.backupDir = backupDir;

    // 统计导出数量（.inf 文件数）
    int count = 0;
    std::error_code ec;
    for (auto it = fs::recursive_directory_iterator(backupDir, ec);
         it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) break;
        if (it->path().extension().wstring() == L".inf") count++;
    }
    result.packageCount = count;

    // 写入 manifest.json
    nlohmann::json manifest;
    manifest["time"] = IceClean::Utils::JsonUtil::WideToUtf8(stamp);
    manifest["systemVersion"] = IceClean::Utils::JsonUtil::WideToUtf8(GetSystemVersion());
    manifest["packageCount"] = count;
    IceClean::Utils::JsonUtil::SaveJson(backupDir + L"\\manifest.json", manifest);

    result.success = ok && count > 0;
    return result;
}

bool DriverBackupManager::RestoreFrom(const std::wstring& backupDir) {
    // ① 还原前强制创建系统还原点
    if (!IceClean::Core::Safety::RestorePointManager::CreateRestorePoint(
            L"IceClean 驱动还原前自动还原点")) {
        // 还原点创建失败即中止，绝不执行导入
        return false;
    }

    // ② 导入并安装
    const int code = PnpUtilRunner::AddDrivers(backupDir);

    // ③ 结果落 OperationLogger
    Models::OperationRecord rec;
    rec.type = Models::OperationType::Restore;
    rec.description = L"从备份还原驱动: " + backupDir;
    rec.success = (code == 0);
    rec.details = std::wstring(L"{\"backupDir\":\"") + backupDir +
                  L"\",\"exitCode\":" + std::to_wstring(code) + L"}";
    IceClean::Core::Safety::OperationLogger::LogOperation(rec);

    return code == 0;
}

std::vector<DriverBackupManager::BackupEntry> DriverBackupManager::ListBackups(
    const std::wstring& backupRoot) {
    std::vector<BackupEntry> entries;
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(backupRoot, ec)) return entries;

    for (const auto& entry : fs::directory_iterator(backupRoot, ec)) {
        if (!entry.is_directory()) continue;
        const auto manifestPath = entry.path() / "manifest.json";
        if (!fs::exists(manifestPath, ec)) continue;

        BackupEntry be;
        be.dir = entry.path().wstring();
        be.time = entry.path().filename().wstring();

        const auto manifest = IceClean::Utils::JsonUtil::LoadJson(manifestPath.wstring());
        if (manifest.contains("packageCount")) {
            be.packageCount = manifest["packageCount"].get<int>();
        }
        if (manifest.contains("systemVersion")) {
            be.systemVersion =
                IceClean::Utils::JsonUtil::Utf8ToWide(manifest["systemVersion"].get<std::string>());
        }
        entries.push_back(std::move(be));
    }

    std::sort(entries.begin(), entries.end(),
              [](const BackupEntry& a, const BackupEntry& b) { return a.time > b.time; });
    return entries;
}

namespace {
uint64_t GetDriverStoreSize() {
    uint64_t total = 0;
    namespace fs = std::filesystem;
    std::error_code ec;
    const std::wstring store = L"C:\\Windows\\System32\\DriverStore\\FileRepository";
    if (!fs::exists(store, ec)) return 0;
    for (auto it = fs::recursive_directory_iterator(store, ec);
         it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) break;
        if (it->is_regular_file()) {
            std::error_code fec;
            total += it->file_size(fec);
        }
    }
    return total;
}
} // namespace

uint64_t DriverBackupManager::CleanupOldBackups() {
    const uint64_t before = GetDriverStoreSize();

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};
    const std::wstring cmd = L"Dism.exe /Online /Cleanup-Image /StartComponentCleanup";
    if (CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                       FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 300000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    const uint64_t after = GetDriverStoreSize();
    return (after < before) ? (before - after) : 0;
}

} // namespace IceClean::Core::Driver
