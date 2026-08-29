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
        L"$windows.~bt",
        L"$windows.~ws",
        L"windows.old",
        L"recovery",
        L"perflogs",
        L"intel",
        L"amd",
        L"nvidia",
        // 云同步目录：junction 迁移会破坏同步客户端的路径跟踪
        L"onedrive",
        L"dropbox",
    };
    return names;
}

bool LargeFolderDetector::ShouldSkip(const std::wstring& folderName, DWORD attributes) const {
    // 跳过系统文件夹。注意：不能跳过 FILE_ATTRIBUTE_HIDDEN ——
    // AppData 等核心可迁移目录本身即隐藏属性
    if (attributes & FILE_ATTRIBUTE_SYSTEM) {
        return true;
    }

    // 跳过联接链接
    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        return true;
    }

    // 检查是否在跳过列表中（含前缀匹配，覆盖 "OneDrive - xxx" 等变体）
    std::wstring lowerName = folderName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), towlower);

    for (const auto& skip : GetSkippedFolderNames()) {
        if (lowerName == skip) return true;
        if ((skip == L"onedrive" || skip == L"dropbox") &&
            lowerName.rfind(skip, 0) == 0) {
            return true;
        }
    }

    return false;
}

uint64_t LargeFolderDetector::SumItemBytes(const std::vector<Models::MigrationItem>& items) {
    uint64_t b = 0;
    for (const auto& r : items) b += r.size;
    return b;
}

void LargeFolderDetector::MeasureDirectory(const std::wstring& dir,
                                           const std::vector<Models::MigrationItem>& items,
                                           uint64_t baseBytes,
                                           ProgressCallback& cb,
                                           uint64_t& outTotal) {
    outTotal = 0;
    if (cancelled_) return;

    std::wstring searchPath = dir;
    if (!searchPath.empty() && searchPath.back() != L'\\') searchPath += L'\\';
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

        std::wstring full = dir;
        if (!full.empty() && full.back() != L'\\') full += L'\\';
        full += findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 联接目标会在别处统计，避免重复
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;

            // 进入即上报：大子树统计期间 UI 也有持续路径反馈
            if (cb) cb(full, static_cast<int>(items.size()),
                       baseBytes + outTotal);

            uint64_t sub = 0;
            MeasureDirectory(full, items, baseBytes + outTotal, cb, sub);
            outTotal += sub;
        } else {
            ULARGE_INTEGER sz{};
            sz.LowPart = findData.nFileSizeLow;
            sz.HighPart = findData.nFileSizeHigh;
            outTotal += sz.QuadPart;
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

void LargeFolderDetector::ScanDirectory(const std::wstring& path,
                                          std::vector<Models::MigrationItem>& results,
                                          ProgressCallback& progressCallback) {
    if (cancelled_) return;

    std::wstring searchPath = path;
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath += L'\\';
    }
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    const uint64_t itemsBytes = SumItemBytes(results);

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

        // 流式测量：统计期间持续上报当前遍历路径
        if (progressCallback) {
            progressCallback(fullPath, static_cast<int>(results.size()), itemsBytes);
        }
        uint64_t size = 0;
        MeasureDirectory(fullPath, results, itemsBytes, progressCallback, size);

        // 如果大于阈值，添加到结果；父目录已整体入列，不再下钻
        // （子文件夹包含其中，重复列出会导致父子同时勾选、二次迁移失效）
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

            if (progressCallback) {
                progressCallback(fullPath, static_cast<int>(results.size()),
                                 SumItemBytes(results));
            }
            continue;
        }

        // 未达阈值，继续递归查找更深层次的大文件夹
        ScanDirectory(fullPath, results, progressCallback);

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

std::vector<Models::MigrationItem> LargeFolderDetector::Detect(
    ProgressCallback progressCallback) {
    std::wstring systemDrive = Utils::Win32Util::GetSystemDrive();
    std::wstring scanPath = systemDrive;
    if (scanPath.back() != L'\\') scanPath += L'\\';
    return DetectAt(scanPath, std::move(progressCallback));
}

std::vector<Models::MigrationItem> LargeFolderDetector::DetectAt(
    const std::wstring& rootPath, ProgressCallback progressCallback) {
    cancelled_ = false;

    std::vector<Models::MigrationItem> results;

    std::wstring scanPath = rootPath;
    if (scanPath.empty()) return results;
    if (scanPath.back() != L'\\') scanPath += L'\\';

    // 扫描根目录下的一级目录
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

        // 流式测量：统计期间持续上报当前遍历路径（大目录也有细节反馈）
        if (progressCallback) {
            progressCallback(fullPath, static_cast<int>(results.size()),
                             SumItemBytes(results));
        }
        uint64_t size = 0;
        MeasureDirectory(fullPath, results, SumItemBytes(results),
                         progressCallback, size);

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

        // 上报放在入列之后，保证 foundCount/bytes 反映最新状态
        if (progressCallback) {
            uint64_t bytesSoFar = 0;
            for (auto& r : results) bytesSoFar += r.size;
            progressCallback(fullPath, static_cast<int>(results.size()), bytesSoFar);
        }

        if (size >= minSizeBytes_) {
            continue;  // 一级目录已入列，不再下钻重复统计
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
