#pragma once
#include "models/UpdateInfo.h"
#include <string>
#include <functional>
#include <chrono>
#include <mutex>

namespace IceClean::Core::Safety {

// 自动更新检查器
// 负责检查 GitHub Releases 是否有新版本
class UpdateChecker {
public:
    // 获取单例
    static UpdateChecker& Instance();

    // 检查更新（异步）
    // callback 在主线程调用
    void CheckForUpdate(std::function<void(const Models::UpdateCheckResult&)> callback = nullptr);

    // 同步检查更新（阻塞调用）
    Models::UpdateCheckResult CheckForUpdateSync();

    // 获取当前版本
    Models::VersionInfo GetCurrentVersion() const;

    // 获取/设置自动更新配置
    const Models::AutoUpdateSettings& GetSettings() const;
    void SetSettings(const Models::AutoUpdateSettings& settings);

    // 加载/保存配置（JSON）
    void LoadSettings();
    void SaveSettings();

    // 是否应该检查更新（根据时间间隔）
    bool ShouldCheckUpdate() const;

    // 获取上次检查时间
    std::chrono::system_clock::time_point GetLastCheckTime() const;

    // 跳过当前版本
    void SkipVersion(const std::wstring& version);
    bool IsVersionSkipped(const std::wstring& version) const;

private:
    UpdateChecker();
    ~UpdateChecker() = default;

    UpdateChecker(const UpdateChecker&) = delete;
    UpdateChecker& operator=(const UpdateChecker&) = delete;

    // 解析 GitHub Release API 响应
    Models::UpdateCheckResult ParseReleaseResponse(const std::string& jsonResponse) const;

    // 解析版本字符串 "1.2.3.4" → VersionInfo
    Models::VersionInfo ParseVersionString(const std::wstring& version) const;

    // 获取配置文件路径
    static std::wstring GetConfigFilePath();

    // 获取 GitHub Releases API URL
    static std::wstring GetReleasesApiUrl();

    Models::AutoUpdateSettings m_settings;
    std::chrono::system_clock::time_point m_lastCheckTime;
    std::wstring m_skippedVersion;
    mutable std::mutex m_mutex;

    static constexpr const wchar_t* kReleasesUrl = L"https://api.github.com/repos/XiTu893/iceClean/releases/latest";
    static constexpr const wchar_t* kConfigFileName = L"update_config.json";
};

} // namespace IceClean::Core::Safety
