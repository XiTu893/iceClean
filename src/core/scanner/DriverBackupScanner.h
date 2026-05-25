#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class DriverBackupScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"驱动备份文件"; }
    std::wstring GetDescription() const override { return L"扫描驱动存储中的旧驱动备份文件"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Safe; }
    std::wstring GetIcon() const override { return L"driver"; }
    Models::ScanCategory Scan() override;
    bool IsAvailable() const override;
};

} // namespace IceClean::Core::Scanner
