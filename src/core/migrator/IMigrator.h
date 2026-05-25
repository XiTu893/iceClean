#pragma once
#include <string>
#include <vector>
#include <functional>
#include "models/MigrationItem.h"

namespace IceClean::Core::Migrator {

class IMigrator {
public:
    virtual ~IMigrator() = default;
    virtual std::wstring GetName() const = 0;
    virtual Models::MigrationType GetMigrationType() const = 0;
    virtual std::vector<Models::MigrationItem> Detect() = 0;
    virtual Models::MigrationResult Migrate(const std::vector<Models::MigrationItem>& items,
                                             const std::wstring& targetDrive,
                                             std::function<void(const Models::MigrationProgress&)> progressCallback = nullptr) = 0;
};

} // namespace IceClean::Core::Migrator
