#pragma once
#include "models/DriverInfo.h"
#include <vector>
#include <string>
#include <functional>
#include <windows.h>

namespace IceClean::Core::Optimizer {

// 驱动管理器
class DriverManager {
public:
    // 获取所有驱动列表
    std::vector<Models::DriverInfo> GetDrivers();

    // 获取有更新的驱动列表
    std::vector<Models::DriverInfo> GetOutdatedDrivers();

    // 获取第三方驱动列表
    std::vector<Models::DriverInfo> GetThirdPartyDrivers();

    // 备份指定驱动
    bool BackupDriver(const Models::DriverInfo& driver, const std::wstring& backupDir);

    // 批量备份驱动
    int BackupAllDrivers(const std::wstring& backupDir,
                         std::function<void(int, int, const std::wstring&)> progress = nullptr);

    // 从备份还原驱动
    bool RestoreDriver(const Models::DriverBackupInfo& backup);

    // 获取已备份的驱动列表
    std::vector<Models::DriverBackupInfo> GetBackupDrivers(const std::wstring& backupDir);

    // 清理旧驱动备份(在DriverStore中)
    uint64_t CleanupOldDriverBackups(std::function<void(const std::wstring&)> progress = nullptr);

    // 获取驱动存储占用大小
    uint64_t GetDriverStoreSize();

private:
    // 从注册表读取驱动信息
    void ReadDriverFromRegistry(HKEY hKey, const std::wstring& subKey, Models::DriverInfo& info);

    // 判断是否为系统驱动
    bool IsSystemDriver(const std::wstring& provider) const;

    // 判断驱动是否有更新可用
    bool CheckDriverUpdate(const Models::DriverInfo& driver) const;
};

} // namespace IceClean::Core::Optimizer
