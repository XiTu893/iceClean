#include "DismCleaner.h"
#include "utils/DismUtil.h"
#include "utils/Win32Util.h"

namespace IceClean::Core::Cleaner {

Models::CleanResult DismCleaner::Clean(const std::vector<std::wstring>& paths,
                                        std::function<void(const Models::CleanProgress&)> progressCallback) {
    Models::CleanResult result;
    result.success = true;

    // 检查管理员权限
    if (!Utils::Win32Util::IsRunningAsAdmin()) {
        result.success = false;
        result.errorMessage = L"DISM清理需要管理员权限";
        return result;
    }

    int totalItems = static_cast<int>(paths.size());
    int currentItem = 0;

    for (const auto& path : paths) {
        // 发送进度回调
        if (progressCallback) {
            Models::CleanProgress progress;
            progress.currentItem = currentItem;
            progress.totalItems = totalItems;
            progress.currentFile = path;
            progress.isRunning = true;
            progressCallback(progress);
        }

        // 根据路径判断执行哪种 DISM 操作
        if (path.find(L"WinSxS") != std::wstring::npos) {
            // WinSxS 清理 - 执行组件清理
            bool success = StartComponentCleanup(false, [progressCallback](const std::wstring& line) {
                if (progressCallback) {
                    Models::CleanProgress progress;
                    progress.currentFile = line;
                    progress.isRunning = true;
                    progressCallback(progress);
                }
            });

            if (success) {
                result.cleanedFileCount++;
            } else {
                result.failedFileCount++;
                result.failedFiles.push_back(path);
            }
        } else if (path.find(L"CompactOS") != std::wstring::npos) {
            // CompactOS 压缩
            bool success = CompactOS([progressCallback](const std::wstring& line) {
                if (progressCallback) {
                    Models::CleanProgress progress;
                    progress.currentFile = line;
                    progress.isRunning = true;
                    progressCallback(progress);
                }
            });

            if (success) {
                result.cleanedFileCount++;
            } else {
                result.failedFileCount++;
                result.failedFiles.push_back(path);
            }
        }

        currentItem++;
    }

    // 发送完成回调
    if (progressCallback) {
        Models::CleanProgress progress;
        progress.currentItem = totalItems;
        progress.totalItems = totalItems;
        progress.isRunning = false;
        progressCallback(progress);
    }

    return result;
}

bool DismCleaner::StartComponentCleanup(bool resetBase,
                                         std::function<void(const std::wstring&)> outputCallback) {
    return Utils::DismUtil::StartComponentCleanup(resetBase, outputCallback);
}

bool DismCleaner::CompactOS(std::function<void(const std::wstring&)> outputCallback) {
    return Utils::DismUtil::CompactOS(outputCallback);
}

} // namespace IceClean::Core::Cleaner
