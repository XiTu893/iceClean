#include "FileTypeAnalyzer.h"
#include "utils/FileUtil.h"
#include "utils/Win32Util.h"
#include "utils/FormatUtil.h"
#include <algorithm>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>

namespace {

std::wstring FormatFileSize(uint64_t bytes) {
    if (bytes >= 1024ULL * 1024 * 1024) {
        double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%.2f GB", gb);
        return buf;
    } else if (bytes >= 1024 * 1024) {
        double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%.2f MB", mb);
        return buf;
    } else if (bytes >= 1024) {
        double kb = static_cast<double>(bytes) / 1024.0;
        wchar_t buf[32] = {};
        swprintf_s(buf, L"%.1f KB", kb);
        return buf;
    }
    return std::to_wstring(bytes) + L" B";
}

} // anonymous namespace

namespace IceClean::Core::Analyzer {

using namespace IceClean::Utils;

FileTypeReport FileTypeAnalyzer::Analyze(const std::wstring& path) {
    FileTypeReport report;
    report.scanPath = path;
    report.systemDrive = Win32Util::GetSystemDrive();

    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &timeT);
    wchar_t timeBuf[64] = {};
    wcsftime(timeBuf, 64, L"%Y-%m-%d %H:%M:%S", &tm);
    report.generatedTime = timeBuf;

    std::map<std::wstring, FileTypeStat> statsMap;
    uint64_t totalSize = 0;
    int totalCount = 0;

    ScanDirectory(path, statsMap, totalSize, totalCount);

    report.totalSize = totalSize;
    report.totalFileCount = totalCount;

    // 转换到 vector 并按大小排序
    for (auto& [ext, stat] : statsMap) {
        auto [category, icon] = GetCategory(ext);
        stat.extension = ext;
        stat.category = category;
        stat.categoryIcon = icon;
        report.stats.push_back(stat);
    }

    std::sort(report.stats.begin(), report.stats.end(),
              [](const FileTypeStat& a, const FileTypeStat& b) {
                  return a.totalSize > b.totalSize;
              });

    return report;
}

void FileTypeAnalyzer::ScanDirectory(const std::wstring& dirPath,
                                       std::map<std::wstring, FileTypeStat>& statsMap,
                                       uint64_t& totalSize, int& totalCount) {
    if (!FileUtil::Exists(dirPath) || !FileUtil::IsDirectory(dirPath)) return;

    std::wstring searchPath = dirPath;
    if (searchPath.back() != L'\\') searchPath += L'\\';
    searchPath += L"*";

    WIN32_FIND_DATAW findData = {};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;

        std::wstring fullPath = dirPath;
        if (fullPath.back() != L'\\') fullPath += L'\\';
        fullPath += findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 跳过隐藏目录和系统目录
            if (findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) {
                // 仍然扫描隐藏目录内容（用户可能想知道所有内容）
            }
            ScanDirectory(fullPath, statsMap, totalSize, totalCount);
        } else {
            ULARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;

            std::wstring ext = GetExtension(findData.cFileName);
            UpdateStat(statsMap, ext, fileSize.QuadPart, fullPath);
            totalSize += fileSize.QuadPart;
            totalCount++;
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

void FileTypeAnalyzer::UpdateStat(std::map<std::wstring, FileTypeStat>& statsMap,
                                    const std::wstring& ext, uint64_t size, const std::wstring& path) {
    auto it = statsMap.find(ext);
    if (it != statsMap.end()) {
        it->second.totalSize += size;
        it->second.fileCount++;
    } else {
        FileTypeStat stat;
        stat.extension = ext;
        stat.totalSize = size;
        stat.fileCount = 1;
        stat.typicalPath = path;
        statsMap[ext] = stat;
    }
}

std::wstring FileTypeAnalyzer::GetExtension(const std::wstring& fileName) {
    size_t dot = fileName.find_last_of(L'.');
    if (dot == std::wstring::npos) return L"(无扩展名)";
    std::wstring ext = fileName.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    return ext;
}

std::pair<std::wstring, std::wstring> FileTypeAnalyzer::GetCategory(const std::wstring& extension) {
    std::wstring lower = extension;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    if (lower == L".jpg" || lower == L".jpeg" || lower == L".png" || lower == L".gif" ||
        lower == L".bmp" || lower == L".webp" || lower == L".svg" || lower == L".ico" ||
        lower == L".tiff" || lower == L".heic" || lower == L".avif") {
        return { L"图片", L"🖼" };
    }
    if (lower == L".mp4" || lower == L".avi" || lower == L".mkv" || lower == L".mov" ||
        lower == L".wmv" || lower == L".flv" || lower == L".webm" || lower == L".m4v" ||
        lower == L".mpg" || lower == L".mpeg") {
        return { L"视频", L"🎬" };
    }
    if (lower == L".mp3" || lower == L".wav" || lower == L".flac" || lower == L".aac" ||
        lower == L".ogg" || lower == L".wma" || lower == L".m4a") {
        return { L"音频", L"🎵" };
    }
    if (lower == L".pdf") {
        return { L"PDF 文档", L"📄" };
    }
    if (lower == L".doc" || lower == L".docx" || lower == L".xls" || lower == L".xlsx" ||
        lower == L".ppt" || lower == L".pptx" || lower == L".csv") {
        return { L"Office 文档", L"📊" };
    }
    if (lower == L".txt" || lower == L".log" || lower == L".md" || lower == L".rtf") {
        return { L"文本文件", L"📝" };
    }
    if (lower == L".zip" || lower == L".rar" || lower == L".7z" || lower == L".tar" ||
        lower == L".gz" || lower == L".bz2" || lower == L".xz" || lower == L".iso") {
        return { L"压缩/镜像", L"🗜" };
    }
    if (lower == L".exe" || lower == L".msi" || lower == L".bat" || lower == L".cmd" ||
        lower == L".ps1") {
        return { L"可执行/脚本", L"⚡" };
    }
    if (lower == L".dll" || lower == L".sys" || lower == L".drv" || lower == L".ocx") {
        return { L"系统文件", L"⚙" };
    }
    if (lower == L".tmp" || lower == L".temp" || lower == L".etl" || lower == L".dmp") {
        return { L"临时/日志", L"🗑" };
    }
    if (lower == L".db" || lower == L".sqlite" || lower == L".sql" || lower == L".mdb") {
        return { L"数据库", L"🗄" };
    }
    if (lower == L".git" || lower == L".svn" || lower == L".hg") {
        return { L"版本控制", L"🔧" };
    }
    if (lower == L".json" || lower == L".xml" || lower == L".yaml" || lower == L".yml" ||
        lower == L".toml" || lower == L".ini" || lower == L".cfg" || lower == L".conf") {
        return { L"配置文件", L"⚙" };
    }

    return { L"其他", L"📁" };
}

bool FileTypeAnalyzer::ExportHtml(const FileTypeReport& report, const std::wstring& outputPath) {
    std::wofstream ofs(outputPath);
    if (!ofs.is_open()) return false;

    ofs << L"<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
        << L"<title>IceClean 文件类型分析报告</title>"
        << L"<style>"
        << L"body{font-family:'Microsoft YaHei',sans-serif;background:#f5f5f5;margin:20px;color:#333}"
        << L".header{background:#2196F3;color:#fff;padding:20px;border-radius:8px;margin-bottom:20px}"
        << L".stat-card{background:#fff;border-radius:8px;padding:15px;margin-bottom:10px;box-shadow:0 1px 3px rgba(0,0,0,0.1)}"
        << L".stat-row{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #eee}"
        << L".progress-bar{height:20px;background:#e0e0e0;border-radius:10px;overflow:hidden;margin:4px 0}"
        << L".progress-fill{height:100%;background:#4CAF50;border-radius:10px}"
        << L".summary{font-size:18px;font-weight:bold;margin:10px 0}"
        << L"</style></head><body>";

    double totalMB = static_cast<double>(report.totalSize) / (1024.0 * 1024.0);

    ofs << L"<div class=\"header\">";
    ofs << L"<h1>IceClean 文件类型分析报告</h1>";
    ofs << L"<p>扫描路径: " << report.scanPath << L"</p>";
    ofs << L"<p>生成时间: " << report.generatedTime << L"</p>";
    ofs << L"</div>";

    ofs << L"<div class=\"stat-card\">";
    ofs << L"<div class=\"summary\">";
    ofs << L"总大小: " << FormatFileSize(report.totalSize);
    ofs << L" | 总文件数: " << report.totalFileCount;
    ofs << L"</div></div>";

    for (const auto& stat : report.stats) {
        double percent = report.totalSize > 0
            ? (static_cast<double>(stat.totalSize) / report.totalSize) * 100.0
            : 0.0;
        double statMB = static_cast<double>(stat.totalSize) / (1024.0 * 1024.0);
        int barWidth = static_cast<int>(percent);

        ofs << L"<div class=\"stat-card\">";
        ofs << L"<div class=\"stat-row\">";
        ofs << L"<span>" << stat.categoryIcon << L" <strong>" << stat.extension << L"</strong>";
        ofs << L" (" << stat.category << L")</span>";
        ofs << L"<span>" << FormatFileSize(stat.totalSize);
        ofs << L" (" << stat.fileCount << L" 个文件, " << static_cast<int>(percent) << L"%)</span>";
        ofs << L"</div>";
        ofs << L"<div class=\"progress-bar\"><div class=\"progress-fill\" style=\"width:" << barWidth << L"%\"></div></div>";
        ofs << L"<div style=\"font-size:12px;color:#999;\">典型: " << stat.typicalPath << L"</div>";
        ofs << L"</div>";
    }

    ofs << L"</body></html>";
    ofs.close();
    return true;
}

bool FileTypeAnalyzer::ExportTxt(const FileTypeReport& report, const std::wstring& outputPath) {
    std::wofstream ofs(outputPath);
    if (!ofs.is_open()) return false;

    ofs << L"============================================" << std::endl;
    ofs << L"  IceClean 文件类型分析报告" << std::endl;
    ofs << L"============================================" << std::endl;
    ofs << L"扫描路径: " << report.scanPath << std::endl;
    ofs << L"生成时间: " << report.generatedTime << std::endl;
    ofs << L"总大小: " << FormatFileSize(report.totalSize) << std::endl;
    ofs << L"总文件数: " << report.totalFileCount << std::endl;
    ofs << L"--------------------------------------------" << std::endl;
    ofs << L"  扩展名       | 分类          | 大小         | 文件数" << std::endl;
    ofs << L"--------------------------------------------" << std::endl;

    for (const auto& stat : report.stats) {
        wchar_t buf[128] = {};
        swprintf_s(buf, L"  %-12s | %-12s | %-12s | %d",
                   stat.extension.c_str(),
                   stat.category.c_str(),
                   FormatFileSize(stat.totalSize).c_str(),
                   stat.fileCount);
        ofs << buf << std::endl;
    }

    ofs << L"--------------------------------------------" << std::endl;
    ofs.close();
    return true;
}

} // namespace IceClean::Core::Analyzer
