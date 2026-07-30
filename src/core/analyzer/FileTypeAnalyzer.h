#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace IceClean::Core::Analyzer {

struct FileTypeStat {
    std::wstring extension;
    std::wstring category;
    std::wstring categoryIcon;
    uint64_t totalSize = 0;
    int fileCount = 0;
    std::wstring typicalPath;
    bool canClean = false;
};

struct FileTypeReport {
    std::wstring scanPath;
    std::vector<FileTypeStat> stats;
    uint64_t totalSize = 0;
    int totalFileCount = 0;
    std::wstring generatedTime;
    std::wstring systemDrive;
};

class FileTypeAnalyzer {
public:
    // 扫描指定路径，按文件类型分类统计
    FileTypeReport Analyze(const std::wstring& path);

    // 获取分类名称和图标
    static std::pair<std::wstring, std::wstring> GetCategory(const std::wstring& extension);

    // 导出 HTML 报告
    bool ExportHtml(const FileTypeReport& report, const std::wstring& outputPath);

    // 导出 TXT 报告
    bool ExportTxt(const FileTypeReport& report, const std::wstring& outputPath);

private:
    // 扫描目录递归
    void ScanDirectory(const std::wstring& dirPath,
                        std::map<std::wstring, FileTypeStat>& statsMap,
                        uint64_t& totalSize, int& totalCount);

    // 更新统计
    void UpdateStat(std::map<std::wstring, FileTypeStat>& statsMap,
                     const std::wstring& ext, uint64_t size, const std::wstring& path);

    // 获取扩展名
    static std::wstring GetExtension(const std::wstring& fileName);
};

} // namespace IceClean::Core::Analyzer
