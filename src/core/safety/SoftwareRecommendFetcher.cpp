#include "SoftwareRecommendFetcher.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

namespace IceClean::Core::Safety {

using json = nlohmann::json;

// ── 单例 ──

SoftwareRecommendFetcher& SoftwareRecommendFetcher::Instance() {
    static SoftwareRecommendFetcher instance;
    return instance;
}

// ── 异步获取 ──

void SoftwareRecommendFetcher::FetchAsync(
    std::function<void(bool success, const Models::RecommendData& data)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fetching) {
        // 已在获取中，不重复请求
        if (callback) {
            Models::RecommendData empty;
            callback(false, empty);
        }
        return;
    }
    m_fetching = true;

    std::thread([this, callback]() {
        Models::RecommendData data;
        bool success = FetchSync(data);

        m_fetching = false;

        if (callback) {
            callback(success, data);
        }
    }).detach();
}

// ── 同步获取 ──

bool SoftwareRecommendFetcher::FetchSync(Models::RecommendData& outData) {
    spdlog::info("正在获取推荐软件数据...");

    std::string response;
    bool success = HttpGet(kGitHubServer, kGitHubPath, response);

    if (!success || response.empty()) {
        spdlog::warn("获取推荐软件数据失败");
        return false;
    }

    if (!ParseJson(response, outData)) {
        spdlog::warn("解析推荐软件数据失败");
        return false;
    }

    spdlog::info("推荐软件数据获取成功: {} 个分类, {} 个软件",
                 outData.categories.size(), outData.software.size());
    return true;
}

// ── HTTP GET ──

bool SoftwareRecommendFetcher::HttpGet(const std::wstring& server,
                                        const std::wstring& path,
                                        std::string& response) {
    HINTERNET hSession = WinHttpOpen(
        L"IceClean/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!hSession) {
        spdlog::error("WinHttpOpen 失败: {}", GetLastError());
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(
        hSession,
        server.c_str(),
        INTERNET_DEFAULT_HTTPS_PORT,
        0);

    if (!hConnect) {
        spdlog::error("WinHttpConnect 失败: {}", GetLastError());
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"GET",
        path.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!hRequest) {
        spdlog::error("WinHttpOpenRequest 失败: {}", GetLastError());
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    // 设置超时（5秒连接，10秒接收）
    DWORD connectTimeout = 5000;
    DWORD receiveTimeout = 10000;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT,
                     &connectTimeout, sizeof(connectTimeout));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT,
                     &receiveTimeout, sizeof(receiveTimeout));

    // 添加 User-Agent 头
    LPCWSTR headers = L"User-Agent: IceClean-SoftwareRecommend\r\n";
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
            } else {
                spdlog::warn("推荐软件数据获取: HTTP {}", statusCode);
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return false;
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return !response.empty();
}

// ── JSON 解析 ──

bool SoftwareRecommendFetcher::ParseJson(const std::string& jsonStr,
                                          Models::RecommendData& outData) {
    try {
        auto j = json::parse(jsonStr);

        outData.version = j.value("version", 0);

        // 解析 updated_at
        if (j.contains("updated_at")) {
            std::string updatedAt = j["updated_at"].get<std::string>();
            outData.updatedAt = std::wstring(updatedAt.begin(), updatedAt.end());
        }

        // 解析分类
        if (j.contains("categories") && j["categories"].is_array()) {
            for (const auto& catJ : j["categories"]) {
                Models::RecommendCategory cat;
                cat.id = std::wstring(catJ.value("id", "").begin(), catJ.value("id", "").end());
                cat.name = std::wstring(catJ.value("name", "").begin(), catJ.value("name", "").end());
                cat.icon = std::wstring(catJ.value("icon", "").begin(), catJ.value("icon", "").end());
                cat.sortOrder = catJ.value("sort_order", 0);
                outData.categories.push_back(std::move(cat));

                // 解析该分类下的软件
                if (catJ.contains("software") && catJ["software"].is_array()) {
                    for (const auto& swJ : catJ["software"]) {
                        Models::RecommendedSoftware sw;
                        sw.id = std::wstring(swJ.value("id", "").begin(), swJ.value("id", "").end());
                        sw.name = std::wstring(swJ.value("name", "").begin(), swJ.value("name", "").end());
                        sw.description = std::wstring(swJ.value("description", "").begin(), swJ.value("description", "").end());
                        sw.version = std::wstring(swJ.value("version", "").begin(), swJ.value("version", "").end());
                        sw.categoryId = std::wstring(swJ.value("category_id", "").begin(), swJ.value("category_id", "").end());
                        sw.downloadUrl = std::wstring(swJ.value("download_url", "").begin(), swJ.value("download_url", "").end());
                        sw.officialUrl = std::wstring(swJ.value("official_url", "").begin(), swJ.value("official_url", "").end());
                        sw.iconUrl = std::wstring(swJ.value("icon_url", "").begin(), swJ.value("icon_url", "").end());
                        sw.sizeMb = swJ.value("size_mb", 0);
                        sw.platform = std::wstring(swJ.value("platform", "").begin(), swJ.value("platform", "").end());
                        sw.isRecommended = swJ.value("is_recommended", false);
                        sw.sortOrder = swJ.value("sort_order", 0);

                        // 解析 tags
                        if (swJ.contains("tags") && swJ["tags"].is_array()) {
                            for (const auto& tagJ : swJ["tags"]) {
                                std::string tag = tagJ.get<std::string>();
                                sw.tags.push_back(std::wstring(tag.begin(), tag.end()));
                            }
                        }

                        outData.software.push_back(std::move(sw));
                    }
                }
            }
        }

        return !outData.categories.empty();
    } catch (const json::exception& e) {
        spdlog::error("解析推荐软件JSON失败: {}", e.what());
        return false;
    }
}

std::wstring SoftwareRecommendFetcher::GetRemoteUrl() {
    return std::wstring(L"https://") + kGitHubServer + kGitHubPath;
}

} // namespace IceClean::Core::Safety
