#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class WinSxSScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"WinSxS组件存储"; }
    std::wstring GetDescription() const override { return L"扫描WinSxS组件存储大小（仅报告，清理需使用DISM）"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Caution; }
    std::wstring GetIcon() const override { return L"winsxs"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;
};

} // namespace IceClean::Core::Scanner
