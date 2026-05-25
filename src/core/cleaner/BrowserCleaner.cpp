#include "BrowserCleaner.h"
#include "utils/FileUtil.h"
#include "utils/Win32Util.h"

namespace IceClean::Core::Cleaner {

Models::CleanResult BrowserCleaner::Clean(const std::vector<std::wstring>& paths,
                                           std::function<void(const Models::CleanProgress&)> progressCallback) {
    Models::CleanResult result;
    result.success = true;

    // 检查浏览器是否正在运行
    if (IsAnyBrowserRunning()) {
        result.success = false;
        result.errorMessage = L"浏览器正在运行，请关闭浏览器后重试";
        return result;
    }

    int totalItems = static_cast<int>(paths.size());
    uint64_t totalSize = 0;

    // 预计算总大小
    for (const auto& path : paths) {
        if (Utils::FileUtil::IsDirectory(path)) {
            totalSize += Utils::FileUtil::GetFolderSize(path);
        } else {
            totalSize += Utils::FileUtil::GetFileSize(path);
        }
    }

    uint64_t cleanedSize = 0;
    int currentItem = 0;

    for (const auto& path : paths) {
        // 检查白名单
        if (IsWhitelisted(path)) {
            currentItem++;
            continue;
        }

        // 再次检查浏览器是否正在运行（可能在清理过程中被启动）
        if (IsAnyBrowserRunning()) {
            result.success = false;
            result.errorMessage = L"检测到浏览器已启动，清理已中止";
            break;
        }

        // 发送进度回调
        if (progressCallback) {
            Models::CleanProgress progress;
            progress.currentItem = currentItem;
            progress.totalItems = totalItems;
            progress.cleanedSize = cleanedSize;
            progress.totalSize = totalSize;
            progress.currentFile = path;
            progress.isRunning = true;
            progressCallback(progress);
        }

        bool deleted = false;

        if (Utils::FileUtil::IsDirectory(path)) {
            deleted = Utils::FileUtil::DeleteFolder(path);
        } else {
            deleted = Utils::FileUtil::DeleteFilePermanently(path);
        }

        if (deleted) {
            result.cleanedFileCount++;
        } else {
            result.failedFileCount++;
            result.failedFiles.push_back(path);
        }

        currentItem++;
    }

    result.totalCleanedSize = cleanedSize;

    // 发送完成回调
    if (progressCallback) {
        Models::CleanProgress progress;
        progress.currentItem = totalItems;
        progress.totalItems = totalItems;
        progress.cleanedSize = cleanedSize;
        progress.totalSize = totalSize;
        progress.isRunning = false;
        progressCallback(progress);
    }

    return result;
}

bool BrowserCleaner::IsAnyBrowserRunning() const {
    return Utils::Win32Util::IsProcessRunning(L"chrome.exe") ||
           Utils::Win32Util::IsProcessRunning(L"msedge.exe") ||
           Utils::Win32Util::IsProcessRunning(L"firefox.exe");
}

} // namespace IceClean::Core::Cleaner
