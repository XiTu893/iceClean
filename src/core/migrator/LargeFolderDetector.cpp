#include "LargeFolderDetector.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <algorithm>
#include <cctype>

namespace IceClean::Core::Migrator {

LargeFolderDetector::LargeFolderDetector(uint64_t minSizeMB)
    : minSizeBytes_(minSizeMB * 1024 * 1024) {}

const std::vector<std::wstring>& LargeFolderDetector::GetSkippedFolderNames() {
    static const std::vector<std::wstring> names = {
        L"windows",
        L"winnt",
        L"program files",
        L"program files (x86)",
        L"programdata",
        L"system volume information",
        L"$recycle.bin",
        L"recovery",
        L"perflogs",
        L"intel",
        L"amd",
        L"nvidia",
    };
    return names;
}

bool LargeFolderDetector::ShouldSkip(const std::wstring& folderName, DWORD attributes) const {
    // 跳过隐藏和系统文件夹
    if (attributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) {
        return true;
    }

    // 跳过联接链接
    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        return true;
    }

    // 检查是否在跳过列表中
    std::wstring lowerName = folderName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), towlower);

    for (const auto& skip : GetSkippedFolderNames()) {
        if (lowerName == skip) return true;
    }

    return false;
}

void LargeFolderDetector::ScanDirectory(const std::wstring& path,
                                          std::vector<Models::MigrationItem>& results,
                                          std::function<void(const std::wstring&)>& progressCallback) {
    if (cancelled_) return;

    std::wstring searchPath = path;
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath += L'\\';
    }
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (cancelled_) break;

        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;

        // 检查是否应该跳过
        if (ShouldSkip(findData.cFileName, findData.dwFileAttributes)) continue;

        std::wstring fullPath = path;
        if (!fullPath.empty() && fullPath.back() != L'\\') fullPath += L'\\';
        fullPath += findData.cFileName;

        if (progressCallback) {
            progressCallback(fullPath);
        }

        // 计算文件夹大小
        uint64_t size = Utils::FileUtil::GetFolderSize(fullPath);

        // 如果大于阈值，添加到结果
        if (size >= minSizeBytes_) {
            Models::MigrationItem item;
            item.name = findData.cFileName;
            item.sourcePath = fullPath;
            item.size = size;
            item.type = Models::MigrationType::LargeSoftware;
            item.advice = size > 5ULL * 1024 * 1024 * 1024
                ? Models::MigrationAdvice::Recommended
                : Models::MigrationAdvice::Possible;
            item.selected = false;
            item.migrated = false;

            results.push_back(item);
        }

        // 继续递归扫描子目录
        ScanDirectory(fullPath, results, progressCallback);

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

std::vector<Models::MigrationItem> LargeFolderDetector::Detect(
    std::function<void(const std::wstring&)> progressCallback) {
    cancelled_ = false;

    std::vector<Models::MigrationItem> results;

    std::wstring systemDrive = Utils::Win32Util::GetSystemDrive();
    std::wstring scanPath = systemDrive;
    if (scanPath.back() != L'\\') scanPath += L'\\';

    // 扫描C盘根目录下的所有一级和二级目录
    std::wstring searchPath = scanPath + L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return results;

    do {
        if (cancelled_) break;

        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;

        if (ShouldSkip(findData.cFileName, findData.dwFileAttributes)) continue;

        std::wstring fullPath = scanPath + findData.cFileName;

        if (progressCallback) {
            progressCallback(fullPath);
        }

        // 计算一级目录大小
        uint64_t size = Utils::FileUtil::GetFolderSize(fullPath);

        if (size >= minSizeBytes_) {
            Models::MigrationItem item;
            item.name = findData.cFileName;
            item.sourcePath = fullPath;
            item.size = size;
            item.type = Models::MigrationType::LargeSoftware;
            item.advice = size > 5ULL * 1024 * 1024 * 1024
                ? Models::MigrationAdvice::Recommended
                : Models::MigrationAdvice::Possible;
            item.selected = false;
            item.migrated = false;

            results.push_back(item);
        }

        // 递归扫描子目录，查找更深层次的大文件夹
        ScanDirectory(fullPath, results, progressCallback);

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);

    // 按大小降序排序
    std::sort(results.begin(), results.end(),
              [](const Models::MigrationItem& a, const Models::MigrationItem& b) {
                  return a.size > b.size;
              });

    return results;
}

void LargeFolderDetector::Cancel() {
    cancelled_ = true;
}

} // namespace IceClean::Core::Migrator
