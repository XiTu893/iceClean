#include "UserFolderMigrator.h"
#include "utils/RegistryUtil.h"
#include <shlobj.h>

#ifndef CSIDL_DOWNLOADS
#define CSIDL_DOWNLOADS 0x0027
#endif

namespace IceClean::Core::Migrator {

std::wstring UserFolderMigrator::GetName() const {
    return L"用户文件夹迁移";
}

Models::MigrationType UserFolderMigrator::GetMigrationType() const {
    return Models::MigrationType::UserFolder;
}

const std::vector<UserFolderInfo>& UserFolderMigrator::GetUserFolderInfos() {
    static const std::vector<UserFolderInfo> infos = {
        { CSIDL_DESKTOP,           L"桌面",     L"{B4BFCC3A-DB2C-424C-B029-7FE99A87C641}" },
        { CSIDL_PERSONAL,          L"文档",     L"{FDD39AD0-238F-46AF-ADB4-6C85480369C7}" },
        { CSIDL_DOWNLOADS,         L"下载",     L"{374DE290-123F-4565-9164-39C4925E467B}" },
        { CSIDL_MYPICTURES,        L"图片",     L"{33E28130-4E1E-4676-835A-98304C4E5218}" },
        { CSIDL_MYVIDEO,           L"视频",     L"{18989B1D-99B5-455B-841C-AB7C74E4DDFC}" },
        { CSIDL_MYMUSIC,           L"音乐",     L"{4BD8D571-6D19-48D3-BE97-422220080E43}" },
    };
    return infos;
}

std::vector<Models::MigrationItem> UserFolderMigrator::Detect() {
    std::vector<Models::MigrationItem> items;
    std::wstring systemDrive = Utils::Win32Util::GetSystemDrive();

    for (const auto& info : GetUserFolderInfos()) {
        std::wstring currentPath = Utils::Win32Util::GetSpecialFolder(info.csidl);
        if (currentPath.empty()) continue;

        // 只检测C盘上的文件夹
        if (currentPath.size() < 2 || towlower(currentPath[0]) != towlower(systemDrive[0])) {
            continue;
        }

        // 检查是否已经是联接链接
        if (Utils::JunctionPoint::IsJunction(currentPath)) {
            continue;
        }

        uint64_t size = Utils::FileUtil::GetFolderSize(currentPath);

        Models::MigrationItem item;
        item.name = info.name;
        item.sourcePath = currentPath;
        item.size = size;
        item.type = Models::MigrationType::UserFolder;
        item.advice = size > 1024ULL * 1024 * 1024
            ? Models::MigrationAdvice::Recommended
            : Models::MigrationAdvice::Possible;
        item.selected = false;
        item.migrated = false;

        items.push_back(item);
    }

    return items;
}

bool UserFolderMigrator::UpdateUserShellFolder(const std::wstring& registryName, const std::wstring& newPath) const {
    const std::wstring subKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\User Shell Folders";
    return Utils::RegistryUtil::WriteStringValue(HKEY_CURRENT_USER, subKey, registryName, newPath);
}

Models::MigrationResult UserFolderMigrator::Migrate(const std::vector<Models::MigrationItem>& items,
                                                      const std::wstring& targetDrive,
                                                      std::function<void(const Models::MigrationProgress&)> progressCallback) {
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

        const auto& item = items[i];

        // 安全检查：确保源和目标在不同驱动器
        if (!IsOnDifferentDrive(item.sourcePath, targetDrive)) {
            ++failedCount;
            continue;
        }

        if (progressCallback) {
            Models::MigrationProgress progress;
            progress.currentItem = i + 1;
            progress.totalItems = totalItems;
            progress.migratedSize = totalMigratedSize;
            progress.totalSize = item.size;
            progress.currentFile = item.sourcePath;
            progress.isRunning = true;
            progressCallback(progress);
        }

        // 构建目标路径
        std::wstring targetPath = BuildTargetPath(item.sourcePath, targetDrive);

        // 确保目标路径的父目录存在
        std::wstring parentDir = targetPath;
        auto lastSlash = parentDir.find_last_of(L'\\');
        if (lastSlash != std::wstring::npos) {
            parentDir = parentDir.substr(0, lastSlash);
            if (!Utils::FileUtil::Exists(parentDir)) {
                Utils::FileUtil::CreateDirectoryRecursive(parentDir);
            }
        }

        // 复制文件夹到目标驱动器
        bool copySuccess = Utils::FileUtil::CopyFolder(item.sourcePath, targetPath);
        if (!copySuccess) {
            Utils::FileUtil::DeleteFolder(targetPath);
            ++failedCount;
            continue;
        }

        // 删除源文件夹
        if (!Utils::FileUtil::DeleteFolder(item.sourcePath)) {
            // 删除源失败，回滚
            Utils::FileUtil::DeleteFolder(targetPath);
            ++failedCount;
            continue;
        }

        // 创建联接链接
        if (!Utils::JunctionPoint::Create(item.sourcePath, targetPath)) {
            // 创建联接失败，尝试恢复
            Utils::FileUtil::MoveFolder(targetPath, item.sourcePath);
            ++failedCount;
            continue;
        }

        // 更新注册表中的用户Shell文件夹路径
        // 查找对应的注册表名称
        for (const auto& info : GetUserFolderInfos()) {
            std::wstring currentPath = Utils::Win32Util::GetSpecialFolder(info.csidl);
            // 比较路径（不区分大小写）
            if (_wcsicmp(currentPath.c_str(), item.sourcePath.c_str()) == 0 ||
                _wcsicmp(info.name.c_str(), item.name.c_str()) == 0) {
                UpdateUserShellFolder(info.registryName, targetPath);
                break;
            }
        }

        ++migratedCount;
        totalMigratedSize += item.size;
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
