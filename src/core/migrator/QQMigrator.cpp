#include "QQMigrator.h"
#include "utils/Win32Util.h"
#include <shlobj.h>

namespace IceClean::Core::Migrator {

std::wstring QQMigrator::GetName() const {
    return L"QQ缓存迁移";
}

Models::MigrationType QQMigrator::GetMigrationType() const {
    return Models::MigrationType::QQCache;
}

std::wstring QQMigrator::GetQQDataPath() const {
    // QQ数据默认在 Documents\Tencent Files\
    std::wstring docsPath = Utils::Win32Util::GetSpecialFolder(CSIDL_PERSONAL);
    if (docsPath.empty()) return L"";

    std::wstring qqPath = docsPath;
    if (qqPath.back() != L'\\') qqPath += L'\\';
    qqPath += L"Tencent Files";

    if (Utils::FileUtil::Exists(qqPath)) {
        return qqPath;
    }

    return L"";
}

bool QQMigrator::IsQQRunning() const {
    // 检查QQ和TIM进程
    return IsProcessRunning(L"QQ.exe") || IsProcessRunning(L"TIM.exe");
}

std::vector<Models::MigrationItem> QQMigrator::Detect() {
    std::vector<Models::MigrationItem> items;

    std::wstring qqPath = GetQQDataPath();
    if (qqPath.empty()) return items;

    std::wstring systemDrive = Utils::Win32Util::GetSystemDrive();

    // 只检测C盘上的QQ数据
    if (qqPath.size() < 2 || towlower(qqPath[0]) != towlower(systemDrive[0])) {
        return items;
    }

    // 检查是否已经是联接链接
    if (Utils::JunctionPoint::IsJunction(qqPath)) {
        return items;
    }

    // 扫描QQ号子目录(纯数字命名的目录)
    std::wstring searchPath = qqPath;
    if (searchPath.back() != L'\\') searchPath += L'\\';
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        return items;
    }

    do {
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (wcscmp(findData.cFileName, L".") == 0 || wcscmp(findData.cFileName, L"..") == 0) continue;

        // 检查是否为QQ号目录(纯数字或以数字开头的目录)
        bool isQQDir = false;
        if (!findData.cFileName[0]) continue;

        // 纯数字目录(传统QQ号)
        isQQDir = true;
        for (const wchar_t* p = findData.cFileName; *p; ++p) {
            if (!iswdigit(*p)) {
                isQQDir = false;
                break;
            }
        }

        // 也包含一些特殊目录如 "All Users" 等，跳过
        if (!isQQDir) continue;

        std::wstring qqDirPath = qqPath;
        if (qqDirPath.back() != L'\\') qqDirPath += L'\\';
        qqDirPath += findData.cFileName;

        // 跳过联接链接
        if (Utils::JunctionPoint::IsJunction(qqDirPath)) continue;

        uint64_t size = Utils::FileUtil::GetFolderSize(qqDirPath);

        Models::MigrationItem item;
        item.name = std::wstring(L"QQ - ") + findData.cFileName;
        item.sourcePath = qqDirPath;
        item.size = size;
        item.type = Models::MigrationType::QQCache;
        item.advice = size > 500ULL * 1024 * 1024
            ? Models::MigrationAdvice::Recommended
            : Models::MigrationAdvice::Possible;
        item.selected = false;
        item.migrated = false;

        items.push_back(item);
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);

    return items;
}

Models::MigrationResult QQMigrator::Migrate(const std::vector<Models::MigrationItem>& items,
                                              const std::wstring& targetDrive,
                                              std::function<void(const Models::MigrationProgress&)> progressCallback) {
    Models::MigrationResult result;

    if (IsQQRunning()) {
        result.errorMessage = L"QQ正在运行，请先关闭QQ后再迁移";
        return result;
    }

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
