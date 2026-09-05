#pragma once
#include "MigratorBase.h"
#include <vector>
#include <string>
#include <atomic>

namespace IceClean::Core::Migrator {

// 程序迁移器：扫描 C:\Program Files 和 C:\Program Files (x86) 下的应用
class ProgramMigrator : public MigratorBase {
public:
    using ProgressCallback = std::function<void(const std::wstring& path, int foundCount, uint64_t foundBytes)>;

    ProgramMigrator();

    std::wstring GetName() const override;
    Models::MigrationType GetMigrationType() const override;

    std::vector<Models::MigrationItem> Detect() override;
    std::vector<Models::MigrationItem> DetectWithCallback(ProgressCallback progressCallback);

    Models::MigrationResult Migrate(const std::vector<Models::MigrationItem>& items,
                                      const std::wstring& targetDrive,
                                      std::function<void(const Models::MigrationProgress&)> progressCb = nullptr) override;

    void Cancel();

private:
    struct ProgramRegInfo {
        std::wstring displayName;
        std::wstring installLocation;
        std::wstring publisher;
        std::wstring version;
        std::wstring uninstallString;
        bool is64Bit = false;
    };

    struct SafetyConfig {
        Models::ProgramSafetyLevel level;
        const wchar_t* reason;
    };

    std::vector<ProgramRegInfo> ScanRegistry();
    std::vector<ProgramRegInfo> ScanRegistryKey(HKEY rootKey, const std::wstring& subKey);

    uint64_t MeasureDirectorySize(const std::wstring& path,
                                  ProgressCallback& cb,
                                  int foundCount);

    SafetyConfig EvaluateSafety(const std::wstring& programName,
                                const std::wstring& installPath,
                                bool isSystemComponent);

    static bool IsSystemProgram(const std::wstring& name);
    static bool IsInProgramFiles(const std::wstring& path);
    static bool IsProcessRunning(const std::wstring& processName);

    static uint64_t GetFolderSizeSync(const std::wstring& path);

    std::atomic<bool> cancelled_{false};
};

} // namespace IceClean::Core::Migrator
