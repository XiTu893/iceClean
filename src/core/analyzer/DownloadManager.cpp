#include "DownloadManager.h"
#include "utils/FileUtil.h"
#include "utils/Win32Util.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <map>

namespace IceClean::Core::Analyzer {

using namespace IceClean::Utils;

std::vector<DownloadItem> DownloadManager::ScanDownloads(const std::wstring& downloadPath) {
    std::vector<DownloadItem> items;

    if (!FileUtil::Exists(downloadPath)) return items;

    std::wstring searchPath = downloadPath;
    if (searchPath.back() != L'\\') searchPath += L'\\';
    searchPath += L"*";

    WIN32_FIND_DATAW findData = {};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return items;

    do {
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;

        std::wstring fullPath = downloadPath;
        if (fullPath.back() != L'\\') fullPath += L'\\';
        fullPath += findData.cFileName;

        // 跳过目录
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        // 跳过隐藏文件
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) continue;

        ULARGE_INTEGER fileSize;
        fileSize.LowPart = findData.nFileSizeLow;
        fileSize.HighPart = findData.nFileSizeHigh;

        DownloadItem item;
        item.path = fullPath;
        item.name = findData.cFileName;
        item.size = fileSize.QuadPart;
        item.lastAccessTime = findData.ftLastAccessTime;
        item.lastWriteTime = findData.ftLastWriteTime;

        // 获取扩展名
        size_t dot = item.name.find_last_of(L'.');
        if (dot != std::wstring::npos) {
            item.extension = item.name.substr(dot);
            std::transform(item.extension.begin(), item.extension.end(), item.extension.begin(), ::towlower);
        }

        item.category = GetFileCategory(item.extension);
        item.isInstaller = IsInstallerFile(item.extension);

        items.push_back(item);
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);

    // 按大小降序排列
    std::sort(items.begin(), items.end(),
              [](const DownloadItem& a, const DownloadItem& b) {
                  return a.size > b.size;
              });

    return items;
}

std::vector<DownloadItem> DownloadManager::GetInstallers(const std::vector<DownloadItem>& items) {
    std::vector<DownloadItem> installers;
    for (const auto& item : items) {
        if (item.isInstaller) {
            installers.push_back(item);
        }
    }
    return installers;
}

std::vector<DownloadItem> DownloadManager::GetInactiveFiles(const std::vector<DownloadItem>& items, int days) {
    std::vector<DownloadItem> inactive;

    // 获取当前时间
    FILETIME now;
    GetSystemTimeAsFileTime(&now);

    // days 天数对应的 100-ns 间隔数
    ULARGE_INTEGER nowLI;
    nowLI.LowPart = now.dwLowDateTime;
    nowLI.HighPart = now.dwHighDateTime;
    uint64_t threshold = static_cast<uint64_t>(days) * 24ULL * 60 * 60 * 10000000ULL;

    for (const auto& item : items) {
        ULARGE_INTEGER accessLI;
        accessLI.LowPart = item.lastAccessTime.dwLowDateTime;
        accessLI.HighPart = item.lastAccessTime.dwHighDateTime;

        if (nowLI.QuadPart > accessLI.QuadPart + threshold) {
            auto oldItem = item;
            oldItem.isOld = true;
            inactive.push_back(oldItem);
        } else {
            // 也检查最后写入时间
            ULARGE_INTEGER writeLI;
            writeLI.LowPart = item.lastWriteTime.dwLowDateTime;
            writeLI.HighPart = item.lastWriteTime.dwHighDateTime;

            if (nowLI.QuadPart > writeLI.QuadPart + threshold) {
                auto oldItem = item;
                oldItem.isOld = true;
                inactive.push_back(oldItem);
            }
        }
    }

    return inactive;
}

std::vector<std::pair<std::wstring, std::vector<DownloadItem>>> DownloadManager::GroupByCategory(
    const std::vector<DownloadItem>& items)
{
    std::map<std::wstring, std::vector<DownloadItem>> groups;
    for (const auto& item : items) {
        groups[item.category].push_back(item);
    }

    std::vector<std::pair<std::wstring, std::vector<DownloadItem>>> result;
    for (auto& [cat, catItems] : groups) {
        result.emplace_back(cat, std::move(catItems));
    }
    return result;
}

DownloadCleanResult DownloadManager::CleanItems(
    const std::vector<DownloadItem>& items,
    std::function<void(int, int)> progressCallback)
{
    DownloadCleanResult result;
    int total = static_cast<int>(items.size());

    for (int i = 0; i < total; ++i) {
        if (progressCallback) progressCallback(i, total);

        if (!FileUtil::Exists(items[i].path)) {
            continue;
        }

        if (IsFileInUse(items[i].path)) {
            result.failedItems.push_back(items[i].name);
            continue;
        }

        if (FileUtil::DeleteFilePermanently(items[i].path)) {
            result.cleanedCount++;
            result.freedBytes += items[i].size;
        } else {
            result.failedItems.push_back(items[i].name);
        }
    }

    return result;
}

DownloadCleanResult DownloadManager::MoveItems(
    const std::vector<DownloadItem>& items,
    const std::wstring& targetPath,
    std::function<void(int, int)> progressCallback)
{
    DownloadCleanResult result;

    // 确保目标目录存在
    if (!FileUtil::Exists(targetPath)) {
        FileUtil::CreateDirectoryRecursive(targetPath);
    }

    int total = static_cast<int>(items.size());
    for (int i = 0; i < total; ++i) {
        if (progressCallback) progressCallback(i, total);

        if (!FileUtil::Exists(items[i].path)) continue;
        if (IsFileInUse(items[i].path)) {
            result.failedItems.push_back(items[i].name);
            continue;
        }

        std::wstring dest = targetPath + L"\\" + items[i].name;
        if (MoveFileExW(items[i].path.c_str(), dest.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING)) {
            result.movedCount++;
            result.freedBytes += items[i].size;
        } else {
            result.failedItems.push_back(items[i].name);
        }
    }

    return result;
}

bool DownloadManager::IsFileInUse(const std::wstring& path) const {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
        return false;
    }
    return GetLastError() == ERROR_SHARING_VIOLATION;
}

std::wstring DownloadManager::GetFileCategory(const std::wstring& extension) {
    std::wstring lower = extension;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    if (IsInstallerFile(lower)) return L"安装包";
    if (lower == L".zip" || lower == L".rar" || lower == L".7z" || lower == L".tar" ||
        lower == L".gz") return L"压缩包";
    if (lower == L".pdf" || lower == L".doc" || lower == L".docx" || lower == L".xls" ||
        lower == L".xlsx" || lower == L".ppt" || lower == L".pptx") return L"文档";
    if (lower == L".jpg" || lower == L".jpeg" || lower == L".png" || lower == L".gif" ||
        lower == L".bmp" || lower == L".webp") return L"图片";
    if (lower == L".mp4" || lower == L".avi" || lower == L".mkv" || lower == L".mov") return L"视频";
    if (lower == L".mp3" || lower == L".wav" || lower == L".flac" || lower == L".aac") return L"音频";
    if (lower == L".torrent") return L"种子";
    if (lower == L".txt" || lower == L".md" || lower == L".log") return L"文本";
    return L"其他";
}

bool DownloadManager::IsInstallerFile(const std::wstring& extension) {
    std::wstring lower = extension;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    return lower == L".exe" || lower == L".msi" || lower == L".iso" ||
           lower == L".appx" || lower == L".appxbundle" ||
           lower == L".msix" || lower == L".msixbundle";
}

} // namespace IceClean::Core::Analyzer
