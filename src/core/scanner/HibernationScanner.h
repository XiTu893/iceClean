#pragma once
#include "ScannerBase.h"

namespace IceClean::Core::Scanner {

class HibernationScanner : public ScannerBase {
public:
    std::wstring GetName() const override { return L"休眠文件"; }
    std::wstring GetDescription() const override { return L"扫描系统休眠文件(hiberfil.sys)"; }
    Models::SafetyRating GetSafetyRating() const override { return Models::SafetyRating::Caution; }
    std::wstring GetIcon() const override { return L"hibernation"; }
    Models::ScanCategory Scan() override;
    bool IsAvailable() const override;
};

} // namespace IceClean::Core::Scanner
