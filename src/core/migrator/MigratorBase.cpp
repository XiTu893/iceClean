#include "MigratorBase.h"

namespace IceClean::Core::Migrator {

bool MigratorBase::MoveAndCreateJunction(const std::wstring& sourcePath, const std::wstring& targetDrive) {
    return MoveAndCreateJunction(sourcePath, targetDrive, nullptr, 0, 0);
}

bool MigratorBase::MoveAndCreateJunction(const std::wstring& sourcePath, const std::wstring& targetDrive,
                                          std::function<void(const Models::MigrationProgress&)> progressCallback,
                                          int currentItem, int totalItems) {
    // 检查源路径是否存在
    if (!Utils::FileUtil::Exists(sourcePath)) {
        return false;
    }

    // 检查是否已经是联接链接
    if (Utils::JunctionPoint::IsJunction(sourcePath)) {
        return false;
    }

    // 构建目标路径
    std::wstring targetPath = BuildTargetPath(sourcePath, targetDrive);

    // 如果目标路径已存在，不覆盖
    if (Utils::FileUtil::Exists(targetPath)) {
        return false;
    }

    // 确保目标路径的父目录存在
    std::wstring parentDir = targetPath;
    auto lastSlash = parentDir.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        parentDir = parentDir.substr(0, lastSlash);
        if (!Utils::FileUtil::Exists(parentDir)) {
            Utils::FileUtil::CreateDirectoryRecursive(parentDir);
        }
    }

    // 计算源文件夹大小用于进度报告
    uint64_t totalSize = Utils::FileUtil::GetFolderSize(sourcePath);
    uint64_t migratedSize = 0;

    // 复制文件夹到目标驱动器(带进度)
    auto copyProgress = [&](uint64_t copied, uint64_t total) -> bool {
        if (progressCallback) {
            Models::MigrationProgress progress;
            progress.currentItem = currentItem;
            progress.totalItems = totalItems;
            progress.migratedSize = copied;
            progress.totalSize = totalSize;
            progress.isRunning = true;
            progressCallback(progress);
        }
        return true;
    };

    bool copySuccess = Utils::FileUtil::CopyFolder(sourcePath, targetPath, copyProgress);
    if (!copySuccess) {
        // 复制失败，清理目标
        Utils::FileUtil::DeleteFolder(targetPath);
        return false;
    }

    // 删除源文件夹
    if (!Utils::FileUtil::DeleteFolder(sourcePath)) {
        // 删除源失败，回滚：删除目标，保留源
        Utils::FileUtil::DeleteFolder(targetPath);
        return false;
    }

    // 创建联接链接
    if (!Utils::JunctionPoint::Create(sourcePath, targetPath)) {
        // 创建联接失败，尝试恢复：将目标移回源路径
        Utils::FileUtil::MoveFolder(targetPath, sourcePath);
        return false;
    }

    return true;
}

std::wstring MigratorBase::BuildTargetPath(const std::wstring& sourcePath, const std::wstring& targetDrive) const {
    // 将C:\xxx替换为D:\xxx
    if (sourcePath.size() >= 2 && sourcePath[1] == L':') {
        std::wstring relativePath = sourcePath.substr(2); // 去掉 "C:"
        std::wstring target = targetDrive;
        // 确保目标驱动器路径格式正确
        if (target.back() != L'\\') {
            target += L'\\';
        }
        // 去掉relativePath开头的反斜杠
        if (!relativePath.empty() && relativePath[0] == L'\\') {
            relativePath = relativePath.substr(1);
        }
        return target + relativePath;
    }
    return sourcePath;
}

bool MigratorBase::IsOnDifferentDrive(const std::wstring& sourcePath, const std::wstring& targetDrive) const {
    if (sourcePath.size() < 2 || targetDrive.empty()) return false;

    wchar_t sourceDrive = sourcePath[0];
    wchar_t target = targetDrive[0];

    // 不区分大小写比较
    return towlower(sourceDrive) != towlower(target);
}

bool MigratorBase::IsProcessRunning(const std::wstring& processName) const {
    return Utils::Win32Util::IsProcessRunning(processName);
}

} // namespace IceClean::Core::Migrator
