#include "LargeFileAnalyzer.h"
#include <filesystem>
#include <algorithm>
#include <unordered_map>
#include <chrono>

namespace IceClean::Core::Analyzer {

std::vector<LargeFileInfo> LargeFileAnalyzer::ScanLargeFiles(
    const std::wstring& path,
    uint64_t minSize,
    std::function<void(const LargeFileScanProgress&)> progress)
{
    m_cancelled = false;
    std::vector<LargeFileInfo> results;
    LargeFileScanProgress progressInfo;

    namespace fs = std::filesystem;

    try {
        for (const auto& entry : fs::recursive_directory_iterator(path,
                    fs::directory_options::skip_permission_denied)) {
            if (m_cancelled) break;

            if (!entry.is_regular_file()) continue;

            auto fileSize = entry.file_size();
            progressInfo.scannedFiles++;

            if (fileSize < minSize) continue;

            LargeFileInfo info;
            info.filePath = entry.path().wstring();
            info.fileName = entry.path().filename().wstring();
            info.fileSize = fileSize;
            info.fileType = entry.path().extension().wstring();

            // 转换扩展名为小写
            std::transform(info.fileType.begin(), info.fileType.end(),
                           info.fileType.begin(), ::towlower);

            // 最后修改时间
            auto ftime = fs::last_write_time(entry.path());
            auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            auto time_t_val = std::chrono::system_clock::to_time_t(sctp);
            struct tm tm_val;
            localtime_s(&tm_val, &time_t_val);
            wchar_t timeBuf[32] = {};
            wcsftime(timeBuf, 32, L"%Y-%m-%d", &tm_val);
            info.lastModified = timeBuf;

            info.isSystemFile = IsSystemFile(info.filePath);
            info.canMove = !info.isSystemFile;

            results.push_back(info);

            if (progress && progressInfo.scannedFiles % 200 == 0) {
                progressInfo.currentPath = entry.path().wstring();
                progress(progressInfo);
            }
        }
    } catch (...) {}

    // 按文件大小降序排序
    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.fileSize > b.fileSize;
    });

    return results;
}

std::vector<std::pair<std::wstring, uint64_t>> LargeFileAnalyzer::GetFileTypeStats(
    const std::vector<LargeFileInfo>& files)
{
    std::unordered_map<std::wstring, uint64_t> typeStats;

    for (const auto& file : files) {
        auto label = GetFileTypeLabel(file.fileType);
        typeStats[label] += file.fileSize;
    }

    std::vector<std::pair<std::wstring, uint64_t>> sortedStats(typeStats.begin(), typeStats.end());
    std::sort(sortedStats.begin(), sortedStats.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    return sortedStats;
}

std::vector<LargeFileInfo> LargeFileAnalyzer::GetFilesByType(
    const std::vector<LargeFileInfo>& files,
    const std::wstring& fileType)
{
    std::vector<LargeFileInfo> result;
    auto targetLabel = GetFileTypeLabel(fileType);

    for (const auto& file : files) {
        if (GetFileTypeLabel(file.fileType) == targetLabel) {
            result.push_back(file);
        }
    }
    return result;
}

std::vector<LargeFileInfo> LargeFileAnalyzer::GetInactiveLargeFiles(
    const std::vector<LargeFileInfo>& files,
    int daysThreshold)
{
    std::vector<LargeFileInfo> result;
    auto now = std::chrono::system_clock::now();

    for (const auto& file : files) {
        // 简化：基于最后修改时间判断
        // 解析日期
        int year = 0, month = 0, day = 0;
        if (swscanf_s(file.lastModified.c_str(), L"%d-%d-%d", &year, &month, &day) == 3) {
            struct tm tm_val = {};
            tm_val.tm_year = year - 1900;
            tm_val.tm_mon = month - 1;
            tm_val.tm_mday = day;
            auto fileTime = std::chrono::system_clock::from_time_t(mktime(&tm_val));
            auto daysSince = std::chrono::duration_cast<std::chrono::hours>(
                now - fileTime).count() / 24;

            if (daysSince > daysThreshold) {
                result.push_back(file);
            }
        }
    }

    return result;
}

bool LargeFileAnalyzer::IsSystemFile(const std::wstring& path) const {
    std::wstring lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    // 系统关键目录
    std::vector<std::wstring> systemPaths = {
        L"\\windows\\",
        L"\\program files\\",
        L"\\program files (x86)\\",
        L"\\programdata\\",
    };

    for (const auto& sysPath : systemPaths) {
        if (lower.find(sysPath) != std::wstring::npos) {
            return true;
        }
    }

    return false;
}

std::wstring LargeFileAnalyzer::GetFileTypeLabel(const std::wstring& ext) const {
    if (ext.empty()) return L"其他";

    // 视频文件
    if (ext == L".mp4" || ext == L".mkv" || ext == L".avi" || ext == L".mov" ||
        ext == L".wmv" || ext == L".flv" || ext == L".webm" || ext == L".m4v") {
        return L"视频";
    }
    // 音频文件
    if (ext == L".mp3" || ext == L".flac" || ext == L".wav" || ext == L".aac" ||
        ext == L".ogg" || ext == L".wma" || ext == L".m4a") {
        return L"音频";
    }
    // 图片文件
    if (ext == L".jpg" || ext == L".jpeg" || ext == L".png" || ext == L".gif" ||
        ext == L".bmp" || ext == L".webp" || ext == L".tiff" || ext == L".svg") {
        return L"图片";
    }
    // 压缩文件
    if (ext == L".zip" || ext == L".rar" || ext == L".7z" || ext == L".tar" ||
        ext == L".gz" || ext == L".bz2") {
        return L"压缩包";
    }
    // 虚拟磁盘/镜像
    if (ext == L".iso" || ext == L".vhd" || ext == L".vhdx" || ext == L".vmdk") {
        return L"磁盘镜像";
    }
    // 数据库文件
    if (ext == L".db" || ext == L".sqlite" || ext == L".mdb" || ext == L".mdf") {
        return L"数据库";
    }
    // 游戏相关
    if (ext == L".pak" || ext == L".bsp" || ext == L".vpk" || ext == L".cache") {
        return L"游戏数据";
    }
    // 日志文件
    if (ext == L".log" || ext == L".evtx") {
        return L"日志文件";
    }
    // 可执行文件
    if (ext == L".exe" || ext == L".dll" || ext == L".sys" || ext == L".msi") {
        return L"程序文件";
    }

    return L"其他";
}

} // namespace IceClean::Core::Analyzer
