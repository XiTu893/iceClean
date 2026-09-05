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
        // 系统目录
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
        // 开发工具缓存目录（由 DevCacheMigrator 专门处理）
        L".gradle",
        L".m2",
        L".npm",
        L".yarn",
        L".pnpm-store",
        // node_modules 常见于大量项目，迁移价值低
        L"node_modules",
        L"bower_components",
        L"__pycache__",
        L".pytest_cache",
        L".tox",
        L".venv",
        L"venv",
        L".idea",
        L".vscode",
        // AppData 是容器目录：其内部子项（如 vcpkg、Google、微信等）是真正可迁移目标，
        // 不应让 AppData 整体入列（27GB），而应扫描其子目录找细粒度大项
        L"appdata",
    };
    return names;
}

bool LargeFolderDetector::ShouldSkip(const std::wstring& folderName, DWORD attributes) const {
    // 跳过联接链接（Junction/Symlink）
    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        return true;
    }

    std::wstring lowerName = folderName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), towlower);

    // 跳过列表（前缀匹配，覆盖 "OneDrive - xxx"、"onedrive-person" 等变体）
    for (const auto& skip : GetSkippedFolderNames()) {
        if (lowerName == skip) return true;
        // 所有跳过项都支持前缀匹配（如 "onedrive" 匹配 "onedrive-person"）
        if (lowerName.rfind(skip, 0) == 0) return true;
    }

    return false;
}

uint64_t LargeFolderDetector::SumItemBytes(const std::vector<Models::MigrationItem>& items) {
    uint64_t b = 0;
    for (const auto& r : items) b += r.size;
    return b;
}

bool LargeFolderDetector::IsUserProfileDir(const std::wstring& dirPath) const {
    static const std::vector<std::wstring> kMarkers = {
        L"\\AppData",
        L"\\Desktop",
        L"\\Documents",
        L"\\Downloads",
        L"\\Pictures",
        L"\\Videos",
        L"\\Music",
    };
    for (const auto& marker : kMarkers) {
        std::wstring test = dirPath + marker;
        if (Utils::FileUtil::Exists(test)) return true;
    }
    return false;
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
        if (ShouldSkip(findData.cFileName, findData.dwFileAttributes)) {
            // AppData 容器：跳过自身但下钻子目录，让 vcpkg/Google 等真正可迁移的大子项被发现
            if (_wcsicmp(findData.cFileName, L"AppData") == 0) {
                std::wstring full = path;
                if (!full.empty() && full.back() != L'\\') full += L'\\';
                full += findData.cFileName;
                ScanContainerChildren(full, results, progressCallback);
            }
            continue;
        }

        std::wstring fullPath = path;
        if (!fullPath.empty() && fullPath.back() != L'\\') fullPath += L'\\';
        fullPath += findData.cFileName;

        if (progressCallback) {
            progressCallback(fullPath, static_cast<int>(results.size()), itemsBytes);
        }
        uint64_t size = 0;
        MeasureDirectory(fullPath, results, itemsBytes, progressCallback, size);

        if (size >= minSizeBytes_) {
            // 大于 5GB 的目录几乎一定是"容器"（如 AppData\Local、.lmstudio、用户配置目录），
            // 整体入列会屏蔽内部的真正可迁移大子项（vcpkg、模型文件等），改为下钻
            constexpr uint64_t kContainerThreshold = 5ULL * 1024 * 1024 * 1024;
            if (size >= kContainerThreshold) {
                ScanDirectory(fullPath, results, progressCallback);
                continue;
            }

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

    // 扫描根目录的一级子目录
    std::wstring searchPath = scanPath + L"*";
    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (cancelled_) break;
            if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;

            // C:\Users 是用户目录容器，跳过自身但深入扫描其子目录
            if (_wcsicmp(findData.cFileName, L"Users") == 0) {
                ScanUsersContainer(scanPath + findData.cFileName, results, progressCallback);
                continue;
            }

            if (ShouldSkip(findData.cFileName, findData.dwFileAttributes)) continue;

            std::wstring fullPath = scanPath + findData.cFileName;

            if (progressCallback) {
                progressCallback(fullPath, static_cast<int>(results.size()), SumItemBytes(results));
            }
            uint64_t size = 0;
            MeasureDirectory(fullPath, results, SumItemBytes(results), progressCallback, size);

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

            if (progressCallback) {
                uint64_t bytesSoFar = 0;
                for (auto& r : results) bytesSoFar += r.size;
                progressCallback(fullPath, static_cast<int>(results.size()), bytesSoFar);
            }

            if (size < minSizeBytes_) {
                ScanDirectory(fullPath, results, progressCallback);
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    std::sort(results.begin(), results.end(),
              [](const Models::MigrationItem& a, const Models::MigrationItem& b) {
                  return a.size > b.size;
              });

    return results;
}

void LargeFolderDetector::ScanUsersContainer(const std::wstring& usersPath,
                                            std::vector<Models::MigrationItem>& results,
                                            ProgressCallback& progressCallback) {
    if (cancelled_) return;

    std::wstring searchPath = usersPath;
    if (!searchPath.empty() && searchPath.back() != L'\\') searchPath += L'\\';
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (cancelled_) break;
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;

        std::wstring fullPath = usersPath;
        if (!fullPath.empty() && fullPath.back() != L'\\') fullPath += L'\\';
        fullPath += findData.cFileName;

        // Public / Default 等系统用户目录不是迁移目标，扫描其子目录即可
        if (_wcsicmp(findData.cFileName, L"Public") == 0 ||
            _wcsicmp(findData.cFileName, L"Default") == 0 ||
            _wcsicmp(findData.cFileName, L"Default User") == 0) {
            ScanContainerChildren(fullPath, results, progressCallback);
            continue;
        }

        if (progressCallback) {
            progressCallback(fullPath, static_cast<int>(results.size()), SumItemBytes(results));
        }
        uint64_t size = 0;
        MeasureDirectory(fullPath, results, SumItemBytes(results), progressCallback, size);

        // 用户配置文件目录（如 zeus-zzp）永远不入列，但始终下钻以发现 AppData 等大子项
        bool isUserProfile = IsUserProfileDir(fullPath);
        if (size >= minSizeBytes_ && !isUserProfile) {
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

        if (progressCallback) {
            uint64_t bytesSoFar = 0;
            for (auto& r : results) bytesSoFar += r.size;
            progressCallback(fullPath, static_cast<int>(results.size()), bytesSoFar);
        }

        // 始终下钻：即使是大目录（用户配置目录），也要深入找 AppData 等子项
        ScanDirectory(fullPath, results, progressCallback);
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

void LargeFolderDetector::ScanContainerChildren(const std::wstring& containerPath,
                                                  std::vector<Models::MigrationItem>& results,
                                                  ProgressCallback& progressCallback) {
    if (cancelled_) return;

    std::wstring searchPath = containerPath;
    if (!searchPath.empty() && searchPath.back() != L'\\') searchPath += L'\\';
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (cancelled_) break;
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;
        if (ShouldSkip(findData.cFileName, findData.dwFileAttributes)) continue;

        std::wstring full = containerPath;
        if (!full.empty() && full.back() != L'\\') full += L'\\';
        full += findData.cFileName;

        // 直接用 ScanDirectory：会测量 + 按阈值入列 + 未达阈值继续下钻
        ScanDirectory(full, results, progressCallback);
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

void LargeFolderDetector::Cancel() {
    cancelled_ = true;
}

} // namespace IceClean::Core::Migrator
