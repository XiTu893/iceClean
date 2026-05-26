#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class WindowsUpdateScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"Windows更新缓存"; }
    std::wstring GetDescription() const override { return L"扫描Windows Update下载缓存和传递优化文件"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Safe; }
    std::wstring GetIcon() const override { return L"windows_update"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;
};

} // namespace IceClean::Core::Scanner
