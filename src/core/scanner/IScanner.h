#pragma once
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include "models/ScanResult.h"

namespace IceClean::Core::Scanner {

// 扫描进度回调类型：参数为已扫描文件数
using ScanProgressCallback = std::function<void(int filesScanned)>;

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
    // stopFlag: 指向停止标志，若非空且值为true则应尽快停止扫描
    // progressCb: 进度回调，每扫描若干文件后调用
    virtual Models::ScanCategory Scan(const std::atomic<bool>* stopFlag = nullptr,
                                       ScanProgressCallback progressCb = nullptr) = 0;

    // Check if this scanner is available on the current system
    virtual bool IsAvailable() const = 0;
};

} // namespace IceClean::Core::Scanner
