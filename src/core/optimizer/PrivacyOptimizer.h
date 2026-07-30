#pragma once
#include "models/PrivacyItem.h"
#include <vector>
#include <string>
#include <functional>
#include <atomic>

namespace IceClean::Core::Optimizer {

class PrivacyOptimizer {
public:
    PrivacyOptimizer();

    // 获取所有隐私设置项
    std::vector<Models::PrivacyItem> GetItems() const;

    // 获取预设配置
    std::vector<Models::PrivacyPreset> GetPresets() const;

    // 应用单个设置
    bool ApplyItem(const Models::PrivacyItem& item);

    // 批量应用
    Models::PrivacyResult ApplyItems(const std::vector<Models::PrivacyItem>& items,
                                      std::function<void(int, int)> progressCallback = nullptr,
                                      const std::atomic<bool>* cancelFlag = nullptr);

    // 还原单个设置
    bool RevertItem(const Models::PrivacyItem& item);

    // 扫描当前状态
    void RefreshCurrentValues();

private:
    void InitializeItems();
    bool ApplyRegistryDword(const std::wstring& fullPath, const std::wstring& valueName, DWORD value);
    bool ApplyRegistryString(const std::wstring& fullPath, const std::wstring& valueName, const std::wstring& value);

    std::vector<Models::PrivacyItem> m_items;
};

} // namespace IceClean::Core::Optimizer
