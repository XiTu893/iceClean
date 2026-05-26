#include "RecycleBinCleaner.h"
#include "utils/ShellUtil.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Cleaner {

Models::CleanResult RecycleBinCleaner::Clean(const std::vector<std::wstring>& paths,
                                              std::function<void(const Models::CleanProgress&)> progressCallback,
                                              const std::atomic<bool>* cancelFlag) {
    Models::CleanResult result;

    // 获取回收站当前大小（用于报告清理了多少）
    uint64_t sizeBefore = Utils::ShellUtil::GetRecycleBinSize();

    // 发送进度回调
    if (progressCallback) {
        Models::CleanProgress progress;
        progress.currentItem = 0;
        progress.totalItems = 1;
        progress.cleanedSize = 0;
        progress.totalSize = sizeBefore;
        progress.currentFile = L"回收站";
        progress.isRunning = true;
        progressCallback(progress);
    }

    // 使用 Shell API 清空回收站
    bool success = Utils::ShellUtil::EmptyRecycleBin();

    if (success) {
        result.success = true;
        result.totalCleanedSize = sizeBefore;
        result.cleanedFileCount = 1;
    } else {
        result.success = false;
        result.errorMessage = L"清空回收站失败";
        result.failedFileCount = 1;
    }

    // 发送完成回调
    if (progressCallback) {
        Models::CleanProgress progress;
        progress.currentItem = 1;
        progress.totalItems = 1;
        progress.cleanedSize = success ? sizeBefore : 0;
        progress.totalSize = sizeBefore;
        progress.isRunning = false;
        progressCallback(progress);
    }

    return result;
}

} // namespace IceClean::Core::Cleaner
