#pragma once
#include <string>
#include <vector>
#include <chrono>

namespace IceClean::Models {

// 备份类型
enum class BackupType {
    RegistryBackup,      // 注册表备份
    DriverBackup,        // 驱动备份
    SystemStateBackup,   // 系统状态备份
    StartupBackup        // 启动项备份
};

// 备份记录
struct BackupRecord {
    std::wstring backupId;          // 备份ID
    BackupType type;                // 备份类型
    std::wstring description;       // 备份描述
    std::wstring backupPath;        // 备份文件路径
    std::chrono::system_clock::time_point createTime; // 创建时间
    uint64_t backupSize = 0;       // 备份大小
    bool canRestore = true;        // 是否可还原
};

// 系统状态摘要(用于仪表盘显示)
struct SystemStateSummary {
    int totalDrivers = 0;           // 驱动总数
    int outdatedDrivers = 0;        // 过时驱动数
    int runningServices = 0;        // 运行中服务数
    int startupItems = 0;           // 启动项数
    int backupCount = 0;            // 备份数量
    uint64_t totalBackupSize = 0;   // 备份总大小
};

} // namespace IceClean::Models
