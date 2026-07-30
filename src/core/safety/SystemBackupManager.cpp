#include "SystemBackupManager.h"
#include "utils/FormatUtil.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

#include <filesystem>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <map>

namespace IceClean::Core::Safety {

std::wstring SystemBackupManager::GetBackupDirectory() {
    wchar_t appData[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_COMMON_APPDATA, nullptr, 0, appData))) {
        std::wstring dir = std::wstring(appData) + L"\\IceClean\\Backups";
        std::filesystem::create_directories(dir);
        return dir;
    }
    return L"C:\\ProgramData\\IceClean\\Backups";
}

bool SystemBackupManager::BackupRegistry(const std::wstring& backupName,
                                            const std::vector<std::wstring>& registryPaths) {
    auto backupDir = GetBackupDirectory();
    auto backupId = GenerateBackupId();
    auto backupPath = backupDir + L"\\" + backupId;

    std::filesystem::create_directories(backupPath);

    bool allSuccess = true;
    int fileIndex = 0;

    for (const auto& regPath : registryPaths) {
        std::wstring filePath = backupPath + L"\\reg_" + std::to_wstring(fileIndex++) + L".reg";

        HKEY hRootKey = nullptr;
        std::wstring subKey;

        if (regPath.find(L"HKEY_LOCAL_MACHINE\\") == 0) {
            hRootKey = HKEY_LOCAL_MACHINE;
            subKey = regPath.substr(19);
        } else if (regPath.find(L"HKEY_CURRENT_USER\\") == 0) {
            hRootKey = HKEY_CURRENT_USER;
            subKey = regPath.substr(18);
        } else if (regPath.find(L"HKEY_CLASSES_ROOT\\") == 0) {
            hRootKey = HKEY_CLASSES_ROOT;
            subKey = regPath.substr(18);
        } else if (regPath.find(L"HKEY_USERS\\") == 0) {
            hRootKey = HKEY_USERS;
            subKey = regPath.substr(11);
        } else {
            allSuccess = false;
            continue;
        }

        if (!ExportRegistryKey(hRootKey, subKey, filePath)) {
            allSuccess = false;
        }
    }

    if (allSuccess || !registryPaths.empty()) {
        Models::BackupRecord record;
        record.backupId = backupId;
        record.type = Models::BackupType::RegistryBackup;
        record.description = backupName;
        record.backupPath = backupPath;
        record.createTime = std::chrono::system_clock::now();
        record.canRestore = true;

        uint64_t size = 0;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(backupPath)) {
            if (entry.is_regular_file()) {
                size += entry.file_size();
            }
        }
        record.backupSize = size;

        SaveBackupMetadata(record);
    }

    return allSuccess;
}

bool SystemBackupManager::BackupFullRegistry(const std::wstring& backupName) {
    std::vector<std::wstring> criticalPaths = {
        L"HKEY_LOCAL_MACHINE\\SOFTWARE",
        L"HKEY_LOCAL_MACHINE\\SYSTEM",
        L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
        L"HKEY_CURRENT_USER\\SOFTWARE",
        L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
    };

    return BackupRegistry(backupName.empty() ? L"完整注册表备份" : backupName, criticalPaths);
}

bool SystemBackupManager::BackupStartupItems(const std::wstring& backupName) {
    std::vector<std::wstring> startupPaths = {
        L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
        L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
        L"HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
        L"HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Run",
    };

    return BackupRegistry(backupName.empty() ? L"启动项备份" : backupName, startupPaths);
}

bool SystemBackupManager::RestoreRegistry(const Models::BackupRecord& record) {
    if (!record.canRestore) return false;

    namespace fs = std::filesystem;

    try {
        for (const auto& entry : fs::directory_iterator(record.backupPath)) {
            if (entry.is_regular_file() && entry.path().extension() == L".reg") {
                ImportRegistryFile(entry.path().wstring());
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<Models::BackupRecord> SystemBackupManager::GetBackupList() {
    std::vector<Models::BackupRecord> records;
    auto backupDir = GetBackupDirectory();

    namespace fs = std::filesystem;
    if (!fs::exists(backupDir)) return records;

    for (const auto& entry : fs::directory_iterator(backupDir)) {
        if (entry.is_directory()) {
            auto record = ReadBackupMetadata(entry.path().wstring());
            if (!record.backupId.empty()) {
                records.push_back(record);
            }
        }
    }

    std::sort(records.begin(), records.end(), [](const auto& a, const auto& b) {
        return a.createTime > b.createTime;
    });

    return records;
}

bool SystemBackupManager::DeleteBackup(const std::wstring& backupId) {
    auto backupDir = GetBackupDirectory();
    auto targetDir = backupDir + L"\\" + backupId;

    try {
        std::filesystem::remove_all(targetDir);
        return true;
    } catch (...) {
        return false;
    }
}

int SystemBackupManager::CleanupOldBackups(int keepCount) {
    auto records = GetBackupList();

    std::map<Models::BackupType, std::vector<Models::BackupRecord>> grouped;
    for (const auto& record : records) {
        grouped[record.type].push_back(record);
    }

    int deleted = 0;
    for (auto& [type, typeRecords] : grouped) {
        for (int i = keepCount; i < static_cast<int>(typeRecords.size()); ++i) {
            if (DeleteBackup(typeRecords[i].backupId)) {
                deleted++;
            }
        }
    }

    return deleted;
}

uint64_t SystemBackupManager::GetTotalBackupSize() {
    uint64_t total = 0;
    auto backupDir = GetBackupDirectory();

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(backupDir)) {
            if (entry.is_regular_file()) {
                total += entry.file_size();
            }
        }
    } catch (...) {}

    return total;
}

bool SystemBackupManager::ExportRegistryKey(HKEY hKey, const std::wstring& subKey,
                                              const std::wstring& filePath) {
    std::wstring rootName;
    if (hKey == HKEY_LOCAL_MACHINE) rootName = L"HKLM";
    else if (hKey == HKEY_CURRENT_USER) rootName = L"HKCU";
    else if (hKey == HKEY_CLASSES_ROOT) rootName = L"HKCR";
    else if (hKey == HKEY_USERS) rootName = L"HKU";
    else return false;

    std::wstring fullKey = rootName + L"\\" + subKey;
    std::wstring cmd = L"reg export \"" + fullKey + L"\" \"" + filePath + L"\" /y";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                         FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 30000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

bool SystemBackupManager::ImportRegistryFile(const std::wstring& filePath) {
    std::wstring cmd = L"reg import \"" + filePath + L"\"";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                         FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 30000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

std::wstring SystemBackupManager::GenerateBackupId() {
    auto now = std::chrono::system_clock::now();
    auto time_t_val = std::chrono::system_clock::to_time_t(now);
    struct tm tm_val;
    localtime_s(&tm_val, &time_t_val);

    wchar_t buf[32] = {};
    wcsftime(buf, 32, L"%Y%m%d_%H%M%S", &tm_val);

    int randVal = rand() % 10000;
    return std::wstring(buf) + L"_" + std::to_wstring(randVal);
}

bool SystemBackupManager::SaveBackupMetadata(const Models::BackupRecord& record) {
    auto metaPath = record.backupPath + L"\\metadata.txt";

    auto time_t_val = std::chrono::system_clock::to_time_t(record.createTime);
    struct tm tm_val;
    localtime_s(&tm_val, &time_t_val);
    wchar_t timeBuf[32] = {};
    wcsftime(timeBuf, 32, L"%Y-%m-%d %H:%M:%S", &tm_val);

    std::wofstream out(metaPath);
    if (!out.is_open()) return false;

    out << L"BackupId=" << record.backupId << L"\n";
    out << L"Type=" << static_cast<int>(record.type) << L"\n";
    out << L"Description=" << record.description << L"\n";
    out << L"CreateTime=" << timeBuf << L"\n";
    out << L"Size=" << record.backupSize << L"\n";
    out << L"CanRestore=" << (record.canRestore ? L"1" : L"0") << L"\n";
    out.close();
    return true;
}

Models::BackupRecord SystemBackupManager::ReadBackupMetadata(const std::wstring& backupDir) {
    Models::BackupRecord record;
    auto metaPath = backupDir + L"\\metadata.txt";

    std::wifstream in(metaPath);
    if (!in.is_open()) {
        // 如果没有元数据文件，从目录名推断
        record.backupId = std::filesystem::path(backupDir).filename().wstring();
        record.backupPath = backupDir;
        record.type = Models::BackupType::RegistryBackup;
        record.canRestore = true;
        return record;
    }

    std::wstring line;
    while (std::getline(in, line)) {
        auto pos = line.find(L'=');
        if (pos == std::wstring::npos) continue;

        auto key = line.substr(0, pos);
        auto value = line.substr(pos + 1);

        if (key == L"BackupId") record.backupId = value;
        else if (key == L"Type") record.type = static_cast<Models::BackupType>(std::stoi(value));
        else if (key == L"Description") record.description = value;
        else if (key == L"Size") record.backupSize = std::stoull(value);
        else if (key == L"CanRestore") record.canRestore = (value == L"1");
    }

    record.backupPath = backupDir;
    return record;
}

std::wstring SystemBackupManager::GetBackupTypeName(Models::BackupType type) {
    switch (type) {
        case Models::BackupType::RegistryBackup: return L"注册表备份";
        case Models::BackupType::DriverBackup: return L"驱动备份";
        case Models::BackupType::SystemStateBackup: return L"系统状态备份";
        case Models::BackupType::StartupBackup: return L"启动项备份";
        default: return L"未知";
    }
}

} // namespace IceClean::Core::Safety
