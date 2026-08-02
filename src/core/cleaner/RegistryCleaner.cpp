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
                if (RegOpenKeyExW(rootKey, remaining.c_str(), 0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
                    RegCloseKey(hKey);
                    if (RegDeleteTreeW(rootKey, remaining.c_str()) == ERROR_SUCCESS) {
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
    ScanInvalidSharedDLL(items);
    ScanInvalidFonts(items);
    ScanInvalidHelpFiles(items);
    ScanInvalidAppPaths(items);
    ScanInvalidCOM(items);
    ScanInvalidMUI(items);
    ScanInvalidEnvVars(items);
    ScanInvalidTrayNotify(items);
    ScanInvalidSound(items);

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
// 扫描无效的共享DLL引用
// HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\SharedDLLs
// 每个值指向一个文件路径，如果文件不存在则无效
// ============================================================================

void RegistryCleaner::ScanInvalidSharedDLL(std::vector<RegistryInvalidItem>& items) {
    const std::wstring sharedDllKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SharedDLLs";
    auto valueNames = RegistryUtil::EnumValues(HKEY_LOCAL_MACHINE, sharedDllKey);

    for (const auto& valueName : valueNames) {
        // SharedDLLs中值名就是文件路径
        std::wstring filePath = Win32Util::ExpandEnvVars(valueName);
        if (filePath.empty()) continue;

        if (!PathExists(filePath)) {
            RegistryInvalidItem item;
            item.keyPath = L"HKLM\\" + sharedDllKey;
            item.valueName = valueName;
            item.invalidValue = filePath;
            item.description = L"无效的共享DLL引用: " + valueName;
            item.type = RegistryInvalidItem::Type::InvalidSharedDLL;
            items.push_back(item);
        }
    }
}

// ============================================================================
// 扫描无效的字体引用
// HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Fonts
// 检查字体文件路径是否存在
// ============================================================================

void RegistryCleaner::ScanInvalidFonts(std::vector<RegistryInvalidItem>& items) {
    const std::wstring fontsKey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
    auto valueNames = RegistryUtil::EnumValues(HKEY_LOCAL_MACHINE, fontsKey);

    for (const auto& valueName : valueNames) {
        std::wstring fontPath = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, fontsKey, valueName);
        if (fontPath.empty()) continue;

        std::wstring expandedPath = Win32Util::ExpandEnvVars(fontPath);

        // 字体可能使用相对路径（相对于字体目录）
        if (!PathExists(expandedPath)) {
            // 尝试在Windows\Fonts目录下查找
            wchar_t fontsDir[MAX_PATH] = {};
            if (GetWindowsDirectoryW(fontsDir, MAX_PATH)) {
                std::wstring fullFontPath = std::wstring(fontsDir) + L"\\Fonts\\" + fontPath;
                if (PathExists(fullFontPath)) {
                    continue; // 在字体目录中找到了
                }
            }

            RegistryInvalidItem item;
            item.keyPath = L"HKLM\\" + fontsKey;
            item.valueName = valueName;
            item.invalidValue = fontPath;
            item.description = L"无效的字体引用: " + valueName;
            item.type = RegistryInvalidItem::Type::InvalidFont;
            items.push_back(item);
        }
    }
}

// ============================================================================
// 扫描无效的帮助文件引用
// HKLM\SOFTWARE\Microsoft\Windows\HTML Help
// 检查帮助文件路径是否存在
// ============================================================================

void RegistryCleaner::ScanInvalidHelpFiles(std::vector<RegistryInvalidItem>& items) {
    const std::wstring helpKey = L"SOFTWARE\\Microsoft\\Windows\\HTML Help";
    if (!RegistryUtil::KeyExists(HKEY_LOCAL_MACHINE, helpKey)) return;

    auto valueNames = RegistryUtil::EnumValues(HKEY_LOCAL_MACHINE, helpKey);

    for (const auto& valueName : valueNames) {
        std::wstring helpPath = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, helpKey, valueName);
        if (helpPath.empty()) continue;

        std::wstring expandedPath = Win32Util::ExpandEnvVars(helpPath);
        if (!PathExists(expandedPath)) {
            RegistryInvalidItem item;
            item.keyPath = L"HKLM\\" + helpKey;
            item.valueName = valueName;
            item.invalidValue = helpPath;
            item.description = L"无效的帮助文件引用: " + valueName;
            item.type = RegistryInvalidItem::Type::InvalidHelpFile;
            items.push_back(item);
        }
    }
}

// ============================================================================
// 扫描无效的应用程序路径
// HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\*
// 检查Path值和默认值指向的文件是否存在
// ============================================================================

void RegistryCleaner::ScanInvalidAppPaths(std::vector<RegistryInvalidItem>& items) {
    const std::wstring appPathsKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths";
    auto subKeys = RegistryUtil::EnumSubKeys(HKEY_LOCAL_MACHINE, appPathsKey);

    for (const auto& appName : subKeys) {
        std::wstring fullKey = appPathsKey + L"\\" + appName;

        // 检查默认值（可执行文件路径）
        std::wstring defaultPath = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, fullKey, L"");
        if (!defaultPath.empty()) {
            std::wstring expandedPath = Win32Util::ExpandEnvVars(defaultPath);
            // 提取路径中的可执行文件
            std::wstring exePath = ExtractFilePath(expandedPath);
            if (!exePath.empty() && !PathExists(exePath)) {
                RegistryInvalidItem item;
                item.keyPath = L"HKLM\\" + fullKey;
                item.valueName = L"";
                item.invalidValue = defaultPath;
                item.description = L"无效的应用程序路径: " + appName;
                item.type = RegistryInvalidItem::Type::InvalidAppPath;
                items.push_back(item);
                continue; // 已经标记了该键，跳过Path值检查
            }
        }

        // 检查Path值（搜索路径）
        std::wstring pathValue = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, fullKey, L"Path");
        if (!pathValue.empty()) {
            // Path值可能包含多个路径，用分号分隔
            std::wstring expanded = Win32Util::ExpandEnvVars(pathValue);
            size_t start = 0;
            bool hasInvalidPath = false;
            while (start < expanded.size()) {
                size_t sep = expanded.find(L';', start);
                std::wstring singlePath = (sep != std::wstring::npos)
                    ? expanded.substr(start, sep - start) : expanded.substr(start);
                // 去除首尾空格
                size_t b = singlePath.find_first_not_of(L" \t");
                size_t e = singlePath.find_last_not_of(L" \t");
                if (b != std::wstring::npos && e != std::wstring::npos) {
                    singlePath = singlePath.substr(b, e - b + 1);
                    if (!singlePath.empty() && !PathExists(singlePath)) {
                        hasInvalidPath = true;
                        break;
                    }
                }
                if (sep == std::wstring::npos) break;
                start = sep + 1;
            }
            if (hasInvalidPath) {
                RegistryInvalidItem item;
                item.keyPath = L"HKLM\\" + fullKey;
                item.valueName = L"Path";
                item.invalidValue = pathValue;
                item.description = L"无效的应用程序搜索路径: " + appName;
                item.type = RegistryInvalidItem::Type::InvalidAppPath;
                items.push_back(item);
            }
        }
    }
}

// ============================================================================
// 扫描无效的COM/ActiveX组件
// HKCR\CLSID\{GUID}\InprocServer32 或 LocalServer32 指向的文件不存在
// 仅扫描有InprocServer32/LocalServer32的COM对象
// ============================================================================

void RegistryCleaner::ScanInvalidCOM(std::vector<RegistryInvalidItem>& items) {
    const std::wstring clsidKey = L"SOFTWARE\\Classes\\CLSID";
    auto subKeys = RegistryUtil::EnumSubKeys(HKEY_LOCAL_MACHINE, clsidKey);

    for (const auto& guid : subKeys) {
        // 只检查GUID格式的子键
        if (guid.size() < 38 || guid.front() != L'{') continue;

        std::wstring guidKey = clsidKey + L"\\" + guid;

        // 检查 InprocServer32
        std::wstring inprocKey = guidKey + L"\\InprocServer32";
        std::wstring inprocPath = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, inprocKey, L"");
        if (!inprocPath.empty()) {
            std::wstring expandedPath = Win32Util::ExpandEnvVars(inprocPath);
            std::wstring dllPath = ExtractFilePath(expandedPath);
            if (!dllPath.empty() && !PathExists(dllPath)) {
                RegistryInvalidItem item;
                item.keyPath = L"HKLM\\" + inprocKey;
                item.valueName = L"";
                item.invalidValue = inprocPath;
                item.description = L"无效的COM组件: " + guid;
                item.type = RegistryInvalidItem::Type::InvalidCOM;
                items.push_back(item);
                continue;
            }
        }

        // 检查 LocalServer32
        std::wstring localKey = guidKey + L"\\LocalServer32";
        std::wstring localPath = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, localKey, L"");
        if (!localPath.empty()) {
            std::wstring expandedPath = Win32Util::ExpandEnvVars(localPath);
            std::wstring exePath = ExtractFilePath(expandedPath);
            if (!exePath.empty() && !PathExists(exePath)) {
                RegistryInvalidItem item;
                item.keyPath = L"HKLM\\" + localKey;
                item.valueName = L"";
                item.invalidValue = localPath;
                item.description = L"无效的COM本地服务: " + guid;
                item.type = RegistryInvalidItem::Type::InvalidCOM;
                items.push_back(item);
            }
        }
    }
}

// ============================================================================
// 扫描无效的MUI缓存
// HKCU\SOFTWARE\Classes\Local Settings\MuiCache\*\*
// 检查应用程序路径是否存在
// ============================================================================

void RegistryCleaner::ScanInvalidMUI(std::vector<RegistryInvalidItem>& items) {
    const std::wstring muiCacheKey = L"SOFTWARE\\Classes\\Local Settings\\MuiCache";
    auto subKeys1 = RegistryUtil::EnumSubKeys(HKEY_CURRENT_USER, muiCacheKey);

    for (const auto& subKey1 : subKeys1) {
        std::wstring level1Key = muiCacheKey + L"\\" + subKey1;
        auto subKeys2 = RegistryUtil::EnumSubKeys(HKEY_CURRENT_USER, level1Key);

        for (const auto& subKey2 : subKeys2) {
            std::wstring level2Key = level1Key + L"\\" + subKey2;
            auto valueNames = RegistryUtil::EnumValues(HKEY_CURRENT_USER, level2Key);

            for (const auto& valueName : valueNames) {
                // MUI缓存值名通常是 "应用程序路径,资源ID" 格式
                size_t commaPos = valueName.find(L",");
                std::wstring appPath = (commaPos != std::wstring::npos)
                    ? valueName.substr(0, commaPos) : valueName;

                // 提取可执行文件路径
                std::wstring exePath = ExtractFilePath(Win32Util::ExpandEnvVars(appPath));
                if (!exePath.empty() && !PathExists(exePath)) {
                    RegistryInvalidItem item;
                    item.keyPath = L"HKCU\\" + level2Key;
                    item.valueName = valueName;
                    item.invalidValue = appPath;
                    item.description = L"无效的MUI缓存: " + appPath;
                    item.type = RegistryInvalidItem::Type::InvalidMUI;
                    items.push_back(item);
                }
            }
        }
    }
}

// ============================================================================
// 扫描环境变量中的无效路径
// HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment
// HKCU\Environment
// 检查PATH等路径变量中的目录是否存在
// ============================================================================

void RegistryCleaner::ScanInvalidEnvVars(std::vector<RegistryInvalidItem>& items) {
    // 检查系统PATH
    {
        const std::wstring envKey = L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
        std::wstring pathValue = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, envKey, L"Path");
        if (!pathValue.empty()) {
            std::wstring expanded = Win32Util::ExpandEnvVars(pathValue);
            size_t start = 0;
            while (start < expanded.size()) {
                size_t sep = expanded.find(L';', start);
                std::wstring singlePath = (sep != std::wstring::npos)
                    ? expanded.substr(start, sep - start) : expanded.substr(start);
                size_t b = singlePath.find_first_not_of(L" \t");
                size_t e = singlePath.find_last_not_of(L" \t");
                if (b != std::wstring::npos && e != std::wstring::npos) {
                    singlePath = singlePath.substr(b, e - b + 1);
                    if (!singlePath.empty() && !PathExists(singlePath)) {
                        RegistryInvalidItem item;
                        item.keyPath = L"HKLM\\" + envKey;
                        item.valueName = L"Path";
                        item.invalidValue = singlePath;
                        item.description = L"系统PATH中的无效路径: " + singlePath;
                        item.type = RegistryInvalidItem::Type::InvalidEnvVar;
                        items.push_back(item);
                    }
                }
                if (sep == std::wstring::npos) break;
                start = sep + 1;
            }
        }
    }

    // 检查用户PATH
    {
        const std::wstring envKey = L"Environment";
        std::wstring pathValue = RegistryUtil::ReadStringValue(HKEY_CURRENT_USER, envKey, L"Path");
        if (!pathValue.empty()) {
            std::wstring expanded = Win32Util::ExpandEnvVars(pathValue);
            size_t start = 0;
            while (start < expanded.size()) {
                size_t sep = expanded.find(L';', start);
                std::wstring singlePath = (sep != std::wstring::npos)
                    ? expanded.substr(start, sep - start) : expanded.substr(start);
                size_t b = singlePath.find_first_not_of(L" \t");
                size_t e = singlePath.find_last_not_of(L" \t");
                if (b != std::wstring::npos && e != std::wstring::npos) {
                    singlePath = singlePath.substr(b, e - b + 1);
                    if (!singlePath.empty() && !PathExists(singlePath)) {
                        RegistryInvalidItem item;
                        item.keyPath = L"HKCU\\" + envKey;
                        item.valueName = L"Path";
                        item.invalidValue = singlePath;
                        item.description = L"用户PATH中的无效路径: " + singlePath;
                        item.type = RegistryInvalidItem::Type::InvalidEnvVar;
                        items.push_back(item);
                    }
                }
                if (sep == std::wstring::npos) break;
                start = sep + 1;
            }
        }
    }
}

// ============================================================================
// 扫描无效的托盘通知缓存
// HKCU\SOFTWARE\Classes\Local Settings\Software\Microsoft\Windows\CurrentVersion\TrayNotify
// 检查IconStreams和PastIconsStream中引用的程序
// ============================================================================

void RegistryCleaner::ScanInvalidTrayNotify(std::vector<RegistryInvalidItem>& items) {
    const std::wstring trayKey = L"SOFTWARE\\Classes\\Local Settings\\Software\\Microsoft\\Windows\\CurrentVersion\\TrayNotify";
    if (!RegistryUtil::KeyExists(HKEY_CURRENT_USER, trayKey)) return;

    // 读取UserAssist中的程序路径
    // 检查已删除的托盘图标引用
    auto valueNames = RegistryUtil::EnumValues(HKEY_CURRENT_USER, trayKey);
    for (const auto& valueName : valueNames) {
        // 只检查字符串类型的值
        if (valueName == L"IconStreams" || valueName == L"PastIconsStream") {
            // 这些是二进制数据，包含程序路径信息
            // 简化处理：标记整个值为可清理
            // 实际应用中需要解析二进制格式提取路径
            continue;
        }

        std::wstring valueData = RegistryUtil::ReadStringValue(HKEY_CURRENT_USER, trayKey, valueName);
        if (valueData.empty()) continue;

        std::wstring expandedPath = Win32Util::ExpandEnvVars(valueData);
        std::wstring exePath = ExtractFilePath(expandedPath);
        if (!exePath.empty() && !PathExists(exePath)) {
            RegistryInvalidItem item;
            item.keyPath = L"HKCU\\" + trayKey;
            item.valueName = valueName;
            item.invalidValue = valueData;
            item.description = L"无效的托盘通知缓存: " + valueName;
            item.type = RegistryInvalidItem::Type::InvalidTrayNotify;
            items.push_back(item);
        }
    }
}

// ============================================================================
// 扫描无效的声音/事件关联
// HKCU\AppEvents\Schemes\Apps\*\*\.Current
// 检查声音文件路径是否存在
// ============================================================================

void RegistryCleaner::ScanInvalidSound(std::vector<RegistryInvalidItem>& items) {
    const std::wstring appEventsKey = L"AppEvents\\Schemes\\Apps";
    auto appSubKeys = RegistryUtil::EnumSubKeys(HKEY_CURRENT_USER, appEventsKey);

    for (const auto& app : appSubKeys) {
        std::wstring appKey = appEventsKey + L"\\" + app;
        auto eventSubKeys = RegistryUtil::EnumSubKeys(HKEY_CURRENT_USER, appKey);

        for (const auto& eventName : eventSubKeys) {
            std::wstring eventKey = appKey + L"\\" + eventName + L"\\.Current";
            std::wstring soundPath = RegistryUtil::ReadStringValue(HKEY_CURRENT_USER, eventKey, L"");
            if (soundPath.empty()) continue;

            // 空值或默认值（无声音）不算无效
            if (soundPath == L"." || soundPath == L"") continue;

            std::wstring expandedPath = Win32Util::ExpandEnvVars(soundPath);
            if (!PathExists(expandedPath)) {
                RegistryInvalidItem item;
                item.keyPath = L"HKCU\\" + eventKey;
                item.valueName = L"";
                item.invalidValue = soundPath;
                item.description = L"无效的声音关联: " + app + L" - " + eventName;
                item.type = RegistryInvalidItem::Type::InvalidSound;
                items.push_back(item);
            }
        }
    }
}

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
            if (RegOpenKeyExW(rootKey, subKeyPath.c_str(), 0, KEY_READ | KEY_WRITE | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                if (RegDeleteTreeW(rootKey, subKeyPath.c_str()) == ERROR_SUCCESS) {
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
