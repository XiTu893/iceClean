#pragma once
#include "models/BackupInfo.h"
#include <windows.h>
#include <vector>
#include <string>
#include <functional>

namespace IceClean::Core::Safety {

// 系统备份管理器
class SystemBackupManager {
public:
    // 创建注册表备份
    bool BackupRegistry(const std::wstring& backupName,
                        const std::vector<std::wstring>& registryPaths);

    // 创建完整注册表备份
    bool BackupFullRegistry(const std::wstring& backupName);

    // 创建启动项备份
    bool BackupStartupItems(const std::wstring& backupName);

    // 从备份还原注册表
    bool RestoreRegistry(const Models::BackupRecord& record);

    // 获取所有备份列表
    std::vector<Models::BackupRecord> GetBackupList();

    // 删除备份
    bool DeleteBackup(const std::wstring& backupId);

    // 清理过期备份
    int CleanupOldBackups(int keepCount = 10);

    // 获取备份目录路径
    static std::wstring GetBackupDirectory();

    // 获取备份总大小
    uint64_t GetTotalBackupSize();

    // 导出注册表到文件
    static bool ExportRegistryKey(HKEY hKey, const std::wstring& subKey, const std::wstring& filePath);

    // 导入注册表文件
    static bool ImportRegistryFile(const std::wstring& filePath);

private:
    // 生成备份ID
    static std::wstring GenerateBackupId();

    // 保存备份元数据
    bool SaveBackupMetadata(const Models::BackupRecord& record);

    // 读取备份元数据
    Models::BackupRecord ReadBackupMetadata(const std::wstring& backupDir);

    // 获取备份类型名称
    static std::wstring GetBackupTypeName(Models::BackupType type);
};

} // namespace IceClean::Core::Safety
