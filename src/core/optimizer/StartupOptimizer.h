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

    // ── StartupApproved 机制（任务管理器同款，非破坏式）──
    // 判断指定项是否被 StartupApproved 标志禁用（值缺失视为启用）
    static bool IsStartupApprovedDisabled(HKEY rootKey, const std::wstring& approvedSubKey,
                                          const std::wstring& valueName);
    // 写入/清除 StartupApproved 标志；enabled=false 写入禁用字节，enabled=true 删除标志
    static bool SetStartupApproved(HKEY rootKey, const std::wstring& approvedSubKey,
                                   const std::wstring& valueName, bool enabled);

    // 真实 StartupApproved 键路径（读侧默认参数使用）
    static const std::wstring& GetApprovedRunSubKey();
    static const std::wstring& GetApprovedFolderSubKey();

    // 指定位置的注册表启动项禁用/启用（生产传真实键，测试传沙箱键）
    // 首选 StartupApproved 标志；写入失败时回退到"备份+删除"旧机制
    bool DisableRegistryItemAt(HKEY rootKey, const std::wstring& runSubKey,
                               const std::wstring& approvedSubKey, const std::wstring& valueName);
    bool EnableRegistryItemAt(HKEY rootKey, const std::wstring& runSubKey,
                              const std::wstring& approvedSubKey, const std::wstring& valueName);

private:
    // 从注册表读取启动项（approvedSubKey 用于查询禁用标志）
    std::vector<Models::StartupItem> ReadRegistryStartupItems(HKEY rootKey, const std::wstring& subKey,
                                                              const std::wstring& approvedSubKey);

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
