#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class ThumbnailScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"缩略图缓存"; }
    std::wstring GetDescription() const override { return L"扫描Windows资源管理器的缩略图缓存文件"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Safe; }
    std::wstring GetIcon() const override { return L"thumbnail"; }
    Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                               ScanProgressCallback progressCb = nullptr) override;
    bool IsAvailable() const override;
};

} // namespace IceClean::Core::Scanner
