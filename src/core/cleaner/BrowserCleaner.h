#pragma once
#include "CleanerBase.h"
#include <atomic>

namespace IceClean::Core::Cleaner {

class BrowserCleaner : public CleanerBase {
public:
    std::wstring GetName() const override { return L"浏览器缓存清理器"; }

    Models::CleanResult Clean(const std::vector<std::wstring>& paths,
                               std::function<void(const Models::CleanProgress&)> progressCallback = nullptr,
                               const std::atomic<bool>* cancelFlag = nullptr) override;

private:
    // 检查浏览器是否正在运行
    bool IsAnyBrowserRunning() const;
};

} // namespace IceClean::Core::Cleaner
