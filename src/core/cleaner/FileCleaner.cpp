#include "FileCleaner.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Cleaner {

Models::CleanResult FileCleaner::Clean(const std::vector<std::wstring>& paths,
                                        std::function<void(const Models::CleanProgress&)> progressCallback,
                                        const std::atomic<bool>* cancelFlag) {
    Models::CleanResult result;
    result.success = true;

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
        // 检查取消标志
        if (cancelFlag && cancelFlag->load()) {
            result.success = false;
            result.errorMessage = L"清理已取消";
            break;
        }

        // 检查白名单
        if (IsWhitelisted(path)) {
            currentItem++;
            continue;
        }

        // 发送清理前进度回调
        if (progressCallback) {
            Models::CleanProgress progress;
            progress.currentItem = currentItem;
            progress.totalItems = totalItems;
            progress.cleanedSize = cleanedSize;
            progress.totalSize = totalSize;
            progress.currentFile = path;
            progress.isRunning = true;
            progress.isCancelled = false;
            progressCallback(progress);
        }

        // 记录删除前的大小
        uint64_t fileSize = 0;
        bool isDir = Utils::FileUtil::IsDirectory(path);
        if (isDir) {
            fileSize = Utils::FileUtil::GetFolderSize(path);
        } else {
            fileSize = Utils::FileUtil::GetFileSize(path);
        }

        bool deleted = false;

        if (isDir) {
            deleted = Utils::FileUtil::DeleteFolder(path);
        } else {
            deleted = Utils::FileUtil::DeleteFilePermanently(path);
        }

        if (deleted) {
            result.cleanedFileCount++;
            cleanedSize += fileSize;
        } else {
            result.failedFileCount++;
            result.failedFiles.push_back(path);
        }

        currentItem++;

        // 每删除一个文件后发送进度回调
        if (progressCallback) {
            Models::CleanProgress progress;
            progress.currentItem = currentItem;
            progress.totalItems = totalItems;
            progress.cleanedSize = cleanedSize;
            progress.totalSize = totalSize;
            progress.currentFile = path;
            progress.isRunning = true;
            progress.isCancelled = false;
            progressCallback(progress);
        }
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
        progress.isCancelled = cancelFlag && cancelFlag->load();
        progressCallback(progress);
    }

    return result;
}

} // namespace IceClean::Core::Cleaner
