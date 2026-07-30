#include "StartupProtector.h"
#include "core/optimizer/StartupOptimizer.h"
#include "utils/RegistryUtil.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

namespace IceClean::Core::Safety {

// ── 建立基线 ──

void StartupProtector::BuildBaseline() {
    auto items = GetCurrentStartupItems();
    SaveBaseline(items);
}

// ── 检测变更 ──

std::vector<StartupChangeRecord> StartupProtector::DetectChanges() {
    std::vector<StartupChangeRecord> changes;

    auto baseline = LoadBaseline();
    auto current = GetCurrentStartupItems();

    // 检测新增项
    for (const auto& cur : current) {
        bool found = false;
        for (const auto& base : baseline) {
            if (base.name == cur.name && base.path == cur.path) {
                found = true;
                // 检查是否状态变更
                if (base.isEnabled != cur.isEnabled) {
                    StartupChangeRecord record;
                    record.itemName = cur.name;
                    record.itemPath = cur.path;
                    record.changeType = cur.isEnabled
                        ? StartupChangeRecord::ChangeType::Enabled
                        : StartupChangeRecord::ChangeType::Disabled;
                    SYSTEMTIME st;
                    GetSystemTime(&st);
                    SystemTimeToFileTime(&st, &record.timestamp);
                    changes.push_back(record);
                }
                break;
            }
        }
        if (!found) {
            StartupChangeRecord record;
            record.itemName = cur.name;
            record.itemPath = cur.path;
            record.changeType = StartupChangeRecord::ChangeType::Added;
            SYSTEMTIME st;
            GetSystemTime(&st);
            SystemTimeToFileTime(&st, &record.timestamp);
            changes.push_back(record);
        }
    }

    // 检测删除项
    for (const auto& base : baseline) {
        bool found = false;
        for (const auto& cur : current) {
            if (base.name == cur.name && base.path == cur.path) {
                found = true;
                break;
            }
        }
        if (!found) {
            StartupChangeRecord record;
            record.itemName = base.name;
            record.itemPath = base.path;
            record.changeType = StartupChangeRecord::ChangeType::Removed;
            SYSTEMTIME st;
            GetSystemTime(&st);
            SystemTimeToFileTime(&st, &record.timestamp);
            changes.push_back(record);
        }
    }

    return changes;
}

// ── 获取当前启动项 ──

std::vector<IceClean::Models::StartupItem> StartupProtector::GetCurrentStartupItems() {
    IceClean::Core::Optimizer::StartupOptimizer optimizer;
    return optimizer.GetStartupItems();
}

// ── 启动锁定 ──

bool StartupProtector::IsStartupLocked() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"SOFTWARE\\IceClean\\StartupProtector",
                       0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD size = sizeof(value);
        if (RegQueryValueExW(hKey, L"Locked", nullptr, nullptr, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return value != 0;
        }
        RegCloseKey(hKey);
    }
    return false;
}

void StartupProtector::SetStartupLocked(bool locked) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                         L"SOFTWARE\\IceClean\\StartupProtector",
                         0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD value = locked ? 1 : 0;
        RegSetValueExW(hKey, L"Locked", 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
        RegCloseKey(hKey);
    }
}

// ── 阻止启动项 ──

bool StartupProtector::BlockStartupItem(const std::wstring& name, const std::wstring& path) {
    // 通过在注册表策略中添加来阻止
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                         L"SOFTWARE\\IceClean\\StartupProtector\\Blocked",
                         0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, name.c_str(), 0, REG_SZ,
                        (const BYTE*)path.c_str(),
                        static_cast<DWORD>((path.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return true;
    }
    return false;
}

bool StartupProtector::UnblockStartupItem(const std::wstring& name) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"SOFTWARE\\IceClean\\StartupProtector\\Blocked",
                       0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        bool ok = RegDeleteValueW(hKey, name.c_str()) == ERROR_SUCCESS;
        RegCloseKey(hKey);
        return ok;
    }
    return false;
}

std::vector<std::pair<std::wstring, std::wstring>> StartupProtector::GetBlockedItems() {
    std::vector<std::pair<std::wstring, std::wstring>> items;

    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"SOFTWARE\\IceClean\\StartupProtector\\Blocked",
                       0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        auto names = IceClean::Utils::RegistryUtil::EnumValues(hKey, L"");
        for (const auto& name : names) {
            auto path = IceClean::Utils::RegistryUtil::ReadStringValue(hKey, L"", name);
            items.push_back({name, path});
        }
        RegCloseKey(hKey);
    }

    return items;
}

// ── 基线保存/读取 ──

void StartupProtector::SaveBaseline(const std::vector<IceClean::Models::StartupItem>& items) {
    auto configPath = GetConfigFilePath();
    nlohmann::json j = nlohmann::json::array();

    for (const auto& item : items) {
        std::string nameStr, pathStr;
        nameStr.reserve(item.name.size());
        pathStr.reserve(item.path.size());
        for (wchar_t wc : item.name) nameStr += static_cast<char>(wc);
        for (wchar_t wc : item.path) pathStr += static_cast<char>(wc);

        j.push_back({
            {"name", nameStr},
            {"path", pathStr},
            {"isEnabled", item.isEnabled}
        });
    }

    std::ofstream ofs(configPath);
    if (ofs.is_open()) {
        ofs << j.dump(2);
    }
}

std::vector<IceClean::Models::StartupItem> StartupProtector::LoadBaseline() {
    std::vector<IceClean::Models::StartupItem> items;
    auto configPath = GetConfigFilePath();

    std::ifstream ifs(configPath);
    if (!ifs.is_open()) return items;

    try {
        nlohmann::json j;
        ifs >> j;

        for (const auto& entry : j) {
            IceClean::Models::StartupItem item;
            std::string name = entry.value("name", "");
            std::string path = entry.value("path", "");
            item.name = std::wstring(name.begin(), name.end());
            item.path = std::wstring(path.begin(), path.end());
            item.isEnabled = entry.value("isEnabled", true);
            items.push_back(item);
        }
    } catch (...) {}

    return items;
}

std::wstring StartupProtector::GetConfigFilePath() {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring dir(path);
    size_t lastSlash = dir.rfind(L'\\');
    if (lastSlash != std::wstring::npos) {
        dir = dir.substr(0, lastSlash);
    }
    return dir + L"\\startup_baseline.json";
}

std::wstring StartupProtector::GetChangeTypeName(StartupChangeRecord::ChangeType type) {
    switch (type) {
    case StartupChangeRecord::ChangeType::Added:    return L"新增";
    case StartupChangeRecord::ChangeType::Removed:  return L"删除";
    case StartupChangeRecord::ChangeType::Modified: return L"修改";
    case StartupChangeRecord::ChangeType::Enabled:  return L"启用";
    case StartupChangeRecord::ChangeType::Disabled: return L"禁用";
    default:                                        return L"未知";
    }
}

} // namespace IceClean::Core::Safety
