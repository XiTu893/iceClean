#pragma once
#include "CleanerBase.h"

namespace IceClean::Core::Cleaner {

class DismCleaner : public CleanerBase {
public:
    std::wstring GetName() const override { return L"DISM深度清理器"; }

    Models::CleanResult Clean(const std::vector<std::wstring>& paths,
                               std::function<void(const Models::CleanProgress&)> progressCallback = nullptr) override;

    // 执行组件清理
    bool StartComponentCleanup(bool resetBase = false,
                               std::function<void(const std::wstring&)> outputCallback = nullptr);

    // 执行 CompactOS 压缩
    bool CompactOS(std::function<void(const std::wstring&)> outputCallback = nullptr);
};

} // namespace IceClean::Core::Cleaner
