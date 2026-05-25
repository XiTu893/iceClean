#pragma once
#include "MigratorBase.h"
#include <vector>

namespace IceClean::Core::Migrator {

class FolderMigrator : public MigratorBase {
public:
    // 构造函数，指定要迁移的文件夹路径和显示名称
    FolderMigrator(const std::wstring& folderPath, const std::wstring& displayName);

    std::wstring GetName() const override;
    Models::MigrationType GetMigrationType() const override;
    std::vector<Models::MigrationItem> Detect() override;
    Models::MigrationResult Migrate(const std::vector<Models::MigrationItem>& items,
                                     const std::wstring& targetDrive,
                                     std::function<void(const Models::MigrationProgress&)> progressCallback = nullptr) override;

private:
    std::wstring folderPath_;
    std::wstring displayName_;
};

} // namespace IceClean::Core::Migrator
