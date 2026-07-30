#pragma once
#include <string>
#include <vector>

namespace IceClean::Models {

enum class DebloatCategory {
    AppxPackage,
    SystemComponent,
    Telemetry,
    Service,
    ScheduledTask,
    ContextMenu,
    RegistryTweak
};

enum class DebloatAction {
    Remove,
    Disable,
    Enable,
    Modify
};

struct DebloatItem {
    std::wstring id;
    std::wstring name;
    std::wstring description;
    DebloatCategory category;
    DebloatAction action;
    std::wstring target;
    std::wstring value;
    bool isRecommended = true;
    bool isSelected = false;
    bool requiresRestart = false;
    std::wstring restoreHint;
};

struct DebloatPreset {
    std::wstring name;
    std::wstring description;
    std::vector<std::wstring> itemIds;
};

struct DebloatResult {
    bool success = false;
    int succeeded = 0;
    int failed = 0;
    std::vector<std::wstring> failedItems;
    std::wstring errorMessage;
};

} // namespace IceClean::Models
