#pragma once
#include <string>
#include <vector>
#include <memory>
#include "models/ScanResult.h"

namespace IceClean::Core::Scanner {

class IScanner {
public:
    virtual ~IScanner() = default;

    // Get the scanner name (e.g., "系统临时文件")
    virtual std::wstring GetName() const = 0;

    // Get the scanner description
    virtual std::wstring GetDescription() const = 0;

    // Get the safety rating for this scanner's items
    virtual Models::SafetyRating GetSafetyRating() const = 0;

    // Get the icon identifier
    virtual std::wstring GetIcon() const = 0;

    // Perform the scan
    virtual Models::ScanCategory Scan() = 0;

    // Check if this scanner is available on the current system
    virtual bool IsAvailable() const = 0;
};

} // namespace IceClean::Core::Scanner
