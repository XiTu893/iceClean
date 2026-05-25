#pragma once
#include "MigratorBase.h"
#include <vector>

namespace IceClean::Core::Migrator {

class QQMigrator : public MigratorBase {
public:
    std::wstring GetName() const override;
    Models::MigrationType GetMigrationType() const override;
    std::vector<Models::MigrationItem> Detect() override;
    Models::MigrationResult Migrate(const std::vector<Models::MigrationItem>& items,
                                     const std::wstring& targetDrive,
                                     std::function<void(const Models::MigrationProgress&)> progressCallback = nullptr) override;

private:
    // 获取QQ数据文件夹路径
    std::wstring GetQQDataPath() const;

    // 检查QQ是否正在运行
    bool IsQQRunning() const;
};

} // namespace IceClean::Core::Migrator
