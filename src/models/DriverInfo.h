#pragma once
#include <string>
#include <vector>

namespace IceClean::Models {

// 驱动状态
enum class DriverStatus {
    Running,        // 正常运行
    Stopped,        // 已停止
    Disabled,       // 已禁用
    Unknown         // 未知
};

// 驱动信息
struct DriverInfo {
    std::wstring deviceName;       // 设备名称
    std::wstring driverName;        // 驱动文件名
    std::wstring driverDesc;        // 驱动描述
    std::wstring driverVersion;     // 驱动版本
    std::wstring driverDate;        // 驱动日期
    std::wstring driverProvider;    // 驱动提供商
    std::wstring driverPath;        // 驱动文件路径
    std::wstring infPath;           // INF 文件路径
    std::wstring hardwareId;        // 硬件ID
    DriverStatus status = DriverStatus::Unknown;
    bool hasUpdate = false;         // 是否有更新
    bool isSystemDriver = false;   // 是否系统驱动
    uint64_t driverSize = 0;       // 驱动文件大小
};

// 驱动备份信息
struct DriverBackupInfo {
    std::wstring backupPath;       // 备份路径
    std::wstring driverDesc;       // 驱动描述
    std::wstring driverVersion;    // 驱动版本
    std::wstring backupDate;       // 备份日期
    uint64_t backupSize = 0;      // 备份大小
};

} // namespace IceClean::Models
