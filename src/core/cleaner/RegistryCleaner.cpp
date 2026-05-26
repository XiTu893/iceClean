#include "RegistryCleaner.h"
#include "utils/RegistryUtil.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Cleaner {

using namespace IceClean::Models;
using namespace IceClean::Utils;

RegistryCleaner::RegistryCleaner() = default;

// ============================================================================
// 基类 ICleaner 接口实现
// ============================================================================

Models::CleanResult RegistryCleaner::Clean(const std::vector<std::wstring>& paths,
                                            std::function<void(const Models::CleanProgress&)> progressCallback,
                                            const std::atomic<bool>* cancelFlag) {
    // 基类接口：paths 作为注册表键路径列表，逐个删除
    Models::CleanResult result;
    result.success = true;

    int totalItems = static_cast<int>(paths.size());
    int currentItem = 0;

    for (const auto& keyPath : paths) {
        if (progressCallback) {
            Models::CleanProgress progress;
            progress.currentItem = currentItem;
            progress.totalItems = totalItems;
            progress.currentFile = keyPath;
            progress.isRunning = true;
            progressCallback(progress);
        }

        // 尝试删除注册表值或键
        // 路径格式: HKLM\SubKey\ValueName 或 HKLM\SubKey（删除整个键）
        bool deleted = false;
        size_t lastBackslash = keyPath.rfind(L'\\');
        if (lastBackslash != std::wstring::npos) {
            // 尝试解析根键
            HKEY rootKey = HKEY_LOCAL_MACHINE;
            std::wstring remaining = keyPath;

            if (remaining.find(L"HKLM\\") == 0) {
                rootKey = HKEY_LOCAL_MACHINE;
                remaining = remaining.substr(5);
            } else if (remaining.find(L"HKCU\\") == 0) {
                rootKey = HKEY_CURRENT_USER;
                remaining = remaining.substr(5);
            } else {
                currentItem++;
                result.failedFileCount++;
                result.failedFiles.push_back(keyPath);
                continue;
            }

            // 分离子键和值名
            size_t valueSep = remaining.rfind(L'\\');
            if (valueSep != std::wstring::npos) {
                std::wstring subKey = remaining.substr(0, valueSep);
                std::wstring valueName = remaining.substr(valueSep + 1);

                // 先尝试删除值
                if (RegistryUtil::DeleteValue(rootKey, subKey, valueName)) {
                    deleted = true;
                }
            }

            // 如果删除值失败，尝试删除键（路径整体作为子键）
            if (!deleted) {
                // remaining 整体作为子键路径
                // 使用 RegDeleteKey 删除
                HKEY hKey;
                if (RegOpenKeyExW(rootKey, remaining.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                    RegCloseKey(hKey);
                    if (RegDeleteKeyW(rootKey, remaining.c_str()) == ERROR_SUCCESS) {
                        deleted = true;
                    }
                }
            }
        }

        if (deleted) {
            result.cleanedFileCount++;
        } else {
            result.failedFileCount++;
            result.failedFiles.push_back(keyPath);
        }

        currentItem++;
    }

    if (progressCallback) {
        Models::CleanProgress progress;
        progress.currentItem = totalItems;
        progress.totalItems = totalItems;
        progress.isRunning = false;
        progressCallback(progress);
    }

    return result;
}

// ============================================================================
// 扫描无效注册表项
// ============================================================================

std::vector<RegistryInvalidItem> RegistryCleaner::ScanInvalidItems() {
    std::vector<RegistryInvalidItem> items;

    ScanInvalidUninstall(items);
    ScanInvalidStartup(items);

    return items;
}

// ============================================================================
// 扫描无效的卸载信息
// ============================================================================

void RegistryCleaner::ScanInvalidUninstall(std::vector<RegistryInvalidItem>& items) {
    // 需要扫描的两个卸载注册表路径
    const std::vector<std::pair<HKEY, std::wstring>> uninstallPaths = {
        {HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"},
        {HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"}
    };

    for (const auto& [rootKey, subKey] : uninstallPaths) {
        auto subKeys = RegistryUtil::EnumSubKeys(rootKey, subKey);

        for (const auto& childKey : subKeys) {
            std::wstring fullKeyPath = subKey + L"\\" + childKey;

            // 读取 DisplayName
            std::wstring displayName = RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"DisplayName");
            if (displayName.empty()) {
                continue; // 没有显示名称的项跳过
            }

            // 读取 UninstallString 和 InstallLocation
            std::wstring uninstallString = RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"UninstallString");
            std::wstring installLocation = RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"InstallLocation");

            bool isInvalid = false;
            std::wstring invalidPath;

            // 检查 InstallLocation 路径是否存在
            if (!installLocation.empty()) {
                std::wstring expandedPath = Win32Util::ExpandEnvVars(installLocation);
                if (!PathExists(expandedPath)) {
                    isInvalid = true;
                    invalidPath = installLocation;
                }
            }

            // 检查 UninstallString 指向的程序是否存在
            if (!isInvalid && !uninstallString.empty()) {
                std::wstring filePath = ExtractFilePath(Win32Util::ExpandEnvVars(uninstallString));
                if (!filePath.empty() && !PathExists(filePath)) {
                    isInvalid = true;
                    invalidPath = uninstallString;
                }
            }

            if (isInvalid) {
                RegistryInvalidItem item;
                item.keyPath = fullKeyPath;
                item.valueName = L""; // 空表示整个键无效
                item.invalidValue = invalidPath;
                item.description = L"无效的卸载信息: " + displayName;
                item.type = RegistryInvalidItem::Type::InvalidUninstall;

                // 构建完整的注册表路径用于显示
                if (rootKey == HKEY_LOCAL_MACHINE) {
                    item.keyPath = L"HKLM\\" + fullKeyPath;
                }

                items.push_back(item);
            }
        }
    }
}

// ============================================================================
// 扫描无效的启动项
// ============================================================================

void RegistryCleaner::ScanInvalidStartup(std::vector<RegistryInvalidItem>& items) {
    const std::vector<std::pair<HKEY, std::wstring>> runPaths = {
        {HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"},
        {HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"}
    };

    for (const auto& [rootKey, subKey] : runPaths) {
        auto valueNames = RegistryUtil::EnumValues(rootKey, subKey);

        for (const auto& valueName : valueNames) {
            std::wstring valueData = RegistryUtil::ReadStringValue(rootKey, subKey, valueName);
            if (valueData.empty()) {
                continue;
            }

            // 从命令行中提取文件路径
            std::wstring filePath = ExtractFilePath(Win32Util::ExpandEnvVars(valueData));
            if (filePath.empty()) {
                continue;
            }

            if (!PathExists(filePath)) {
                RegistryInvalidItem item;
                if (rootKey == HKEY_CURRENT_USER) {
                    item.keyPath = L"HKCU\\" + subKey;
                } else {
                    item.keyPath = L"HKLM\\" + subKey;
                }
                item.valueName = valueName;
                item.invalidValue = valueData;
                item.description = L"无效的启动项: " + valueName;
                item.type = RegistryInvalidItem::Type::InvalidStartup;

                items.push_back(item);
            }
        }
    }
}

// ============================================================================
// 检查路径是否存在（展开环境变量）
// ============================================================================

bool RegistryCleaner::PathExists(const std::wstring& path) const {
    if (path.empty()) {
        return false;
    }

    std::wstring expandedPath = Win32Util::ExpandEnvVars(path);
    return FileUtil::Exists(expandedPath);
}

// ============================================================================
// 从命令行字符串中提取文件路径
// 处理格式:
//   "C:\Program Files\App\app.exe" --arg
//   C:\Program Files\App\app.exe --arg
//   C:\Programs\App.exe --arg
// ============================================================================

std::wstring RegistryCleaner::ExtractFilePath(const std::wstring& value) const {
    if (value.empty()) {
        return L"";
    }

    std::wstring trimmed = value;

    // 去掉前导空白
    size_t start = trimmed.find_first_not_of(L" \t");
    if (start == std::wstring::npos) {
        return L"";
    }
    trimmed = trimmed.substr(start);

    // 情况1: 路径用引号包裹
    if (trimmed[0] == L'"') {
        size_t endQuote = trimmed.find(L'"', 1);
        if (endQuote != std::wstring::npos) {
            return trimmed.substr(1, endQuote - 1);
        }
        // 引号未闭合，返回引号后的全部内容
        return trimmed.substr(1);
    }

    // 情况2: 路径用单引号包裹
    if (trimmed[0] == L'\'') {
        size_t endQuote = trimmed.find(L'\'', 1);
        if (endQuote != std::wstring::npos) {
            return trimmed.substr(1, endQuote - 1);
        }
        return trimmed.substr(1);
    }

    // 情况3: 无引号的路径
    // 查找第一个参数分隔符（空格或制表符）之前的部分
    // 但需要处理路径中包含空格的情况，尝试逐步扩展路径检查是否存在
    size_t spacePos = trimmed.find_first_of(L" \t");
    std::wstring candidate = (spacePos != std::wstring::npos) ? trimmed.substr(0, spacePos) : trimmed;

    // 如果候选路径存在，直接返回
    if (FileUtil::Exists(Win32Util::ExpandEnvVars(candidate))) {
        return candidate;
    }

    // 路径可能包含空格，尝试逐步扩展
    size_t searchStart = 0;
    while (searchStart < trimmed.size()) {
        size_t nextSpace = trimmed.find_first_of(L" \t", searchStart);
        if (nextSpace == std::wstring::npos) {
            // 已到末尾
            candidate = trimmed;
        } else {
            candidate = trimmed.substr(0, nextSpace);
        }

        std::wstring expandedCandidate = Win32Util::ExpandEnvVars(candidate);
        if (FileUtil::Exists(expandedCandidate)) {
            return candidate;
        }

        // 尝试添加常见可执行文件扩展名
        const std::vector<std::wstring> extensions = {L".exe", L".com", L".bat", L".cmd", L".msi"};
        for (const auto& ext : extensions) {
            if (FileUtil::Exists(expandedCandidate + ext)) {
                return candidate + ext;
            }
        }

        if (nextSpace == std::wstring::npos) {
            break;
        }
        searchStart = nextSpace + 1;
    }

    // 无法确定有效路径，返回第一个空格前的部分
    if (spacePos != std::wstring::npos) {
        return trimmed.substr(0, spacePos);
    }

    return trimmed;
}

// ============================================================================
// 清理指定的无效注册表项
// ============================================================================

Models::CleanResult RegistryCleaner::Clean(const std::vector<RegistryInvalidItem>& items,
                                            const std::wstring& backupPath,
                                            std::function<void(const Models::CleanProgress&)> progressCb) {
    Models::CleanResult result;
    result.success = true;

    int totalItems = static_cast<int>(items.size());
    int currentItem = 0;

    for (const auto& item : items) {
        // 发送进度回调
        if (progressCb) {
            Models::CleanProgress progress;
            progress.currentItem = currentItem;
            progress.totalItems = totalItems;
            progress.currentFile = item.keyPath + (item.valueName.empty() ? L"" : L"\\" + item.valueName);
            progress.isRunning = true;
            progressCb(progress);
        }

        // 备份注册表项
        if (!backupPath.empty()) {
            std::wstring keyToBackup = item.valueName.empty() ? item.keyPath : item.keyPath;
            BackupRegistryKey(keyToBackup, backupPath);
        }

        // 解析根键和子键路径
        HKEY rootKey = HKEY_LOCAL_MACHINE;
        std::wstring subKeyPath;

        if (item.keyPath.find(L"HKLM\\") == 0) {
            rootKey = HKEY_LOCAL_MACHINE;
            subKeyPath = item.keyPath.substr(5);
        } else if (item.keyPath.find(L"HKCU\\") == 0) {
            rootKey = HKEY_CURRENT_USER;
            subKeyPath = item.keyPath.substr(5);
        } else {
            result.failedFileCount++;
            result.failedFiles.push_back(item.keyPath);
            currentItem++;
            continue;
        }

        bool deleted = false;

        if (item.valueName.empty()) {
            // 删除整个键
            HKEY hKey;
            if (RegOpenKeyExW(rootKey, subKeyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                if (RegDeleteKeyW(rootKey, subKeyPath.c_str()) == ERROR_SUCCESS) {
                    deleted = true;
                }
            }
        } else {
            // 删除指定的值
            if (RegistryUtil::DeleteValue(rootKey, subKeyPath, item.valueName)) {
                deleted = true;
            }
        }

        if (deleted) {
            result.cleanedFileCount++;
        } else {
            result.failedFileCount++;
            result.failedFiles.push_back(item.keyPath + (item.valueName.empty() ? L"" : L"\\" + item.valueName));
        }

        currentItem++;
    }

    // 发送完成回调
    if (progressCb) {
        Models::CleanProgress progress;
        progress.currentItem = totalItems;
        progress.totalItems = totalItems;
        progress.isRunning = false;
        progressCb(progress);
    }

    return result;
}

// ============================================================================
// 备份注册表项到 .reg 文件
// 使用 reg export 命令导出
// ============================================================================

bool RegistryCleaner::BackupRegistryKey(const std::wstring& keyPath, const std::wstring& backupFile) {
    // 构建 reg export 命令
    // 格式: reg export "HKLM\SOFTWARE\..." "backup.reg" /y
    std::wstring command = L"reg export \"" + keyPath + L"\" \"" + backupFile + L"\" /y";

    // 执行命令
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};

    // 使用 CreateProcess 执行命令
    std::wstring cmdLine = L"cmd.exe /c " + command;
    BOOL success = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) {
        return false;
    }

    // 等待命令完成
    WaitForSingleObject(pi.hProcess, 30000); // 30秒超时

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

} // namespace IceClean::Core::Cleaner
