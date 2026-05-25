#include "DriverBackupScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Scanner {

Models::ScanCategory DriverBackupScanner::Scan() {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    // 扫描 C:\Windows\System32\DriverStore\FileRepository
    // 注意：这里只报告大小，实际清理应使用 pnputil 或 DISM
    std::wstring driverStorePath = Utils::Win32Util::ExpandEnvVars(
        L"%SystemRoot%\\System32\\DriverStore\\FileRepository");

    if (Utils::FileUtil::Exists(driverStorePath)) {
        // 获取整个 DriverStore 的大小
        uint64_t driverStoreSize = Utils::FileUtil::GetFolderSize(driverStorePath);

        if (driverStoreSize > 0) {
            // 创建一个虚拟项来表示驱动存储
            Models::ScanFileItem item;
            item.path = driverStorePath;
            item.size = driverStoreSize;
            item.selected = true;

            SYSTEMTIME st;
            GetSystemTime(&st);
            SystemTimeToFileTime(&st, &item.lastWriteTime);

            category.items.push_back(item);
            category.totalSize = driverStoreSize;
        }
    }

    return category;
}

bool DriverBackupScanner::IsAvailable() const {
    std::wstring driverStorePath = Utils::Win32Util::ExpandEnvVars(
        L"%SystemRoot%\\System32\\DriverStore\\FileRepository");
    return Utils::FileUtil::Exists(driverStorePath);
}

} // namespace IceClean::Core::Scanner
