#pragma once
#include "IMigrator.h"
#include <cstdint>
#include <vector>
#include <functional>
#include <atomic>

namespace IceClean::Core::Migrator {

class LargeFolderDetector {
public:
    // 构造函数
    // minSizeMB: 最小文件夹大小阈值(MB)，默认500MB
    explicit LargeFolderDetector(uint64_t minSizeMB = 500);

    // 检测C盘上的大文件夹
    std::vector<Models::MigrationItem> Detect(
        std::function<void(const std::wstring&)> progressCallback = nullptr);

    // 取消检测
    void Cancel();

private:
    uint64_t minSizeBytes_;
    std::atomic<bool> cancelled_{false};

    // 需要跳过的系统文件夹名称(小写)
    static const std::vector<std::wstring>& GetSkippedFolderNames();

    // 判断文件夹是否应该跳过
    bool ShouldSkip(const std::wstring& folderName, DWORD attributes) const;

    // 递归扫描目录
    void ScanDirectory(const std::wstring& path,
                       std::vector<Models::MigrationItem>& results,
                       std::function<void(const std::wstring&)>& progressCallback);
};

} // namespace IceClean::Core::Migrator
