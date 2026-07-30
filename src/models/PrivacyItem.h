#pragma once
#include <string>
#include <vector>

namespace IceClean::Models {

enum class PrivacyCategory {
    Telemetry,
    Cortana,
    Advertising,
    Location,
    Camera,
    Microphone,
    Notifications,
    Speech,
    Search,
    Updates,
    OneDrive,
    ActivityHistory,
    DeliveryOptimization,
    GameDVR,
    AppPermissions
};

enum class SafetyLevel {
    Safe,
    Caution,
    Warning
};

struct PrivacyItem {
    std::wstring id;
    std::wstring name;
    std::wstring description;
    PrivacyCategory category;
    SafetyLevel safetyLevel;
    std::wstring registryPath;
    std::wstring valueName;
    DWORD currentValue = 0;
    DWORD recommendedValue = 0;
    bool isApplied = false;
    bool requiresRestart = false;
    std::wstring restoreHint;
};

struct PrivacyPreset {
    std::wstring name;
    std::wstring description;
    std::vector<std::wstring> itemIds;
};

struct PrivacyResult {
    int succeeded = 0;
    int failed = 0;
    std::vector<std::wstring> failedItems;
};

} // namespace IceClean::Models
