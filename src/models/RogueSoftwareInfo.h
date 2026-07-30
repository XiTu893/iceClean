#pragma once
#include <string>
#include <vector>

namespace IceClean::Models {

struct RogueIndicator {
    bool hasProcesses = false;
    bool hasRegistry = false;
    bool hasFiles = false;
    bool hasServices = false;
    bool hasScheduledTasks = false;
    bool hasBrowserHijack = false;
};

struct RogueCleanupActions {
    bool killProcess = false;
    bool stopService = false;
    bool deleteRegistry = false;
    bool deleteFiles = false;
    bool restoreBrowser = false;
};

struct RogueSoftwareRule {
    std::wstring id;
    std::wstring name;
    std::vector<std::wstring> aliases;
    std::wstring category;
    std::wstring safetyLevel;
    RogueIndicator hasIndicator;
    RogueCleanupActions actions;
    std::wstring description;
};

struct RogueDetectionResult {
    std::wstring ruleId;
    std::wstring name;
    std::wstring category;
    std::wstring safetyLevel;
    std::wstring description;
    bool detected = false;
    std::vector<std::wstring> detectedProcesses;
    std::vector<std::wstring> detectedRegistry;
    std::vector<std::wstring> detectedFiles;
    std::vector<std::wstring> detectedServices;
    std::vector<std::wstring> detectedScheduledTasks;
    RogueCleanupActions actions;
    bool cleaned = false;
    std::wstring cleanMessage;
};

struct RogueSoftwareDB {
    int version = 0;
    std::wstring updatedAt;
    std::vector<RogueSoftwareRule> rules;
};

} // namespace IceClean::Models
