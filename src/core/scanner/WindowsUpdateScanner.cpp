#include "WindowsUpdateScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Scanner {

Models::ScanCategory WindowsUpdateScanner::Scan(const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    // 扫描 Windows Update 下载缓存
    std::wstring downloadPath = Utils::Win32Util::ExpandEnvVars(
        L"%SystemRoot%\\SoftwareDistribution\\Download");
    if (Utils::FileUtil::Exists(downloadPath)) {
        ScanDirectory(downloadPath, L"*", true, true, category, stopFlag, progressCb);
    }

    // 扫描传递优化缓存
    std::wstring deliveryPath = Utils::Win32Util::ExpandEnvVars(
        L"%SystemRoot%\\SoftwareDistribution\\DeliveryOptimization");
    if (Utils::FileUtil::Exists(deliveryPath)) {
        ScanDirectory(deliveryPath, L"*", true, true, category, stopFlag, progressCb);
    }

    return category;
}

bool WindowsUpdateScanner::IsAvailable() const {
    // SoftwareDistribution 在所有现代Windows上存在
    std::wstring path = Utils::Win32Util::ExpandEnvVars(
        L"%SystemRoot%\\SoftwareDistribution");
    return Utils::FileUtil::Exists(path);
}

} // namespace IceClean::Core::Scanner
