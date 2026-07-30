#include "SystemFileManager.h"
#include "utils/Win32Util.h"
#include "utils/RegistryUtil.h"
#include <spdlog/spdlog.h>
#include <sstream>

namespace IceClean::Core::Optimizer {

using namespace IceClean::Utils;

// ── 休眠文件 ──

SystemFileInfo SystemFileManager::GetHibernationFileInfo() {
    SystemFileInfo info;
    info.name = L"休眠文件 (hiberfil.sys)";
    info.path = Win32Util::GetSystemDrive() + L"\\hiberfil.sys";
    info.canManage = true;
    info.isActive = IsHibernationEnabled();
    info.sizeBytes = GetHibernationFileSize();
    info.sizeDisplay = FormatSize(info.sizeBytes);
    info.description = L"休眠功能保留系统状态到硬盘的文件。不使用休眠可安全禁用";
    info.actionLabel = info.isActive ? L"禁用休眠并删除" : L"休眠已禁用";
    return info;
}

bool SystemFileManager::IsHibernationEnabled() const {
    std::wstring output = RunCommandWithOutput(L"powercfg /a");
    return output.find(L"休眠") != std::wstring::npos ||
           output.find(L"hibernate") != std::wstring::npos;
}

uint64_t SystemFileManager::GetHibernationFileSize() const {
    std::wstring path = Win32Util::GetSystemDrive() + L"\\hiberfil.sys";
    WIN32_FILE_ATTRIBUTE_DATA attr = {};
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attr)) {
        ULARGE_INTEGER size;
        size.LowPart = attr.nFileSizeLow;
        size.HighPart = attr.nFileSizeHigh;
        return size.QuadPart;
    }
    return 0;
}

SystemFileActionResult SystemFileManager::DisableHibernation() {
    SystemFileActionResult result;
    result.success = RunAdminCommand(L"powercfg /h off");
    if (result.success) {
        result.freedBytes = GetHibernationFileSize();
        result.message = L"休眠已禁用，hiberfil.sys 已删除，释放 " + FormatSize(result.freedBytes);
        spdlog::info("系统休眠已禁用");
    } else {
        result.message = L"禁用休眠失败，可能需要管理员权限";
        spdlog::warn("禁用休眠失败");
    }
    return result;
}

SystemFileActionResult SystemFileManager::SetHibernationSize(DWORD percent) {
    SystemFileActionResult result;
    if (percent > 100) percent = 100;
    if (percent < 40) percent = 40; // 最小 40%

    uint64_t oldSize = GetHibernationFileSize();
    std::wstring cmd = L"powercfg /h /size " + std::to_wstring(percent);
    result.success = RunAdminCommand(cmd);

    if (result.success) {
        uint64_t newSize = GetHibernationFileSize();
        if (oldSize > newSize) result.freedBytes = oldSize - newSize;
        result.message = L"休眠文件已调整为 " + std::to_wstring(percent) + L"% (" + FormatSize(newSize) + L")";
    } else {
        result.message = L"调整休眠文件大小失败";
    }
    return result;
}

// ── 虚拟内存 ──

SystemFileInfo SystemFileManager::GetPageFileInfo() {
    SystemFileInfo info;
    info.name = L"虚拟内存 (pagefile.sys)";
    info.path = Win32Util::GetSystemDrive() + L"\\pagefile.sys";
    info.canManage = false; // 不提供直接禁用，仅提供优化建议

    uint64_t totalSize = 0, recommendedSize = 0;
    if (GetPageFileInfo(totalSize, recommendedSize)) {
        info.sizeBytes = totalSize;
        info.sizeDisplay = FormatSize(totalSize);
    }
    info.description = L"虚拟内存文件。系统需要虚拟内存运行，可优化但不可禁用";
    info.actionLabel = L"优化虚拟内存";
    info.isActive = true;
    return info;
}

bool SystemFileManager::GetPageFileInfo(uint64_t& totalSize, uint64_t& recommendedSize) const {
    // 通过注册表获取页面文件大小
    std::wstring regPath = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management";
    DWORD initialSize = RegistryUtil::ReadDwordValue(HKEY_LOCAL_MACHINE, regPath, L"PagingFiles");
    // 简化：通过内存大小估算推荐值
    MEMORYSTATUSEX memStatus = { sizeof(memStatus) };
    if (GlobalMemoryStatusEx(&memStatus)) {
        recommendedSize = memStatus.ullTotalPhys;
        totalSize = memStatus.ullTotalPageFile;
        return true;
    }
    return false;
}

SystemFileActionResult SystemFileManager::OptimizePageFile() {
    SystemFileActionResult result;
    // 建议用户通过系统调整（不自动修改虚拟内存大小，避免风险）
    result.success = true;
    result.message = L"虚拟内存推荐由系统托管管理：设置→系统→关于→高级系统设置→性能→高级→虚拟内存→自动管理";
    return result;
}

// ── Windows.old ──

SystemFileInfo SystemFileManager::GetWindowsOldInfo() {
    SystemFileInfo info;
    info.name = L"Windows.old";
    info.path = Win32Util::GetSystemDrive() + L"\\Windows.old";
    info.canManage = true;
    info.isActive = HasWindowsOld();
    info.sizeBytes = GetWindowsOldSize();
    info.sizeDisplay = FormatSize(info.sizeBytes);
    info.description = L"Windows 系统更新时保留的旧系统文件。确认新系统运行正常后可删除";
    info.actionLabel = info.isActive ? L"删除 Windows.old" : L"不存在";
    return info;
}

bool SystemFileManager::HasWindowsOld() const {
    std::wstring path = Win32Util::GetSystemDrive() + L"\\Windows.old";
    return FileUtil::Exists(path);
}

uint64_t SystemFileManager::GetWindowsOldSize() const {
    std::wstring path = Win32Util::GetSystemDrive() + L"\\Windows.old";
    if (!FileUtil::Exists(path)) return 0;
    return FileUtil::GetFolderSize(path);
}

SystemFileActionResult SystemFileManager::CleanWindowsOld() {
    SystemFileActionResult result;
    if (!HasWindowsOld()) {
        result.message = L"Windows.old 不存在";
        return result;
    }

    uint64_t oldSize = GetWindowsOldSize();
    // 使用 DISM 清理
    result.success = RunAdminCommand(L"dism /online /Remove-InitialTask /Cleanup-Image /StartComponentCleanup /ResetBase");

    if (!result.success) {
        // 备用方案：直接使用磁盘清理工具
        result.success = RunAdminCommand(L"cleanmgr /d " + Win32Util::GetSystemDrive()[0] + L" /sagerun:1");
    }

    if (result.success) {
        result.freedBytes = oldSize;
        result.message = L"Windows.old 已删除，释放 " + FormatSize(oldSize);
        spdlog::info("Windows.old 清理完成");
    } else {
        result.message = L"清理 Windows.old 失败";
    }
    return result;
}

// ── 回收站 ──

SystemFileInfo SystemFileManager::GetRecycleBinInfo() {
    SystemFileInfo info;
    info.name = L"回收站";
    info.path = Win32Util::GetSystemDrive() + L"\\$Recycle.Bin";
    info.canManage = true;
    info.isActive = GetRecycleBinSize() > 0;
    info.sizeBytes = GetRecycleBinSize();
    info.sizeDisplay = FormatSize(info.sizeBytes);
    info.description = L"已删除文件的临时存放处。清空回收站可立即释放磁盘空间";
    info.actionLabel = info.isActive ? L"清空回收站" : L"回收站为空";
    return info;
}

uint64_t SystemFileManager::GetRecycleBinSize() const {
    std::wstring recyclePath = Win32Util::GetSystemDrive() + L"\\$Recycle.Bin";
    if (!FileUtil::Exists(recyclePath)) return 0;

    uint64_t totalSize = 0;
    WIN32_FIND_DATAW findData;
    std::wstring searchPath = recyclePath + L"\\*";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;
            std::wstring fullPath = recyclePath + L"\\" + findData.cFileName;
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                totalSize += FileUtil::GetFolderSize(fullPath);
            } else {
                ULARGE_INTEGER size;
                size.LowPart = findData.nFileSizeLow;
                size.HighPart = findData.nFileSizeHigh;
                totalSize += size.QuadPart;
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }
    return totalSize;
}

SystemFileActionResult SystemFileManager::EmptyRecycleBin() {
    SystemFileActionResult result;
    uint64_t oldSize = GetRecycleBinSize();

    // 使用 SHEmptyRecycleBin API
    HRESULT hr = SHEmptyRecycleBinW(nullptr, nullptr, SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI);
    result.success = SUCCEEDED(hr);

    if (result.success) {
        result.freedBytes = oldSize;
        result.message = L"回收站已清空，释放 " + FormatSize(oldSize);
        spdlog::info("回收站已清空，释放 {}", oldSize);
    } else {
        result.message = L"清空回收站失败";
        spdlog::warn("清空回收站失败: HRESULT={}", hr);
    }
    return result;
}

// ── WinSxS 清理 ──

SystemFileActionResult SystemFileManager::CleanWinSxS() {
    SystemFileActionResult result;
    result.success = RunAdminCommand(L"dism /online /Cleanup-Image /StartComponentCleanup /ResetBase");
    if (result.success) {
        result.message = L"WinSxS 组件清理完成";
    } else {
        result.message = L"WinSxS 清理失败";
    }
    return result;
}

// ── 工具方法 ──

std::wstring SystemFileManager::FormatSize(uint64_t bytes) {
    if (bytes >= 1024ULL * 1024 * 1024) {
        double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%.2f GB", gb);
        return buf;
    } else if (bytes >= 1024 * 1024) {
        double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%.2f MB", mb);
        return buf;
    } else if (bytes >= 1024) {
        double kb = static_cast<double>(bytes) / 1024.0;
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%.1f KB", kb);
        return buf;
    }
    return std::to_wstring(bytes) + L" B";
}

bool SystemFileManager::RunAdminCommand(const std::wstring& cmd, DWORD timeoutMs) const {
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                         FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, timeoutMs);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode == 0;
}

std::wstring SystemFileManager::RunCommandWithOutput(const std::wstring& cmd) const {
    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return L"";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                         TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return L"";
    }

    CloseHandle(hWritePipe);

    char buffer[4096] = {};
    DWORD bytesRead = 0;
    std::string output;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }

    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return std::wstring(output.begin(), output.end());
}

} // namespace IceClean::Core::Optimizer
