#include "WinSxSScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Scanner {

Models::ScanCategory WinSxSScanner::Scan(const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    // 谨慎项默认不选中
    category.selected = false;

    std::wstring winsxsPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\WinSxS");

    if (Utils::FileUtil::Exists(winsxsPath)) {
        if (stopFlag && stopFlag->load()) return category;

        // 使用 GetCompressedFileSize 获取 WinSxS 文件夹的压缩大小
        // 注意：WinSxS 包含大量硬链接，直接遍历会重复计算
        // 更准确的方式是使用 DISM /Online /Cleanup-Image /AnalyzeComponentStore
        // 这里我们使用文件夹大小作为估算值
        uint64_t winsxsSize = Utils::FileUtil::GetFolderSize(winsxsPath);

        if (winsxsSize > 0) {
            Models::ScanFileItem item;
            item.path = winsxsPath;
            item.size = winsxsSize;
            item.selected = false;  // 谨慎项默认不选中

            SYSTEMTIME st;
            GetSystemTime(&st);
            SystemTimeToFileTime(&st, &item.lastWriteTime);

            category.items.push_back(item);
            category.totalSize = winsxsSize;
        }
    }

    return category;
}

bool WinSxSScanner::IsAvailable() const {
    std::wstring winsxsPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\WinSxS");
    return Utils::FileUtil::Exists(winsxsPath);
}

} // namespace IceClean::Core::Scanner
