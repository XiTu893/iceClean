#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class LogScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"系统日志文件"; }
    std::wstring GetDescription() const override { return L"扫描Windows系统日志和事件日志文件"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Safe; }
    std::wstring GetIcon() const override { return L"log"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;
};

} // namespace IceClean::Core::Scanner
