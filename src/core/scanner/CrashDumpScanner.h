#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class CrashDumpScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"崩溃转储文件"; }
    std::wstring GetDescription() const override { return L"扫描系统崩溃转储和小内存转储文件"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Safe; }
    std::wstring GetIcon() const override { return L"crash_dump"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;
};

} // namespace IceClean::Core::Scanner
