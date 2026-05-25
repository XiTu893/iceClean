#pragma once
#include "MigratorBase.h"
#include <vector>

namespace IceClean::Core::Migrator {

struct SteamGameInfo {
    std::wstring appId;
    std::wstring name;
    std::wstring installDir;
    uint64_t size;
    std::wstring libraryPath;
};

class SteamMigrator : public MigratorBase {
public:
    std::wstring GetName() const override;
    Models::MigrationType GetMigrationType() const override;
    std::vector<Models::MigrationItem> Detect() override;
    Models::MigrationResult Migrate(const std::vector<Models::MigrationItem>& items,
                                     const std::wstring& targetDrive,
                                     std::function<void(const Models::MigrationProgress&)> progressCallback = nullptr) override;

private:
    // 从注册表获取Steam安装路径
    std::wstring GetSteamInstallPath() const;

    // 解析libraryfolders.vdf获取所有Steam库路径
    std::vector<std::wstring> ParseLibraryFolders(const std::wstring& steamPath) const;

    // 解析appmanifest_*.acf文件获取游戏信息
    SteamGameInfo ParseAppManifest(const std::wstring& manifestPath, const std::wstring& libraryPath) const;

    // 从VDF行中提取带引号的字符串值
    std::wstring ExtractQuotedValue(const std::string& line, const std::string& key) const;

    // 检查Steam是否正在运行
    bool IsSteamRunning() const;
};

} // namespace IceClean::Core::Migrator
