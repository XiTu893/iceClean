#include "FolderMigrator.h"

namespace IceClean::Core::Migrator {

FolderMigrator::FolderMigrator(const std::wstring& folderPath, const std::wstring& displayName)
    : folderPath_(folderPath), displayName_(displayName) {}

std::wstring FolderMigrator::GetName() const {
    return displayName_;
}

Models::MigrationType FolderMigrator::GetMigrationType() const {
    return Models::MigrationType::CustomFolder;
}

std::vector<Models::MigrationItem> FolderMigrator::Detect() {
    std::vector<Models::MigrationItem> items;

    if (!Utils::FileUtil::Exists(folderPath_)) {
        return items;
    }

    // 检查是否已经是联接链接
    if (Utils::JunctionPoint::IsJunction(folderPath_)) {
        return items;
    }

    // 检查是否在C盘
    std::wstring systemDrive = Utils::Win32Util::GetSystemDrive();
    if (folderPath_.size() < 2 || towlower(folderPath_[0]) != towlower(systemDrive[0])) {
        return items;
    }

    uint64_t size = Utils::FileUtil::GetFolderSize(folderPath_);

    Models::MigrationItem item;
    item.name = displayName_;
    item.sourcePath = folderPath_;
    item.size = size;
    item.type = Models::MigrationType::CustomFolder;
    item.advice = size > 500ULL * 1024 * 1024
        ? Models::MigrationAdvice::Recommended
        : Models::MigrationAdvice::Possible;
    item.selected = false;
    item.migrated = false;

    items.push_back(item);
    return items;
}

Models::MigrationResult FolderMigrator::Migrate(const std::vector<Models::MigrationItem>& items,
                                                  const std::wstring& targetDrive,
                                                  std::function<void(const Models::MigrationProgress&)> progressCallback) {
    Models::MigrationResult result;

    if (items.empty()) {
        result.errorMessage = L"没有需要迁移的项目";
        return result;
    }

    // 检查目标驱动器是否与源在不同驱动器
    if (!IsOnDifferentDrive(folderPath_, targetDrive)) {
        result.errorMessage = L"目标驱动器与源在同一驱动器上";
        return result;
    }

    // 检查目标驱动器是否有足够空间
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

        if (progressCallback) {
            Models::MigrationProgress progress;
            progress.currentItem = i + 1;
            progress.totalItems = totalItems;
            progress.migratedSize = totalMigratedSize;
            progress.totalSize = items[i].size;
            progress.currentFile = items[i].sourcePath;
            progress.isRunning = true;
            progressCallback(progress);
        }

        bool success = MoveAndCreateJunction(items[i].sourcePath, targetDrive,
                                              progressCallback, i + 1, totalItems);

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
