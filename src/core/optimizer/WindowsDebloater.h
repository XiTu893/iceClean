#pragma once
#include "models/DebloatItem.h"
#include <vector>
#include <string>
#include <functional>
#include <atomic>

namespace IceClean::Core::Optimizer {

class WindowsDebloater {
public:
    WindowsDebloater();

    // 获取所有精简项
    std::vector<Models::DebloatItem> GetDebloatItems() const;

    // 获取预设配置
    std::vector<Models::DebloatPreset> GetPresets() const;

    // 应用精简项
    bool ApplyItem(const Models::DebloatItem& item);

    // 批量应用
    Models::DebloatResult ApplyItems(const std::vector<Models::DebloatItem>& items,
                                      std::function<void(int, int)> progressCallback = nullptr,
                                      const std::atomic<bool>* cancelFlag = nullptr);

    // 还原精简项
    bool RevertItem(const Models::DebloatItem& item);

    // 扫描当前已安装的 AppxPackage
    std::vector<std::wstring> GetInstalledAppxPackages() const;

    // 移除 AppxPackage
    bool RemoveAppxPackage(const std::wstring& packageName) const;

private:
    // 初始化精简项列表
    void InitializeItems();

    // Appx 包移除
    bool ApplyAppxPackage(const Models::DebloatItem& item) const;

    // 系统组件禁用
    bool ApplySystemComponent(const Models::DebloatItem& item) const;

    // 遥测禁用
    bool ApplyTelemetry(const Models::DebloatItem& item) const;

    // 服务禁用
    bool ApplyService(const Models::DebloatItem& item) const;

    // 计划任务禁用
    bool ApplyScheduledTask(const Models::DebloatItem& item) const;

    // 上下文菜单修改
    bool ApplyContextMenu(const Models::DebloatItem& item) const;

    // 注册表优化
    bool ApplyRegistryTweak(const Models::DebloatItem& item) const;

    std::vector<Models::DebloatItem> m_items;
};

} // namespace IceClean::Core::Optimizer
