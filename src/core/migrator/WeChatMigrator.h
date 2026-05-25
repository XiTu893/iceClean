#pragma once
#include "MigratorBase.h"
#include <vector>

namespace IceClean::Core::Migrator {

class WeChatMigrator : public MigratorBase {
public:
    std::wstring GetName() const override;
    Models::MigrationType GetMigrationType() const override;
    std::vector<Models::MigrationItem> Detect() override;
    Models::MigrationResult Migrate(const std::vector<Models::MigrationItem>& items,
                                     const std::wstring& targetDrive,
                                     std::function<void(const Models::MigrationProgress&)> progressCallback = nullptr) override;

private:
    // 获取WeChat数据文件夹路径
    std::wstring GetWeChatDataPath() const;

    // 检查WeChat是否正在运行
    bool IsWeChatRunning() const;
};

} // namespace IceClean::Core::Migrator
