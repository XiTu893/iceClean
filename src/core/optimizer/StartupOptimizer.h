#pragma once
#include "models/StartupItem.h"
#include <vector>
#include <string>
#include <windows.h>

namespace IceClean::Core::Optimizer {

class StartupOptimizer {
public:
    // 获取所有启动项
    std::vector<Models::StartupItem> GetStartupItems();

    // 禁用启动项
    bool DisableItem(const Models::StartupItem& item);

    // 启用启动项
    bool EnableItem(const Models::StartupItem& item);

private:
    // 从注册表读取启动项
    std::vector<Models::StartupItem> ReadRegistryStartupItems(HKEY rootKey, const std::wstring& subKey);

    // 从启动文件夹读取启动项
    std::vector<Models::StartupItem> ReadStartupFolderItems();

    // 检查是否为关键启动项(不可禁用)
    bool IsCriticalItem(const std::wstring& name, const std::wstring& path) const;

    // 获取启动文件夹路径
    std::wstring GetStartupFolderPath() const;
    std::wstring GetCommonStartupFolderPath() const;

    // 在注册表中禁用/启用启动项
    bool DisableRegistryItem(HKEY rootKey, const std::wstring& subKey, const std::wstring& valueName);
    bool EnableRegistryItem(HKEY rootKey, const std::wstring& subKey,
                            const std::wstring& valueName, const std::wstring& value);

    // 关键启动项白名单(小写)
    static const std::vector<std::wstring>& GetCriticalNames();
};

} // namespace IceClean::Core::Optimizer
