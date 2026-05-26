#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class RecycleBinScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"回收站"; }
    std::wstring GetDescription() const override { return L"扫描回收站中的文件大小"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Safe; }
    std::wstring GetIcon() const override { return L"recycle_bin"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;
};

} // namespace IceClean::Core::Scanner
