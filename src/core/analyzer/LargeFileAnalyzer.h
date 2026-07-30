#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdint>
#include <atomic>

namespace IceClean::Core::Analyzer {

// 大文件信息
struct LargeFileInfo {
    std::wstring filePath;       // 文件完整路径
    std::wstring fileName;       // 文件名
    uint64_t fileSize = 0;      // 文件大小
    std::wstring fileType;       // 文件类型(扩展名)
    std::wstring lastModified;   // 最后修改时间
    int accessCount = 0;        // 访问次数(估算)
    bool isSystemFile = false;   // 是否系统文件
    bool canMove = true;         // 是否可迁移
};

// 大文件扫描进度
struct LargeFileScanProgress {
    int scannedFiles = 0;
    int totalFiles = 0;
    std::wstring currentPath;
};

// 大文件分析器
class LargeFileAnalyzer {
public:
    // 扫描指定路径下的大文件
    std::vector<LargeFileInfo> ScanLargeFiles(
        const std::wstring& path,
        uint64_t minSize = 100 * 1024 * 1024,  // 默认最小100MB
        std::function<void(const LargeFileScanProgress&)> progress = nullptr
    );

    // 按文件类型分类统计
    std::vector<std::pair<std::wstring, uint64_t>> GetFileTypeStats(
        const std::vector<LargeFileInfo>& files
    );

    // 获取指定类型的文件列表
    std::vector<LargeFileInfo> GetFilesByType(
        const std::vector<LargeFileInfo>& files,
        const std::wstring& fileType
    );

    // 获取最近未访问的大文件
    std::vector<LargeFileInfo> GetInactiveLargeFiles(
        const std::vector<LargeFileInfo>& files,
        int daysThreshold = 90  // 超过90天未访问
    );

    // 取消扫描
    void Cancel() { m_cancelled = true; }

private:
    bool IsSystemFile(const std::wstring& path) const;
    std::wstring GetFileTypeLabel(const std::wstring& ext) const;

    std::atomic<bool> m_cancelled{false};
};

} // namespace IceClean::Core::Analyzer
