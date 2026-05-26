#pragma once
#include "MigratorBase.h"
#include <vector>

namespace IceClean::Core::Migrator {

// 开发工具缓存迁移项信息
struct DevCacheInfo {
    std::wstring name;          // 如 "npm cache", "pip cache"
    std::wstring sourcePath;    // 源路径
    uint64_t size = 0;          // 大小
    bool canMigrate = true;     // 是否可迁移
};

class DevCacheMigrator : public MigratorBase {
public:
    std::wstring GetName() const override { return L"开发工具缓存迁移"; }
    Models::MigrationType GetMigrationType() const override { return Models::MigrationType::DevCache; }
    std::vector<Models::MigrationItem> Detect() override;
    Models::MigrationResult Migrate(const std::vector<Models::MigrationItem>& items,
                                     const std::wstring& targetDrive,
                                     std::function<void(const Models::MigrationProgress&)> progressCb = nullptr) override;

private:
    // 获取所有开发工具缓存路径
    std::vector<DevCacheInfo> GetDevCachePaths() const;

    // 计算目录大小
    uint64_t CalculateDirectorySize(const std::wstring& path) const;
};

} // namespace IceClean::Core::Migrator
