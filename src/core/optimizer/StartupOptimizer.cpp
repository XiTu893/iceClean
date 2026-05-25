#include "StartupOptimizer.h"
#include "utils/RegistryUtil.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <shlobj.h>
#include <algorithm>

namespace IceClean::Core::Optimizer {

const std::vector<std::wstring>& StartupOptimizer::GetCriticalNames() {
    static const std::vector<std::wstring> names = {
        L"securityhealth",
        L"windowsdefender",
        L"msmpeng",
        L"nissrv",
        L"securityhealthservice",
        L"ctfmon",
        L"taskhostw",
        L"explorer",
    };
    return names;
}

bool StartupOptimizer::IsCriticalItem(const std::wstring& name, const std::wstring& path) const {
    std::wstring lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), towlower);

    for (const auto& critical : GetCriticalNames()) {
        if (lowerName.find(critical) != std::wstring::npos) {
            return true;
        }
    }

    // 检查路径中是否包含Windows Defender等关键路径
    std::wstring lowerPath = path;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), towlower);

    if (lowerPath.find(L"windows defender") != std::wstring::npos ||
        lowerPath.find(L"securityhealth") != std::wstring::npos ||
        lowerPath.find(L"msmpeng") != std::wstring::npos) {
        return true;
    }

    return false;
}

std::wstring StartupOptimizer::GetStartupFolderPath() const {
    return Utils::Win32Util::GetSpecialFolder(CSIDL_STARTUP);
}

std::wstring StartupOptimizer::GetCommonStartupFolderPath() const {
    return Utils::Win32Util::GetSpecialFolder(CSIDL_COMMON_STARTUP);
}

std::vector<Models::StartupItem> StartupOptimizer::ReadRegistryStartupItems(HKEY rootKey, const std::wstring& subKey) {
    std::vector<Models::StartupItem> items;

    auto valueNames = Utils::RegistryUtil::EnumValues(rootKey, subKey);
    for (const auto& valueName : valueNames) {
        std::wstring value = Utils::RegistryUtil::ReadStringValue(rootKey, subKey, valueName);
        if (value.empty()) continue;

        Models::StartupItem item;
        item.name = valueName;
        item.path = value;
        item.type = Models::StartupItemType::Registry;
        item.isEnabled = true;
        item.isSystemCritical = IsCriticalItem(valueName, value);
        item.canDisable = !item.isSystemCritical;

        items.push_back(item);
    }

    return items;
}

std::vector<Models::StartupItem> StartupOptimizer::ReadStartupFolderItems() {
    std::vector<Models::StartupItem> items;

    // 用户启动文件夹
    std::wstring userStartup = GetStartupFolderPath();
    if (!userStartup.empty() && Utils::FileUtil::Exists(userStartup)) {
        std::vector<std::wstring> files;
        Utils::FileUtil::ScanFiles(userStartup, L"*.lnk", files, false);
        Utils::FileUtil::ScanFiles(userStartup, L"*.exe", files, false);

        for (const auto& filePath : files) {
            // 提取文件名
            auto lastSlash = filePath.find_last_of(L'\\');
            std::wstring fileName = (lastSlash != std::wstring::npos)
                ? filePath.substr(lastSlash + 1) : filePath;

            Models::StartupItem item;
            item.name = fileName;
            item.path = filePath;
            item.type = Models::StartupItemType::StartupFolder;
            item.isEnabled = true;
            item.isSystemCritical = IsCriticalItem(fileName, filePath);
            item.canDisable = !item.isSystemCritical;

            items.push_back(item);
        }
    }

    // 公共启动文件夹
    std::wstring commonStartup = GetCommonStartupFolderPath();
    if (!commonStartup.empty() && Utils::FileUtil::Exists(commonStartup) && commonStartup != userStartup) {
        std::vector<std::wstring> files;
        Utils::FileUtil::ScanFiles(commonStartup, L"*.lnk", files, false);
        Utils::FileUtil::ScanFiles(commonStartup, L"*.exe", files, false);

        for (const auto& filePath : files) {
            auto lastSlash = filePath.find_last_of(L'\\');
            std::wstring fileName = (lastSlash != std::wstring::npos)
                ? filePath.substr(lastSlash + 1) : filePath;

            Models::StartupItem item;
            item.name = fileName;
            item.path = filePath;
            item.type = Models::StartupItemType::StartupFolder;
            item.isEnabled = true;
            item.isSystemCritical = IsCriticalItem(fileName, filePath);
            item.canDisable = !item.isSystemCritical;

            items.push_back(item);
        }
    }

    return items;
}

std::vector<Models::StartupItem> StartupOptimizer::GetStartupItems() {
    std::vector<Models::StartupItem> items;

    // 读取注册表启动项
    auto hkcuRun = ReadRegistryStartupItems(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    auto hkcuRunOnce = ReadRegistryStartupItems(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");
    auto hklmRun = ReadRegistryStartupItems(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
    auto hklmRunOnce = ReadRegistryStartupItems(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce");

    items.insert(items.end(), hkcuRun.begin(), hkcuRun.end());
    items.insert(items.end(), hkcuRunOnce.begin(), hkcuRunOnce.end());
    items.insert(items.end(), hklmRun.begin(), hklmRun.end());
    items.insert(items.end(), hklmRunOnce.begin(), hklmRunOnce.end());

    // 读取启动文件夹项
    auto folderItems = ReadStartupFolderItems();
    items.insert(items.end(), folderItems.begin(), folderItems.end());

    return items;
}

bool StartupOptimizer::DisableRegistryItem(HKEY rootKey, const std::wstring& subKey,
                                             const std::wstring& valueName) {
    // 将启动项从Run键移动到IceClean备份键
    std::wstring backupSubKey = L"Software\\IceClean\\DisabledStartup";

    // 读取当前值
    std::wstring value = Utils::RegistryUtil::ReadStringValue(rootKey, subKey, valueName);
    if (value.empty()) return false;

    // 保存到备份位置
    if (!Utils::RegistryUtil::WriteStringValue(rootKey, backupSubKey, valueName, value)) {
        return false;
    }

    // 从原位置删除
    if (!Utils::RegistryUtil::DeleteValue(rootKey, subKey, valueName)) {
        // 删除失败，回滚备份
        Utils::RegistryUtil::DeleteValue(rootKey, backupSubKey, valueName);
        return false;
    }

    return true;
}

bool StartupOptimizer::EnableRegistryItem(HKEY rootKey, const std::wstring& subKey,
                                            const std::wstring& valueName, const std::wstring& value) {
    // 将启动项恢复到Run键
    if (!Utils::RegistryUtil::WriteStringValue(rootKey, subKey, valueName, value)) {
        return false;
    }

    // 从备份位置删除
    std::wstring backupSubKey = L"Software\\IceClean\\DisabledStartup";
    Utils::RegistryUtil::DeleteValue(rootKey, backupSubKey, valueName);

    return true;
}

bool StartupOptimizer::DisableItem(const Models::StartupItem& item) {
    if (item.isSystemCritical || !item.canDisable) return false;

    if (item.type == Models::StartupItemType::Registry) {
        // 确定注册表位置
        // 尝试在所有可能的注册表位置查找
        struct RegLocation {
            HKEY rootKey;
            std::wstring subKey;
        };

        std::vector<RegLocation> locations = {
            { HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" },
            { HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" },
            { HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" },
            { HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" },
        };

        for (const auto& loc : locations) {
            std::wstring value = Utils::RegistryUtil::ReadStringValue(loc.rootKey, loc.subKey, item.name);
            if (!value.empty()) {
                return DisableRegistryItem(loc.rootKey, loc.subKey, item.name);
            }
        }

        return false;
    }

    if (item.type == Models::StartupItemType::StartupFolder) {
        // 对于启动文件夹项，重命名文件添加.disabled后缀
        std::wstring disabledPath = item.path + L".disabled";
        return MoveFileW(item.path.c_str(), disabledPath.c_str()) != 0;
    }

    return false;
}

bool StartupOptimizer::EnableItem(const Models::StartupItem& item) {
    if (item.type == Models::StartupItemType::Registry) {
        // 从备份位置恢复
        std::wstring backupSubKey = L"Software\\IceClean\\DisabledStartup";

        struct RegLocation {
            HKEY rootKey;
            std::wstring subKey;
        };

        std::vector<RegLocation> locations = {
            { HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" },
            { HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" },
            { HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" },
            { HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" },
        };

        for (const auto& loc : locations) {
            std::wstring value = Utils::RegistryUtil::ReadStringValue(loc.rootKey, backupSubKey, item.name);
            if (!value.empty()) {
                return EnableRegistryItem(loc.rootKey, loc.subKey, item.name, value);
            }
        }

        return false;
    }

    if (item.type == Models::StartupItemType::StartupFolder) {
        // 恢复被禁用的启动文件夹项
        std::wstring disabledPath = item.path + L".disabled";
        if (Utils::FileUtil::Exists(disabledPath)) {
            return MoveFileW(disabledPath.c_str(), item.path.c_str()) != 0;
        }
    }

    return false;
}

} // namespace IceClean::Core::Optimizer
