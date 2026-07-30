#pragma once
#include "models/DuplicateFileGroup.h"
#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <atomic>
#include <unordered_map>

namespace IceClean::Core::Analyzer {

// 重复文件查找器
class DuplicateFileFinder {
public:
    // 扫描指定路径查找重复文件
    std::vector<Models::DuplicateFileGroup> Scan(
        const std::wstring& path,
        std::function<void(const Models::DuplicateScanProgress&)> progress = nullptr,
        uint64_t minFileSize = 1024,  // 最小文件大小(默认1KB)
        int maxDepth = -1              // 最大扫描深度(-1=无限制)
    );

    // 扫描多个路径
    std::vector<Models::DuplicateFileGroup> ScanMultiple(
        const std::vector<std::wstring>& paths,
        std::function<void(const Models::DuplicateScanProgress&)> progress = nullptr,
        uint64_t minFileSize = 1024,
        int maxDepth = -1
    );

    // 删除重复文件(保留每组第一个)
    int DeleteDuplicates(
        std::vector<Models::DuplicateFileGroup>& groups,
        const std::vector<int>& groupsToDelete,  // 要删除的组索引
        std::function<void(int, int)> progress = nullptr
    );

    // 移动重复文件到指定目录
    int MoveDuplicates(
        std::vector<Models::DuplicateFileGroup>& groups,
        const std::vector<int>& groupsToMove,
        const std::wstring& targetDir,
        std::function<void(int, int)> progress = nullptr
    );

    // 获取重复文件总浪费空间
    static uint64_t GetTotalWastedSpace(const std::vector<Models::DuplicateFileGroup>& groups);

    // 取消扫描
    void Cancel() { m_cancelled = true; }

private:
    // 计算文件SHA256哈希
    static std::wstring ComputeFileHash(const std::wstring& filePath);

    // 按大小分组(快速筛选)
    void GroupBySize(const std::wstring& path, uint64_t minFileSize, int maxDepth, int currentDepth,
                     std::unordered_map<uint64_t, std::vector<std::wstring>>& sizeGroups,
                     Models::DuplicateScanProgress& progressInfo,
                     std::function<void(const Models::DuplicateScanProgress&)>& progress);

    std::atomic<bool> m_cancelled{false};
};

} // namespace IceClean::Core::Analyzer
