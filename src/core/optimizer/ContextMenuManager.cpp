#include "ContextMenuManager.h"
#include "utils/RegistryUtil.h"
#include "utils/Win32Util.h"
#include <algorithm>

namespace IceClean::Core::Optimizer {

using namespace IceClean::Utils;

std::vector<ContextMenuItem> ContextMenuManager::ScanContextMenu() {
    std::vector<ContextMenuItem> items;

    // 文件右键菜单
    ScanShellMenu(HKEY_CLASSES_ROOT, L"*\\shell", ContextMenuItem::Location::File, items);
    ScanContextMenuHandlers(HKEY_CLASSES_ROOT, L"*\\shellex\\ContextMenuHandlers",
                            ContextMenuItem::Location::File, items);

    // 文件夹右键菜单
    ScanShellMenu(HKEY_CLASSES_ROOT, L"Folder\\shell", ContextMenuItem::Location::Folder, items);
    ScanContextMenuHandlers(HKEY_CLASSES_ROOT, L"Folder\\shellex\\ContextMenuHandlers",
                            ContextMenuItem::Location::Folder, items);

    // 目录背景右键
    ScanShellMenu(HKEY_CLASSES_ROOT, L"Directory\\Background\\shell",
                  ContextMenuItem::Location::Directory, items);
    ScanContextMenuHandlers(HKEY_CLASSES_ROOT, L"Directory\\Background\\shellex\\ContextMenuHandlers",
                            ContextMenuItem::Location::Directory, items);

    // 目录右键菜单
    ScanShellMenu(HKEY_CLASSES_ROOT, L"Directory\\shell", ContextMenuItem::Location::Folder, items);
    ScanContextMenuHandlers(HKEY_CLASSES_ROOT, L"Directory\\shellex\\ContextMenuHandlers",
                            ContextMenuItem::Location::Folder, items);

    // 驱动器右键
    ScanShellMenu(HKEY_CLASSES_ROOT, L"Drive\\shell", ContextMenuItem::Location::Drive, items);
    ScanContextMenuHandlers(HKEY_CLASSES_ROOT, L"Drive\\shellex\\ContextMenuHandlers",
                            ContextMenuItem::Location::Drive, items);

    // 桌面背景右键
    ScanShellMenu(HKEY_CLASSES_ROOT, L"DesktopBackground\\shell",
                  ContextMenuItem::Location::Desktop, items);
    ScanContextMenuHandlers(HKEY_CLASSES_ROOT, L"DesktopBackground\\shellex\\ContextMenuHandlers",
                            ContextMenuItem::Location::Desktop, items);

    return items;
}

void ContextMenuManager::ScanShellMenu(HKEY rootKey, const std::wstring& basePath,
                                        ContextMenuItem::Location location,
                                        std::vector<ContextMenuItem>& items) {
    auto subKeys = RegistryUtil::EnumSubKeys(rootKey, basePath);
    if (subKeys.empty()) return;

    for (const auto& subKey : subKeys) {
        std::wstring fullKey = basePath + L"\\" + subKey;

        // 跳过已禁用项（以 _disabled_ 开头的）
        bool isDisabled = (subKey.find(L"_disabled_") == 0);

        // 读取显示文本
        std::wstring displayText;
        // 先读MUIVerb
        std::wstring muiText = RegistryUtil::ReadStringValue(rootKey, fullKey, L"MUIVerb");
        if (!muiText.empty()) {
            displayText = muiText;
        } else {
            // 读默认值
            displayText = RegistryUtil::ReadStringValue(rootKey, fullKey, L"");
            if (displayText.empty()) {
                // 使用键名作为显示文本
                displayText = subKey;
                // 去掉 _disabled_ 前缀
                if (isDisabled && displayText.length() > 10) {
                    displayText = displayText.substr(10);
                }
            }
        }

        // 读取命令
        std::wstring commandKey = fullKey + L"\\command";
        std::wstring command = RegistryUtil::ReadStringValue(rootKey, commandKey, L"");

        // 读取图标
        std::wstring icon = RegistryUtil::ReadStringValue(rootKey, fullKey, L"Icon");
        if (icon.empty()) {
            icon = RegistryUtil::ReadStringValue(rootKey, fullKey, L"DefaultIcon");
        }

        // 检查是否为系统项（用键名比较，而非完整路径）
        bool isSystem = IsSystemItem(subKey, displayText);

        // 构建路径前缀
        std::wstring keyPathPrefix;
        if (rootKey == HKEY_CLASSES_ROOT) {
            keyPathPrefix = L"HKCR\\";
        }

        ContextMenuItem item;
        item.keyPath = keyPathPrefix + fullKey;
        item.valueName = subKey;
        item.command = command;
        item.displayText = displayText;
        item.iconPath = icon;
        item.location = location;
        item.isSystem = isSystem;
        item.isEnabled = !isDisabled;

        items.push_back(item);
    }
}

void ContextMenuManager::ScanContextMenuHandlers(HKEY rootKey, const std::wstring& basePath,
                                                   ContextMenuItem::Location location,
                                                   std::vector<ContextMenuItem>& items) {
    auto subKeys = RegistryUtil::EnumSubKeys(rootKey, basePath);
    if (subKeys.empty()) return;

    for (const auto& subKey : subKeys) {
        std::wstring fullKey = basePath + L"\\" + subKey;

        // 跳过已禁用项
        bool isDisabled = (subKey.find(L"_disabled_") == 0);

        // 读取 CLSID（默认值）
        std::wstring clsid = RegistryUtil::ReadStringValue(rootKey, fullKey, L"");

        // 尝试从 CLSID 获取友好的名称
        std::wstring displayText = subKey;
        if (!clsid.empty()) {
            // 从 HKCR\CLSID\{clsid} 读取 COM 对象名称
            std::wstring clsidPath = L"CLSID\\" + clsid;
            std::wstring clsidName = RegistryUtil::ReadStringValue(HKEY_CLASSES_ROOT, clsidPath, L"");
            if (!clsidName.empty()) {
                displayText = clsidName;
            }
        }

        // 去掉 _disabled_ 前缀的显示文本
        std::wstring cleanSubKey = subKey;
        if (isDisabled && cleanSubKey.length() > 10) {
            cleanSubKey = cleanSubKey.substr(10);
        }

        // 检查是否为系统项
        bool isSystem = IsSystemItem(cleanSubKey, displayText);

        // 构建路径前缀
        std::wstring keyPathPrefix;
        if (rootKey == HKEY_CLASSES_ROOT) {
            keyPathPrefix = L"HKCR\\";
        }

        ContextMenuItem item;
        item.keyPath = keyPathPrefix + fullKey;
        item.valueName = subKey;
        item.command = clsid;  // 对于 COM 扩展，command 字段存储 CLSID
        item.displayText = displayText;
        item.iconPath = L"";
        item.location = location;
        item.isSystem = isSystem;
        item.isEnabled = !isDisabled;

        items.push_back(item);
    }
}

bool ContextMenuManager::DisableItem(const ContextMenuItem& item) {
    // 通过重命名注册表键来禁用
    if (item.keyPath.find(L"HKCR\\") != 0) return false;

    std::wstring subKey = item.keyPath.substr(5);

    // 找到最后一个反斜杠，分离父键和子键名
    size_t lastSlash = subKey.rfind(L'\\');
    if (lastSlash == std::wstring::npos) return false;

    std::wstring parentKey = subKey.substr(0, lastSlash);
    std::wstring keyName = subKey.substr(lastSlash + 1);

    if (keyName.empty()) return false;

    // 构建新键名(添加 _disabled_ 前缀)
    std::wstring newKeyName = L"_disabled_" + keyName;
    std::wstring newSubKey = parentKey + L"\\" + newKeyName;

    // 复制注册表键(通过reg命令)
    std::wstring command = L"reg copy \"HKCR\\" + subKey + L"\" \"HKCR\\" + newSubKey +
                           L"\" /s /f";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    std::wstring cmdLine = L"cmd.exe /c " + command;
    BOOL success = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) return false;

    WaitForSingleObject(pi.hProcess, 10000);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) return false;

    // 删除原始键
    command = L"reg delete \"HKCR\\" + subKey + L"\" /f";
    cmdLine = L"cmd.exe /c " + command;
    success = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                              CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) return false;

    WaitForSingleObject(pi.hProcess, 10000);
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

bool ContextMenuManager::EnableItem(const ContextMenuItem& item) {
    // 恢复被禁用的项(移除 _disabled_ 前缀)
    if (item.keyPath.find(L"HKCR\\") != 0) return false;

    // 查找 _disabled_ 前缀的键
    std::wstring subKey = item.keyPath.substr(5);

    // 构建禁用后的键路径
    size_t lastSlash = subKey.rfind(L'\\');
    if (lastSlash == std::wstring::npos) return false;

    std::wstring parentKey = subKey.substr(0, lastSlash);
    std::wstring keyName = subKey.substr(lastSlash + 1);

    std::wstring disabledKeyName = L"_disabled_" + keyName;
    std::wstring disabledSubKey = parentKey + L"\\" + disabledKeyName;

    // 检查禁用键是否存在
    if (!RegistryUtil::KeyExists(HKEY_CLASSES_ROOT, disabledSubKey)) {
        return false;
    }

    // 恢复原始键
    std::wstring command = L"reg copy \"HKCR\\" + disabledSubKey + L"\" \"HKCR\\" +
                           subKey + L"\" /s /f";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    std::wstring cmdLine = L"cmd.exe /c " + command;
    BOOL success = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) return false;

    WaitForSingleObject(pi.hProcess, 10000);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) return false;

    // 删除禁用键
    command = L"reg delete \"HKCR\\" + disabledSubKey + L"\" /f";
    cmdLine = L"cmd.exe /c " + command;
    CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

bool ContextMenuManager::DeleteItem(const ContextMenuItem& item) {
    if (item.keyPath.find(L"HKCR\\") != 0) return false;

    std::wstring subKey = item.keyPath.substr(5);

    std::wstring command = L"reg delete \"HKCR\\" + subKey + L"\" /f";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    std::wstring cmdLine = L"cmd.exe /c " + command;
    BOOL success = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) return false;

    WaitForSingleObject(pi.hProcess, 10000);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

std::wstring ContextMenuManager::GetLocationName(ContextMenuItem::Location location) {
    switch (location) {
    case ContextMenuItem::Location::File:      return L"文件";
    case ContextMenuItem::Location::Folder:    return L"文件夹";
    case ContextMenuItem::Location::Desktop:   return L"桌面";
    case ContextMenuItem::Location::Drive:     return L"驱动器";
    case ContextMenuItem::Location::Directory: return L"目录背景";
    default:                                   return L"其他";
    }
}

ContextMenuItem::Location ContextMenuManager::DetermineLocation(const std::wstring& keyPath) const {
    if (keyPath.find(L"*\\shell") != std::wstring::npos ||
        keyPath.find(L"*\\shellex") != std::wstring::npos) return ContextMenuItem::Location::File;
    if (keyPath.find(L"Directory\\Background") != std::wstring::npos) return ContextMenuItem::Location::Directory;
    if (keyPath.find(L"DesktopBackground") != std::wstring::npos) return ContextMenuItem::Location::Desktop;
    if (keyPath.find(L"Drive\\") != std::wstring::npos) return ContextMenuItem::Location::Drive;
    if (keyPath.find(L"Folder\\") != std::wstring::npos ||
        keyPath.find(L"Directory\\") != std::wstring::npos) return ContextMenuItem::Location::Folder;
    return ContextMenuItem::Location::Unknown;
}

bool ContextMenuManager::IsSystemItem(const std::wstring& keyName, const std::wstring& displayText) const {
    // 已知系统右键菜单项（使用键名比较）
    static const std::vector<std::wstring> systemKeys = {
        L"open", L"edit", L"print", L"runas", L"runasuser",
        L"cmd", L"PowerShell", L"git_shell", L"git_gui",
        L"VSCode", L"Code", L"OpenWithSetDefaultOn",
        L"opennewprocess", L"opencontaining", L"pintohome",
        L"cut", L"copy", L"paste", L"delete", L"rename",
        L"properties", L"New", L"IncludeInLibrary",
        L"SendTo", L"Share", L"Extract", L"Play",
        L"AddToPlaylist", L"AddToQueue",
        L"OpenWith", L"ModernOpen", L"OpenWithCmd", L"PintoHome",
        // 常见系统 COM 扩展
        L"OpenWith", L"{09799AFB-AD67-11d1-ABCD-00C04FC30936}", // OpenWith
        L"Compatibility", L"WorkFolders", L"PlayTo",
        L"{2BE71A57-852D-441F-A4F4-581DF6584567}", // 排序
        L"{4A5F0F70-5AC6-477A-88E2-62CEBF8A04E1}"  // 包含在库中
    };

    // 使用键名比较
    for (const auto& sys : systemKeys) {
        if (_wcsicmp(keyName.c_str(), sys.c_str()) == 0) {
            return true;
        }
    }

    return false;
}

} // namespace IceClean::Core::Optimizer
