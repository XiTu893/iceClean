#include "StartupOptimizer.h"
#include "ScheduledTaskOptimizer.h"
#include "utils/RegistryUtil.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <shlobj.h>
#include <cstring>
#include <algorithm>

namespace IceClean::Core::Optimizer {

namespace {

// Explorer 启动状态权威标志位（任务管理器"启用/禁用"写的就是这里）
const wchar_t kApprovedRunSubKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\Run";
const wchar_t kApprovedFolderSubKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StartupApproved\\StartupFolder";
// 旧版破坏式禁用的备份位置
const wchar_t kLegacyBackupSubKey[] = L"Software\\IceClean\\DisabledStartup";

} // namespace

const std::wstring& StartupOptimizer::GetApprovedRunSubKey() {
    static const std::wstring k(kApprovedRunSubKey);
    return k;
}

const std::wstring& StartupOptimizer::GetApprovedFolderSubKey() {
    static const std::wstring k(kApprovedFolderSubKey);
    return k;
}

bool StartupOptimizer::IsStartupApprovedDisabled(HKEY rootKey, const std::wstring& approvedSubKey,
                                                 const std::wstring& valueName) {
    HKEY hKey = nullptr;
    LONG result = RegOpenKeyExW(rootKey, approvedSubKey.c_str(), 0,
                                KEY_READ | KEY_WOW64_64KEY, &hKey);
    if (result != ERROR_SUCCESS) return false;

    BYTE data[16] = {};
    DWORD dataSize = sizeof(data);
    result = RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr, data, &dataSize);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS || dataSize < 1) return false;

    // 约定：首字节 bit0 为 1 表示禁用（任务管理器写 0x03 + FILETIME）
    return (data[0] & 0x01) != 0;
}

bool StartupOptimizer::SetStartupApproved(HKEY rootKey, const std::wstring& approvedSubKey,
                                          const std::wstring& valueName, bool enabled) {
    if (enabled) {
        // 标志缺失或首字节 bit0=0 都视为已启用，无需操作
        if (!IsStartupApprovedDisabled(rootKey, approvedSubKey, valueName)) {
            return true;
        }
        // 清除标志即恢复启用（Explorer 对无标志条目默认放行）
        return Utils::RegistryUtil::DeleteValue(rootKey, approvedSubKey, valueName);
    }

    HKEY hKey = nullptr;
    LONG result = RegCreateKeyExW(rootKey, approvedSubKey.c_str(), 0, nullptr,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY,
                                  nullptr, &hKey, nullptr);
    if (result != ERROR_SUCCESS) return false;

    // 12 字节：禁用码(0x03) + 填充 + 当前 FILETIME，与资源管理器格式一致
    BYTE data[12] = {};
    data[0] = 0x03;
    FILETIME now{};
    GetSystemTimeAsFileTime(&now);
    static_assert(sizeof(FILETIME) == 8, "unexpected FILETIME size");
    memcpy(data + 4, &now, sizeof(now));

    result = RegSetValueExW(hKey, valueName.c_str(), 0, REG_BINARY, data, sizeof(data));
    RegCloseKey(hKey);

    return result == ERROR_SUCCESS;
}

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

std::vector<Models::StartupItem> StartupOptimizer::ReadRegistryStartupItems(
    HKEY rootKey, const std::wstring& subKey, const std::wstring& approvedSubKey) {
    std::vector<Models::StartupItem> items;

    auto valueNames = Utils::RegistryUtil::EnumValues(rootKey, subKey);
    for (const auto& valueName : valueNames) {
        std::wstring value = Utils::RegistryUtil::ReadStringValue(rootKey, subKey, valueName);
        if (value.empty()) continue;

        Models::StartupItem item;
        item.name = valueName;
        item.path = value;
        item.type = Models::StartupItemType::Registry;
        // 真实启用状态以 StartupApproved 标志为准
        item.isEnabled = !IsStartupApprovedDisabled(rootKey, approvedSubKey, valueName);
        item.isSystemCritical = IsCriticalItem(valueName, value);
        item.canDisable = !item.isSystemCritical;

        // 从文件版本信息提取发布者
        item.publisher = Utils::Win32Util::GetFilePublisher(value);

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
            item.isEnabled = !IsStartupApprovedDisabled(HKEY_CURRENT_USER,
                                                        GetApprovedFolderSubKey(), fileName);
            item.isSystemCritical = IsCriticalItem(fileName, filePath);
            item.canDisable = !item.isSystemCritical;
            item.publisher = Utils::Win32Util::GetFilePublisher(filePath);

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
            // 公共启动项的审批标志同样存于当前用户的 HKCU
            item.isEnabled = !IsStartupApprovedDisabled(HKEY_CURRENT_USER,
                                                        GetApprovedFolderSubKey(), fileName);
            item.isSystemCritical = IsCriticalItem(fileName, filePath);
            item.canDisable = !item.isSystemCritical;
            item.publisher = Utils::Win32Util::GetFilePublisher(filePath);

            items.push_back(item);
        }
    }

    return items;
}

std::vector<Models::StartupItem> StartupOptimizer::GetStartupItems() {
    std::vector<Models::StartupItem> items;

    // 读取注册表启动项
    auto hkcuRun = ReadRegistryStartupItems(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", GetApprovedRunSubKey());
    auto hkcuRunOnce = ReadRegistryStartupItems(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", GetApprovedRunSubKey());
    auto hklmRun = ReadRegistryStartupItems(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", GetApprovedRunSubKey());
    auto hklmRunOnce = ReadRegistryStartupItems(HKEY_LOCAL_MACHINE,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce", GetApprovedRunSubKey());

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

bool StartupOptimizer::DisableRegistryItemAt(HKEY rootKey, const std::wstring& runSubKey,
                                             const std::wstring& approvedSubKey,
                                             const std::wstring& valueName) {
    // 已被标志禁用 → 幂等成功
    if (IsStartupApprovedDisabled(rootKey, approvedSubKey, valueName)) {
        return true;
    }

    std::wstring value = Utils::RegistryUtil::ReadStringValue(rootKey, runSubKey, valueName);
    if (value.empty()) {
        // Run 值不存在：可能处于旧版"备份+删除"禁用态
        std::wstring backup = Utils::RegistryUtil::ReadStringValue(
            HKEY_CURRENT_USER, kLegacyBackupSubKey, valueName);
        if (backup.empty()) {
            backup = Utils::RegistryUtil::ReadStringValue(
                HKEY_LOCAL_MACHINE, kLegacyBackupSubKey, valueName);
        }
        return !backup.empty();
    }

    // 首选：StartupApproved 标志（非破坏式，应用自愈也无法复活——
    // Explorer 在登录时依据该标志跳过启动，即使应用重写 Run 值）
    if (SetStartupApproved(rootKey, approvedSubKey, valueName, false)) {
        return true;
    }

    // 回退：旧版"备份+删除"
    return DisableRegistryItem(rootKey, runSubKey, valueName);
}

bool StartupOptimizer::EnableRegistryItemAt(HKEY rootKey, const std::wstring& runSubKey,
                                            const std::wstring& approvedSubKey,
                                            const std::wstring& valueName) {
    // 优先清除 StartupApproved 标志
    if (IsStartupApprovedDisabled(rootKey, approvedSubKey, valueName)) {
        return SetStartupApproved(rootKey, approvedSubKey, valueName, true);
    }

    // Run 值仍在 → 本就处于启用态
    std::wstring value = Utils::RegistryUtil::ReadStringValue(rootKey, runSubKey, valueName);
    if (!value.empty()) {
        return true;
    }

    // 旧版机制恢复：从备份键还原
    for (HKEY root : { HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE }) {
        std::wstring backup = Utils::RegistryUtil::ReadStringValue(
            root, kLegacyBackupSubKey, valueName);
        if (!backup.empty()) {
            return EnableRegistryItem(root, runSubKey, valueName, backup);
        }
    }
    return false;
}

bool StartupOptimizer::DisableItem(const Models::StartupItem& item) {
    if (item.isSystemCritical || !item.canDisable) return false;

    if (item.type == Models::StartupItemType::Registry) {
        // 确定注册表位置
        struct RegLocation {
            HKEY rootKey;
            std::wstring subKey;
        };

        const std::vector<RegLocation> locations = {
            { HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" },
            { HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" },
            { HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" },
            { HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" },
        };

        for (const auto& loc : locations) {
            std::wstring value = Utils::RegistryUtil::ReadStringValue(loc.rootKey, loc.subKey, item.name);
            if (!value.empty() ||
                IsStartupApprovedDisabled(loc.rootKey, GetApprovedRunSubKey(), item.name)) {
                return DisableRegistryItemAt(loc.rootKey, loc.subKey,
                                             GetApprovedRunSubKey(), item.name);
            }
        }

        return false;
    }

    if (item.type == Models::StartupItemType::StartupFolder) {
        // 首选 StartupFolder 审批标志；失败时回退重命名 .disabled
        if (SetStartupApproved(HKEY_CURRENT_USER, GetApprovedFolderSubKey(),
                               item.name, false)) {
            return true;
        }
        std::wstring disabledPath = item.path + L".disabled";
        return MoveFileW(item.path.c_str(), disabledPath.c_str()) != 0;
    }

    if (item.type == Models::StartupItemType::ScheduledTask) {
        // 对于计划任务，委托给ScheduledTaskOptimizer禁用
        ScheduledTaskOptimizer taskOpt;
        return taskOpt.DisableTask(item.path, item.name);
    }

    return false;
}

bool StartupOptimizer::EnableItem(const Models::StartupItem& item) {
    if (item.type == Models::StartupItemType::Registry) {
        struct RegLocation {
            HKEY rootKey;
            std::wstring subKey;
        };

        const std::vector<RegLocation> locations = {
            { HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" },
            { HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" },
            { HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run" },
            { HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce" },
        };

        for (const auto& loc : locations) {
            bool hasValue = !Utils::RegistryUtil::ReadStringValue(loc.rootKey, loc.subKey, item.name).empty();
            bool approvedOff = IsStartupApprovedDisabled(loc.rootKey, GetApprovedRunSubKey(), item.name);
            if (hasValue || approvedOff) {
                return EnableRegistryItemAt(loc.rootKey, loc.subKey,
                                            GetApprovedRunSubKey(), item.name);
            }
        }

        return false;
    }

    if (item.type == Models::StartupItemType::StartupFolder) {
        // 清除审批标志即恢复
        if (IsStartupApprovedDisabled(HKEY_CURRENT_USER, GetApprovedFolderSubKey(), item.name)) {
            return SetStartupApproved(HKEY_CURRENT_USER, GetApprovedFolderSubKey(),
                                      item.name, true);
        }
        // 兼容旧版 .disabled 重命名
        std::wstring disabledPath = item.path + L".disabled";
        if (Utils::FileUtil::Exists(disabledPath)) {
            return MoveFileW(disabledPath.c_str(), item.path.c_str()) != 0;
        }
        return Utils::FileUtil::Exists(item.path);
    }

    if (item.type == Models::StartupItemType::ScheduledTask) {
        // 对于计划任务，委托给ScheduledTaskOptimizer启用
        ScheduledTaskOptimizer taskOpt;
        return taskOpt.EnableTask(item.path, item.name);
    }

    return false;
}

} // namespace IceClean::Core::Optimizer
