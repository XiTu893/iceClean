#pragma once
#include <string>
#include <vector>
#include <functional>
#include "core/driver/DeviceDriverInfo.h"

namespace IceClean::Core::Driver {

// 备份编排：目录规划、pnputil 导出、manifest.json 读写、历史备份列举、还原。
class DriverBackupManager {
public:
    struct BackupResult {
        bool success = false;
        std::wstring backupDir;   // 实际生成的带时间戳的备份目录
        int packageCount = 0;     // 导出的驱动包数量
    };

    // 默认备份根目录：%APPDATA%\IceClean\Backups\Drivers
    static std::wstring GetDefaultBackupRoot();

    // 全量备份到 backupRoot 下的时间戳子目录
    // progress: 当前文件数 / 预计总数 / 当前文件名
    static BackupResult BackupAll(const std::wstring& backupRoot,
        std::function<void(int, int, const std::wstring&)> progress = nullptr);

    // 还原：自动创建系统还原点 → 导入并安装备份目录内全部 INF
    // 返回是否成功（还原点失败会中止，不执行导入）
    static bool RestoreFrom(const std::wstring& backupDir);

    // 列举历史备份（含 manifest.json 的目录及其元信息）
    struct BackupEntry {
        std::wstring dir;
        std::wstring time;        // 目录名（时间戳）
        int packageCount = 0;
        std::wstring systemVersion;
    };
    static std::vector<BackupEntry> ListBackups(const std::wstring& backupRoot);

    // 清理 DriverStore 中不再使用的旧驱动包（DISM 组件清理），返回释放的字节数
    static uint64_t CleanupOldBackups();
};

} // namespace IceClean::Core::Driver
