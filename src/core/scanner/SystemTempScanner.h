#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class SystemTempScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"系统临时文件"; }
    std::wstring GetDescription() const override { return L"扫描用户临时文件夹和系统临时文件夹中的可清理文件"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Safe; }
    std::wstring GetIcon() const override { return L"temp"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;
};

} // namespace IceClean::Core::Scanner
