#include "PrefetchScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Scanner {

Models::ScanCategory PrefetchScanner::Scan(const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    // 扫描 C:\Windows\Prefetch\*.pf
    std::wstring prefetchPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\Prefetch");
    if (Utils::FileUtil::Exists(prefetchPath)) {
        ScanDirectory(prefetchPath, L"*.pf", false, false, category, stopFlag, progressCb);
    }

    return category;
}

bool PrefetchScanner::IsAvailable() const {
    // Prefetch 可能被禁用（SuperFetch/SysMain 服务关闭时目录可能不存在）
    std::wstring prefetchPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\Prefetch");
    return Utils::FileUtil::Exists(prefetchPath);
}

} // namespace IceClean::Core::Scanner
