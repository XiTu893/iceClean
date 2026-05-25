#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class BrowserCacheScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"浏览器缓存"; }
    std::wstring GetDescription() const override { return L"扫描Chrome、Edge、Firefox等浏览器的缓存文件"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Safe; }
    std::wstring GetIcon() const override { return L"browser"; }
    Models::ScanCategory Scan() override;
    bool IsAvailable() const override;

private:
    // 扫描单个浏览器的缓存目录
    void ScanBrowserCache(const std::wstring& cachePath, Models::ScanCategory& category);
    // 检查浏览器是否正在运行
    bool IsBrowserRunning(const std::wstring& browserExeName) const;
};

} // namespace IceClean::Core::Scanner
