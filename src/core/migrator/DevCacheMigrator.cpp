#include "DevCacheMigrator.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include "utils/JunctionPoint.h"
#include <shlobj.h>

namespace IceClean::Core::Migrator {

std::vector<DevCacheInfo> DevCacheMigrator::GetDevCachePaths() const {
    std::vector<DevCacheInfo> paths;

    std::wstring localAppData = Utils::Win32Util::GetSpecialFolder(CSIDL_LOCAL_APPDATA);
    std::wstring userProfile = Utils::Win32Util::GetSpecialFolder(CSIDL_PROFILE);

    if (localAppData.empty() || userProfile.empty()) return paths;

    // 辅助函数：拼接路径
    auto joinPath = [](const std::wstring& base, const std::wstring& sub) -> std::wstring {
        std::wstring result = base;
        if (result.back() != L'\\') result += L'\\';
        result += sub;
        return result;
    };

    // npm cache
    paths.push_back({L"npm cache", joinPath(localAppData, L"npm-cache")});

    // yarn cache
    paths.push_back({L"yarn cache", joinPath(localAppData, L"Yarn\\Cache")});

    // pnpm store
    paths.push_back({L"pnpm store", joinPath(localAppData, L"pnpm-store")});

    // .node-gyp
    paths.push_back({L".node-gyp", joinPath(userProfile, L".node-gyp")});

    // pip cache
    paths.push_back({L"pip cache", joinPath(localAppData, L"pip\\Cache")});

    // conda pkgs (miniconda3)
    paths.push_back({L"conda pkgs (miniconda3)", joinPath(userProfile, L"miniconda3\\pkgs")});

    // conda pkgs (anaconda3)
    paths.push_back({L"conda pkgs (anaconda3)", joinPath(userProfile, L"anaconda3\\pkgs")});

    // Maven
    paths.push_back({L"Maven repository", joinPath(userProfile, L".m2\\repository")});

    // Gradle caches
    paths.push_back({L"Gradle caches", joinPath(userProfile, L".gradle\\caches")});

    // Go mod cache
    paths.push_back({L"Go mod cache", joinPath(userProfile, L"go\\pkg\\mod")});

    // NuGet
    paths.push_back({L"NuGet packages", joinPath(userProfile, L".nuget\\packages")});

    // Cargo
    paths.push_back({L"Cargo registry", joinPath(userProfile, L".cargo\\registry")});

    // Electron
    paths.push_back({L"Electron cache", joinPath(localAppData, L"electron\\Cache")});

    return paths;
}

uint64_t DevCacheMigrator::CalculateDirectorySize(const std::wstring& path) const {
    return Utils::FileUtil::GetFolderSize(path);
}

std::vector<Models::MigrationItem> DevCacheMigrator::Detect() {
    std::vector<Models::MigrationItem> items;

    std::vector<DevCacheInfo> cachePaths = GetDevCachePaths();
    std::wstring systemDrive = Utils::Win32Util::GetSystemDrive();

    for (const auto& cache : cachePaths) {
        // 检查路径是否存在
        if (!Utils::FileUtil::Exists(cache.sourcePath)) continue;

        // 检查是否已经是联接链接
        if (Utils::JunctionPoint::IsJunction(cache.sourcePath)) continue;

        // 检查是否在C盘
        if (cache.sourcePath.size() < 2 || towlower(cache.sourcePath[0]) != towlower(systemDrive[0])) continue;

        // 计算目录大小
        uint64_t size = CalculateDirectorySize(cache.sourcePath);
        if (size == 0) continue;

        Models::MigrationItem item;
        item.name = cache.name;
        item.sourcePath = cache.sourcePath;
        item.size = size;
        item.type = Models::MigrationType::DevCache;
        item.advice = Models::MigrationAdvice::Recommended;
        item.selected = false;
        item.migrated = false;

        items.push_back(item);
    }

    return items;
}

Models::MigrationResult DevCacheMigrator::Migrate(const std::vector<Models::MigrationItem>& items,
                                                    const std::wstring& targetDrive,
                                                    std::function<void(const Models::MigrationProgress&)> progressCb) {
    Models::MigrationResult result;

    if (items.empty()) {
        result.errorMessage = L"没有需要迁移的项目";
        return result;
    }

    // 检查目标驱动器空间
    uint64_t driveTotal = 0, driveFree = 0;
    if (Utils::Win32Util::GetDiskSpace(targetDrive, driveTotal, driveFree)) {
        uint64_t totalNeeded = 0;
        for (const auto& item : items) {
            if (item.selected) totalNeeded += item.size;
        }
        if (driveFree < totalNeeded) {
            result.errorMessage = L"目标驱动器空间不足";
            return result;
        }
    }

    int totalItems = static_cast<int>(items.size());
    int migratedCount = 0;
    int failedCount = 0;
    uint64_t totalMigratedSize = 0;

    for (int i = 0; i < totalItems; ++i) {
        if (!items[i].selected) continue;

        if (progressCb) {
            Models::MigrationProgress progress;
            progress.currentItem = i + 1;
            progress.totalItems = totalItems;
            progress.migratedSize = totalMigratedSize;
            progress.totalSize = items[i].size;
            progress.currentFile = items[i].sourcePath;
            progress.isRunning = true;
            progressCb(progress);
        }

        bool success = MoveAndCreateJunction(items[i].sourcePath, targetDrive,
                                              progressCb, i + 1, totalItems);

        if (success) {
            ++migratedCount;
            totalMigratedSize += items[i].size;
        } else {
            ++failedCount;
        }
    }

    result.success = failedCount == 0;
    result.migratedCount = migratedCount;
    result.failedCount = failedCount;
    result.totalMigratedSize = totalMigratedSize;

    if (failedCount > 0 && migratedCount == 0) {
        result.errorMessage = L"所有迁移项目均失败";
    }

    return result;
}

} // namespace IceClean::Core::Migrator
