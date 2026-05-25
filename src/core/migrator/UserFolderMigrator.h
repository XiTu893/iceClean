#pragma once
#include "MigratorBase.h"
#include <vector>
#include <utility>

namespace IceClean::Core::Migrator {

// 用户特殊文件夹信息
struct UserFolderInfo {
    int csidl;                    // CSIDL常量
    std::wstring name;            // 显示名称
    std::wstring registryName;    // 注册表中的值名称
};

class UserFolderMigrator : public MigratorBase {
public:
    std::wstring GetName() const override;
    Models::MigrationType GetMigrationType() const override;
    std::vector<Models::MigrationItem> Detect() override;
    Models::MigrationResult Migrate(const std::vector<Models::MigrationItem>& items,
                                     const std::wstring& targetDrive,
                                     std::function<void(const Models::MigrationProgress&)> progressCallback = nullptr) override;

private:
    // 获取所有可迁移的用户特殊文件夹定义
    static const std::vector<UserFolderInfo>& GetUserFolderInfos();

    // 更新注册表中的用户Shell文件夹路径
    bool UpdateUserShellFolder(const std::wstring& registryName, const std::wstring& newPath) const;
};

} // namespace IceClean::Core::Migrator
