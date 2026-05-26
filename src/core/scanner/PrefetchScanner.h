#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class PrefetchScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"预读取文件"; }
    std::wstring GetDescription() const override { return L"扫描Windows预读取缓存文件(.pf)"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Safe; }
    std::wstring GetIcon() const override { return L"prefetch"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;
};

} // namespace IceClean::Core::Scanner
