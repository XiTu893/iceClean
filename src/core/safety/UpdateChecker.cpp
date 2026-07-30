#include "UpdateChecker.h"
#include <spdlog/spdlog.h>

// 版本号定义（与 resource.h 保持同步）
#ifndef VERSION_MAJOR
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0
#define VERSION_BUILD 0
#define APP_VERSION L"1.0.0.0"
#endif
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <windows.h>
#include <winhttp.h>
#include <shlobj.h>

#pragma comment(lib, "winhttp.lib")

namespace IceClean::Core::Safety {

using json = nlohmann::json;

// ── 构造与单例 ──

UpdateChecker::UpdateChecker() {
    LoadSettings();
}

UpdateChecker& UpdateChecker::Instance() {
    static UpdateChecker instance;
    return instance;
}

// ── 异步检查 ──

void UpdateChecker::CheckForUpdate(std::function<void(const Models::UpdateCheckResult&)> callback) {
    std::thread([this, callback]() {
        auto result = CheckForUpdateSync();

        if (callback) {
            // 使用 PostMessage 确保 callback 在主线程执行
            // 这里简单地在后台线程保存结果，由调用方决定如何通知 UI
            callback(result);
        }
    }).detach();
}

// ── 同步检查 ──

Models::UpdateCheckResult UpdateChecker::CheckForUpdateSync() {
    std::lock_guard<std::mutex> lock(m_mutex);

    Models::UpdateCheckResult result;
    result.currentVersion = GetCurrentVersion();

    spdlog::info("正在检查更新...");

    // 使用 WinHTTP 发送 GET 请求到 GitHub Releases API
    std::string response;
    bool success = false;

    // 初始化 WinHTTP
    HINTERNET hSession = WinHttpOpen(
        L"IceClean/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (hSession) {
        HINTERNET hConnect = WinHttpConnect(
            hSession,
            L"api.github.com",
            INTERNET_DEFAULT_HTTPS_PORT,
            0);

        if (hConnect) {
            HINTERNET hRequest = WinHttpOpenRequest(
                hConnect,
                L"GET",
                L"/repos/XiTu893/iceClean/releases/latest",
                NULL,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                WINHTTP_FLAG_SECURE);

            if (hRequest) {
                // 设置超时（5秒连接，10秒接收）
                DWORD connectTimeout = 5000;
                DWORD receiveTimeout = 10000;
                WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT,
                    &connectTimeout, sizeof(connectTimeout));
                WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT,
                    &receiveTimeout, sizeof(receiveTimeout));

                // 添加 User-Agent 头（GitHub API 要求）
                LPCWSTR headers = L"User-Agent: IceClean-UpdateChecker\r\n";
                WinHttpAddRequestHeaders(hRequest, headers, -1, WINHTTP_ADDREQ_FLAG_ADD);

                BOOL bResults = WinHttpSendRequest(
                    hRequest,
                    WINHTTP_NO_ADDITIONAL_HEADERS,
                    0,
                    WINHTTP_NO_REQUEST_DATA,
                    0,
                    0,
                    0);

                if (bResults) {
                    bResults = WinHttpReceiveResponse(hRequest, NULL);

                    if (bResults) {
                        DWORD statusCode = 0;
                        DWORD statusCodeSize = sizeof(statusCode);
                        WinHttpQueryHeaders(hRequest,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX,
                            &statusCode,
                            &statusCodeSize,
                            WINHTTP_NO_HEADER_INDEX);

                        if (statusCode == 200) {
                            // 读取响应体
                            DWORD dwSize = 0;
                            do {
                                DWORD dwDownloaded = 0;
                                WinHttpQueryDataAvailable(hRequest, &dwSize);
                                if (dwSize == 0) break;

                                std::vector<char> buffer(dwSize + 1);
                                WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);
                                buffer[dwDownloaded] = '\0';
                                response.append(buffer.data(), dwDownloaded);
                            } while (dwSize > 0);

                            success = true;
                        } else {
                            result.networkError = true;
                            result.errorMessage = L"服务器返回错误: " + std::to_wstring(statusCode);
                        }
                    }
                }

                WinHttpCloseHandle(hRequest);
            }

            WinHttpCloseHandle(hConnect);
        }

        WinHttpCloseHandle(hSession);
    }

    if (!success && response.empty()) {
        if (result.errorMessage.empty()) {
            result.networkError = true;
            result.errorMessage = L"无法连接到更新服务器";
        }
        spdlog::warn("更新检查失败: {}", std::string(result.errorMessage.begin(), result.errorMessage.end()));
        return result;
    }

    // 解析响应
    result = ParseReleaseResponse(response);
    result.currentVersion = GetCurrentVersion();

    // 更新检查时间
    m_lastCheckTime = std::chrono::system_clock::now();
    SaveSettings();

    // 判断是否有更新
    if (result.latestVersion.IsNewerThan(result.currentVersion)) {
        // 检查是否已跳过该版本
        if (IsVersionSkipped(result.latestVersion.version)) {
            result.hasUpdate = false;
        } else {
            result.hasUpdate = true;
            spdlog::info("发现新版本: {}", std::string(result.latestVersion.version.begin(), result.latestVersion.version.end()));
        }
    } else {
        result.hasUpdate = false;
        spdlog::info("当前已是最新版本");
    }

    return result;
}

// ── 解析 GitHub Release 响应 ──

Models::UpdateCheckResult UpdateChecker::ParseReleaseResponse(const std::string& jsonResponse) const {
    Models::UpdateCheckResult result;

    try {
        auto j = json::parse(jsonResponse);

        // 提取 tag_name（如 "v1.2.3" 或 "1.2.3"）
        std::string tagName = j.value("tag_name", "");
        std::wstring versionTag(tagName.begin(), tagName.end());

        // 移除 "v" 前缀
        if (!versionTag.empty() && versionTag[0] == L'v') {
            versionTag = versionTag.substr(1);
        }

        result.latestVersion = ParseVersionString(versionTag);
        result.latestVersion.version = versionTag;

        // 提取发布说明
        std::string body = j.value("body", "");
        result.latestVersion.releaseNotes = std::wstring(body.begin(), body.end());

        // 提取下载链接（第一个 .exe 资源）
        if (j.contains("assets") && j["assets"].is_array()) {
            for (const auto& asset : j["assets"]) {
                std::string name = asset.value("name", "");
                if (name.size() >= 4 && name.substr(name.size() - 4) == ".exe") {
                    std::string url = asset.value("browser_download_url", "");
                    result.latestVersion.downloadUrl = std::wstring(url.begin(), url.end());
                    break;
                }
            }
        }

        // 提取发布时间
        std::string publishedAt = j.value("published_at", "");
        if (!publishedAt.empty()) {
            std::tm tm = {};
            std::istringstream ss(publishedAt);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
            if (!ss.fail()) {
                auto time_t_val = std::mktime(&tm);
                result.latestVersion.publishTime = std::chrono::system_clock::from_time_t(time_t_val);
            }
        }
    }
    catch (const json::exception& e) {
        result.errorMessage = L"解析更新信息失败";
        spdlog::error("解析 GitHub Release JSON 失败: {}", e.what());
    }

    return result;
}

// ── 解析版本字符串 ──

Models::VersionInfo UpdateChecker::ParseVersionString(const std::wstring& version) const {
    Models::VersionInfo info;

    // 解析 "1.2.3.4" 格式
    int parts[4] = {0, 0, 0, 0};
    int partIndex = 0;
    std::wstring current;

    for (wchar_t c : version) {
        if (c == L'.') {
            if (partIndex < 4 && !current.empty()) {
                parts[partIndex] = std::stoi(current);
                partIndex++;
            }
            current.clear();
        } else if (c >= L'0' && c <= L'9') {
            current += c;
        }
    }
    if (partIndex < 4 && !current.empty()) {
        parts[partIndex] = std::stoi(current);
    }

    info.major = parts[0];
    info.minor = parts[1];
    info.patch = parts[2];
    info.build = parts[3];
    info.version = version;

    return info;
}

// ── 获取当前版本 ──

Models::VersionInfo UpdateChecker::GetCurrentVersion() const {
    Models::VersionInfo info;
    info.major = VERSION_MAJOR;
    info.minor = VERSION_MINOR;
    info.patch = VERSION_PATCH;
    info.build = VERSION_BUILD;
    info.version = APP_VERSION;
    return info;
}

// ── 设置管理 ──

const Models::AutoUpdateSettings& UpdateChecker::GetSettings() const {
    return m_settings;
}

void UpdateChecker::SetSettings(const Models::AutoUpdateSettings& settings) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_settings = settings;
    SaveSettings();
}

bool UpdateChecker::ShouldCheckUpdate() const {
    if (!m_settings.autoCheckEnabled) return false;

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - m_lastCheckTime);
    return elapsed.count() >= m_settings.checkIntervalHours;
}

std::chrono::system_clock::time_point UpdateChecker::GetLastCheckTime() const {
    return m_lastCheckTime;
}

void UpdateChecker::SkipVersion(const std::wstring& version) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_skippedVersion = version;
    SaveSettings();
}

bool UpdateChecker::IsVersionSkipped(const std::wstring& version) const {
    return m_skippedVersion == version;
}

// ── 配置持久化 ──

std::wstring UpdateChecker::GetConfigFilePath() {
    wchar_t appDataPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return std::wstring(appDataPath) + L"\\IceClean\\" + kConfigFileName;
    }
    return kConfigFileName;
}

void UpdateChecker::LoadSettings() {
    auto path = GetConfigFilePath();
    try {
        std::ifstream file(path);
        if (!file.is_open()) return;

        json j;
        file >> j;

        if (j.contains("autoCheckEnabled")) {
            m_settings.autoCheckEnabled = j["autoCheckEnabled"].get<bool>();
        }
        if (j.contains("checkIntervalHours")) {
            m_settings.checkIntervalHours = j["checkIntervalHours"].get<int>();
        }
        if (j.contains("autoDownloadEnabled")) {
            m_settings.autoDownloadEnabled = j["autoDownloadEnabled"].get<bool>();
        }
        if (j.contains("notifyOnUpdate")) {
            m_settings.notifyOnUpdate = j["notifyOnUpdate"].get<bool>();
        }
        if (j.contains("lastCheckTime")) {
            auto timestamp = j["lastCheckTime"].get<int64_t>();
            m_lastCheckTime = std::chrono::system_clock::from_time_t(timestamp);
        }
        if (j.contains("skippedVersion")) {
            auto sv = j["skippedVersion"].get<std::string>();
            m_skippedVersion = std::wstring(sv.begin(), sv.end());
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("加载更新配置失败: {}", e.what());
    }
}

void UpdateChecker::SaveSettings() {
    auto path = GetConfigFilePath();

    // 确保目录存在
    auto dir = path.substr(0, path.find_last_of(L'\\'));
    CreateDirectoryW(dir.c_str(), NULL);

    try {
        json j;
        j["autoCheckEnabled"] = m_settings.autoCheckEnabled;
        j["checkIntervalHours"] = m_settings.checkIntervalHours;
        j["autoDownloadEnabled"] = m_settings.autoDownloadEnabled;
        j["notifyOnUpdate"] = m_settings.notifyOnUpdate;
        j["lastCheckTime"] = std::chrono::system_clock::to_time_t(m_lastCheckTime);
        j["skippedVersion"] = std::string(m_skippedVersion.begin(), m_skippedVersion.end());

        std::ofstream file(path);
        if (file.is_open()) {
            file << j.dump(2);
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("保存更新配置失败: {}", e.what());
    }
}

} // namespace IceClean::Core::Safety
