#pragma once
#include <string>
#include <functional>
#include <chrono>

namespace IceClean::Models {

// 版本信息
struct VersionInfo {
    int major = 0;
    int minor = 0;
    int patch = 0;
    int build = 0;
    std::wstring releaseNotes;       // 更新说明
    std::wstring downloadUrl;        // 下载链接
    std::wstring version;            // 版本字符串 "1.2.3.4"
    std::chrono::system_clock::time_point publishTime;

    // 比较版本号：this > other 返回正数
    int Compare(const VersionInfo& other) const {
        if (major != other.major) return major - other.major;
        if (minor != other.minor) return minor - other.minor;
        if (patch != other.patch) return patch - other.patch;
        return build - other.build;
    }

    bool IsNewerThan(const VersionInfo& other) const {
        return Compare(other) > 0;
    }
};

// 更新检查结果
struct UpdateCheckResult {
    bool hasUpdate = false;          // 是否有更新
    VersionInfo latestVersion;       // 最新版本信息
    VersionInfo currentVersion;      // 当前版本
    std::wstring errorMessage;       // 错误信息
    bool networkError = false;       // 是否网络错误
};

// 自动更新设置
struct AutoUpdateSettings {
    bool autoCheckEnabled = true;    // 启用自动检查
    int checkIntervalHours = 24;     // 检查间隔（小时）
    bool autoDownloadEnabled = false; // 自动下载
    bool notifyOnUpdate = true;      // 有更新时通知
};

} // namespace IceClean::Models
