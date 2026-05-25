#include "FileCleaner.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Cleaner {

Models::CleanResult FileCleaner::Clean(const std::vector<std::wstring>& paths,
                                        std::function<void(const Models::CleanProgress&)> progressCallback) {
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
        // 检查白名单
        if (IsWhitelisted(path)) {
            currentItem++;
            continue;
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
            uint64_t fileSize = Utils::FileUtil::IsDirectory(path) ? 0 :
                                Utils::FileUtil::GetFileSize(path);
            // 文件已删除，大小用之前记录的值
            if (Utils::FileUtil::IsDirectory(path)) {
                // 目录大小已删除，用0（之前已计算到totalSize中）
            }
            result.cleanedFileCount++;
            cleanedSize += fileSize;
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

} // namespace IceClean::Core::Cleaner
