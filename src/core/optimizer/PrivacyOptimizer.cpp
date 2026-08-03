#include "PrivacyOptimizer.h"
#include "utils/RegistryUtil.h"
#include <spdlog/spdlog.h>

namespace IceClean::Core::Optimizer {

using namespace IceClean::Models;
using namespace IceClean::Utils;

PrivacyOptimizer::PrivacyOptimizer() {
    InitializeItems();
}

void PrivacyOptimizer::InitializeItems() {
    // ── 遥测 ──
    m_items.push_back({ L"privacy-telemetry-allow", L"允许遥测收集", L"控制诊断数据发送给 Microsoft", PrivacyCategory::Telemetry, SafetyLevel::Safe, L"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"AllowTelemetry", 1, 0, false, false, L"值设为 0 禁用" });
    m_items.push_back({ L"privacy-telemetry-optin", L"禁用遥测优化通知", L"关闭遥测首次设置弹窗", PrivacyCategory::Telemetry, SafetyLevel::Safe, L"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"DisableTelemetryOptin", 0, 1, false, false, L"值设为 1" });
    m_items.push_back({ L"privacy-telemetry-census", L"Census 数据收集", L"禁用硬件/软件清单数据收集", PrivacyCategory::Telemetry, SafetyLevel::Safe, L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection", L"AllowCensus", 1, 0, false, false, L"" });
    m_items.push_back({ L"privacy-telemetry-connect", L"Connected User Experiences", L"禁用用户体验改善计划收集", PrivacyCategory::Telemetry, SafetyLevel::Safe, L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection", L"AllowConnectedUser", 1, 0, false, false, L"" });

    // ── Cortana ──
    m_items.push_back({ L"privacy-cortana", L"禁用 Cortana", L"关闭 Cortana 语音助手", PrivacyCategory::Cortana, SafetyLevel::Safe, L"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"AllowCortana", 1, 0, false, true, L"需重启生效" });
    m_items.push_back({ L"privacy-cortana-search", L"禁用搜索云内容", L"禁止搜索在云端获取结果", PrivacyCategory::Cortana, SafetyLevel::Safe, L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search", L"BingSearchEnabled", 1, 0, false, false, L"" });
    m_items.push_back({ L"privacy-cortana-history", L"禁用搜索历史", L"关闭本地搜索历史记录", PrivacyCategory::Cortana, SafetyLevel::Safe, L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search", L"HistoryStoreEnabled", 1, 0, false, false, L"" });

    // ── 广告 ──
    m_items.push_back({ L"privacy-ads-id", L"禁用广告 ID", L"关闭应用使用广告 ID 跟踪", PrivacyCategory::Advertising, SafetyLevel::Safe, L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AdvertisingInfo", L"Enabled", 1, 0, false, false, L"" });
    m_items.push_back({ L"privacy-ads-smartscreen", L"禁用 SmartScreen", L"关闭 Microsoft Defender SmartScreen", PrivacyCategory::Advertising, SafetyLevel::Caution, L"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System", L"EnableSmartScreen", 1, 0, false, false, L"可能有安全性影响" });
    m_items.push_back({ L"privacy-ads-tailored", L"禁用定制化广告", L"关闭基于用户的定制广告体验", PrivacyCategory::Advertising, SafetyLevel::Safe, L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Privacy", L"TailoredExperiencesWithDiagnosticDataEnabled", 1, 0, false, false, L"" });
    m_items.push_back({ L"privacy-ads-appsuggest", L"禁用应用建议", L"关闭在开始菜单中显示应用建议", PrivacyCategory::Advertising, SafetyLevel::Safe, L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\ContentDeliveryManager", L"SubscribedContent-338389Enabled", 1, 0, false, false, L"" });

    // ── 位置 ──
    m_items.push_back({ L"privacy-location", L"禁用位置服务", L"关闭设备位置访问权限", PrivacyCategory::Location, SafetyLevel::Caution, L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\location", L"Value", 1, 0, false, false, L"部分应用可能无法使用位置功能" });

    // ── 摄像头 ──
    m_items.push_back({ L"privacy-camera", L"禁用摄像头访问", L"关闭应用的摄像头访问权限", PrivacyCategory::Camera, SafetyLevel::Caution, L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\webcam", L"Value", 1, 0, false, false, L"视频通话类应用可能无法使用" });

    // ── 麦克风 ──
    m_items.push_back({ L"privacy-mic", L"禁用麦克风访问", L"关闭应用的麦克风访问权限", PrivacyCategory::Microphone, SafetyLevel::Caution, L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\microphone", L"Value", 1, 0, false, false, L"语音输入可能受影响" });

    // ── 活动历史 ──
    m_items.push_back({ L"privacy-activity-history", L"禁用活动历史", L"关闭活动历史记录同步到 Microsoft", PrivacyCategory::ActivityHistory, SafetyLevel::Safe, L"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\System", L"EnableActivityFeed", 1, 0, false, false, L"" });

    // ── OneDrive ──
    m_items.push_back({ L"privacy-onedrive-sync", L"禁用 OneDrive 同步", L"关闭 OneDrive 文件同步", PrivacyCategory::OneDrive, SafetyLevel::Caution, L"HKCU\\SOFTWARE\\Microsoft\\OneDrive", L"DisableFileSyncNGSC", 0, 1, false, true, L"需重启后生效" });

    // ── 传递优化 ──
    m_items.push_back({ L"privacy-delivery-opt", L"禁用传递优化", L"关闭 P2P 更新分发功能", PrivacyCategory::DeliveryOptimization, SafetyLevel::Safe, L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\DeliveryOptimization\\Config", L"DODownloadMode", 1, 0, false, false, L"" });

    // ── 游戏 DVR ──
    m_items.push_back({ L"privacy-gamedvr", L"禁用游戏 DVR", L"关闭后台游戏录制、截图功能", PrivacyCategory::GameDVR, SafetyLevel::Caution, L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\GameDVR", L"AppCaptureEnabled", 1, 0, false, false, L"禁用后台录制和截图" });

    // ── 通知 ──
    m_items.push_back({ L"privacy-notification-suggestions", L"禁用通知建议", L"关闭通知区域的建议/推广通知", PrivacyCategory::Notifications, SafetyLevel::Safe, L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Notifications\\Settings", L"NCSI_Enabled", 1, 0, false, false, L"" });
}

std::vector<PrivacyItem> PrivacyOptimizer::GetItems() const {
    return m_items;
}

std::vector<PrivacyPreset> PrivacyOptimizer::GetPresets() const {
    std::vector<PrivacyPreset> presets;

    presets.push_back({
        L"高隐私 (Maximum Privacy)",
        L"尽可能禁用所有隐私数据收集，大多数应用功能不受影响",
        {
            L"privacy-telemetry-allow", L"privacy-telemetry-optin", L"privacy-telemetry-census",
            L"privacy-telemetry-connect", L"privacy-cortana", L"privacy-cortana-search",
            L"privacy-cortana-history", L"privacy-ads-id", L"privacy-ads-tailored",
            L"privacy-ads-appsuggest", L"privacy-activity-history", L"privacy-delivery-opt",
            L"privacy-notification-suggestions", L"privacy-gamedvr"
        }
    });

    presets.push_back({
        L"终极隐私 (Ultimate)",
        L"禁用所有隐私设置，包括摄像头/麦克风/位置等，部分应用功能可能受限",
        {
            L"privacy-telemetry-allow", L"privacy-telemetry-optin", L"privacy-telemetry-census",
            L"privacy-telemetry-connect", L"privacy-cortana", L"privacy-cortana-search",
            L"privacy-cortana-history", L"privacy-ads-id", L"privacy-ads-smartscreen",
            L"privacy-ads-tailored", L"privacy-ads-appsuggest",
            L"privacy-location", L"privacy-camera", L"privacy-mic",
            L"privacy-activity-history", L"privacy-onedrive-sync",
            L"privacy-delivery-opt", L"privacy-gamedvr",
            L"privacy-notification-suggestions"
        }
    });

    return presets;
}

bool PrivacyOptimizer::ApplyItem(const PrivacyItem& item) {
    return ApplyRegistryDword(item.registryPath, item.valueName, item.recommendedValue);
}

PrivacyResult PrivacyOptimizer::ApplyItems(
    const std::vector<PrivacyItem>& items,
    std::function<void(int, int)> progressCallback,
    const std::atomic<bool>* cancelFlag)
{
    PrivacyResult result;
    int total = static_cast<int>(items.size());

    for (int i = 0; i < total; ++i) {
        if (cancelFlag && *cancelFlag) break;
        if (progressCallback) progressCallback(i, total);

        if (ApplyItem(items[i])) {
            result.succeeded++;
        } else {
            result.failed++;
            result.failedItems.push_back(items[i].name);
        }
    }

    return result;
}

bool PrivacyOptimizer::RevertItem(const PrivacyItem& item) {
    return ApplyRegistryDword(item.registryPath, item.valueName, item.currentValue);
}

void PrivacyOptimizer::RefreshCurrentValues() {
    for (auto& item : m_items) {
        std::wstring path = item.registryPath;
        HKEY rootKey = HKEY_LOCAL_MACHINE;
        if (path.find(L"HKCU\\") == 0) {
            rootKey = HKEY_CURRENT_USER;
            path = path.substr(5);
        } else if (path.find(L"HKLM\\") == 0) {
            rootKey = HKEY_LOCAL_MACHINE;
            path = path.substr(5);
        }

        DWORD current = RegistryUtil::ReadDwordValue(rootKey, path, item.valueName);
        item.currentValue = current;
        item.isApplied = (current == item.recommendedValue);
    }
}

bool PrivacyOptimizer::ApplyRegistryDword(const std::wstring& fullPath, const std::wstring& valueName, DWORD value) {
    std::wstring path = fullPath;
    HKEY rootKey = HKEY_LOCAL_MACHINE;
    if (path.find(L"HKCU\\") == 0) {
        rootKey = HKEY_CURRENT_USER;
        path = path.substr(5);
    } else if (path.find(L"HKLM\\") == 0) {
        rootKey = HKEY_LOCAL_MACHINE;
        path = path.substr(5);
    }

    HKEY hKey;
    if (RegCreateKeyExW(rootKey, path.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        spdlog::warn("PrivacyOptimizer: 无法创建/打开注册表键: {}",
                     std::string(fullPath.begin(), fullPath.end()));
        return false;
    }

    bool ok = RegSetValueExW(hKey, valueName.c_str(), 0, REG_DWORD,
                              (const BYTE*)&value, sizeof(value)) == ERROR_SUCCESS;
    if (!ok) {
        spdlog::warn("PrivacyOptimizer: 设置值失败: {}\\{}",
                     std::string(fullPath.begin(), fullPath.end()),
                     std::string(valueName.begin(), valueName.end()));
    }

    RegCloseKey(hKey);
    return ok;
}

bool PrivacyOptimizer::ApplyRegistryString(const std::wstring& fullPath, const std::wstring& valueName, const std::wstring& value) {
    std::wstring path = fullPath;
    HKEY rootKey = HKEY_LOCAL_MACHINE;
    if (path.find(L"HKCU\\") == 0) {
        rootKey = HKEY_CURRENT_USER;
        path = path.substr(5);
    } else if (path.find(L"HKLM\\") == 0) {
        rootKey = HKEY_LOCAL_MACHINE;
        path = path.substr(5);
    }

    HKEY hKey;
    if (RegCreateKeyExW(rootKey, path.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    bool ok = RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ,
                              (const BYTE*)value.c_str(),
                              static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return ok;
}

} // namespace IceClean::Core::Optimizer
