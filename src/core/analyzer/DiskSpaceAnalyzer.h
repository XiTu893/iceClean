#pragma once
#include "models/DiskNode.h"
#include <functional>
#include <atomic>
#include <string>
#include <cstdint>

namespace IceClean::Core::Analyzer {

// 扫描进度信息
struct ScanProgress {
    std::wstring currentPath;     // 当前正在扫描的路径
    uint64_t scannedDirs = 0;     // 已扫描目录数
    uint64_t scannedFiles = 0;    // 已扫描文件数
    bool isRunning = false;
    bool isCancelled = false;
};

class DiskSpaceAnalyzer {
public:
    DiskSpaceAnalyzer();
    ~DiskSpaceAnalyzer() = default;

    // 扫描指定路径，构建磁盘空间树
    std::shared_ptr<Models::DiskNode> Scan(const std::wstring& path,
                                            std::function<void(const ScanProgress&)> progressCallback = nullptr);

    // 取消扫描
    void Cancel();

    // 获取上次扫描结果
    std::shared_ptr<Models::DiskNode> GetResult() const;

private:
    std::atomic<bool> cancelled_{false};
    std::shared_ptr<Models::DiskNode> result_;

    // 需要跳过的系统保护目录名称(小写)
    static const std::vector<std::wstring>& GetProtectedFolderNames();

    // 判断目录是否应该跳过
    bool ShouldSkip(const std::wstring& folderName, DWORD attributes) const;

    // 递归扫描目录
    void ScanDirectory(const std::wstring& path,
                       Models::DiskNode& parentNode,
                       std::function<void(const ScanProgress&)>& progressCallback,
                       ScanProgress& progress);
};

} // namespace IceClean::Core::Analyzer
