#include "RogueSoftwareFetcher.h"
#include "utils/Win32Util.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <winhttp.h>
#include <fstream>
#include <filesystem>
#include <thread>

#pragma comment(lib, "winhttp.lib")

namespace IceClean::Core::Safety {

using json = nlohmann::json;
namespace fs = std::filesystem;

// ── 单例 ──

RogueSoftwareFetcher& RogueSoftwareFetcher::Instance() {
    static RogueSoftwareFetcher instance;
    return instance;
}

// ── 异步获取 ──

void RogueSoftwareFetcher::FetchAsync(
    std::function<void(bool success, const IceClean::Models::RogueSoftwareDB& db)> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fetching) {
        if (callback) {
            IceClean::Models::RogueSoftwareDB empty;
            callback(false, empty);
        }
        return;
    }
    m_fetching = true;

    std::thread([this, callback]() {
        IceClean::Models::RogueSoftwareDB db;
        bool success = FetchSync(db);
        m_fetching = false;
        if (callback) callback(success, db);
    }).detach();
}

// ── 同步获取 ──

bool RogueSoftwareFetcher::FetchSync(IceClean::Models::RogueSoftwareDB& outDb) {
    spdlog::info("获取流氓软件规则数据...");

    std::string response;
    bool httpSuccess = HttpGet(kGitHubServer, kGitHubPath, response);

    if (httpSuccess && !response.empty() && ParseJson(response, outDb)) {
        spdlog::info("远程规则获取成功，版本: {}", outDb.version);
        SaveLocalCache(outDb);
        return true;
    }

    spdlog::warn("远程获取失败，尝试使用本地缓存");
    return LoadLocalCache(outDb);
}

// ── HTTP GET ──

bool RogueSoftwareFetcher::HttpGet(const std::wstring& server,
                                    const std::wstring& path,
                                    std::string& response)
{
    HINTERNET hSession = WinHttpOpen(
        L"IceClean-RogueFetcher/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(
        hSession, server.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect, L"GET", path.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD connectTimeout = 5000;
    DWORD receiveTimeout = 10000;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT,
                     &connectTimeout, sizeof(connectTimeout));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT,
                     &receiveTimeout, sizeof(receiveTimeout));

    LPCWSTR headers = L"User-Agent: IceClean-RogueFetcher\r\n";
    WinHttpAddRequestHeaders(hRequest, headers, -1, WINHTTP_ADDREQ_FLAG_ADD);

    BOOL bResults = WinHttpSendRequest(
        hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0);

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, nullptr);
        if (bResults) {
            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest,
                                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX,
                                &statusCode, &statusCodeSize,
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
                spdlog::warn("RogueSoftwareFetcher HTTP {}", statusCode);
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

bool RogueSoftwareFetcher::ParseJson(const std::string& jsonStr,
                                      IceClean::Models::RogueSoftwareDB& outDb)
{
    try {
        auto j = json::parse(jsonStr);

        outDb.version = j.value("version", 0);
        if (j.contains("updated_at")) {
            std::string updatedAt = j["updated_at"].get<std::string>();
            outDb.updatedAt = std::wstring(updatedAt.begin(), updatedAt.end());
        }

        if (j.contains("rules") && j["rules"].is_array()) {
            for (const auto& ruleJ : j["rules"]) {
                IceClean::Models::RogueSoftwareRule rule;
                rule.id = std::wstring(ruleJ.value("id", "").begin(), ruleJ.value("id", "").end());
                rule.name = std::wstring(ruleJ.value("name", "").begin(), ruleJ.value("name", "").end());
                rule.category = std::wstring(ruleJ.value("category", "").begin(), ruleJ.value("category", "").end());
                rule.safetyLevel = std::wstring(ruleJ.value("safety_level", "").begin(), ruleJ.value("safety_level", "").end());
                rule.description = std::wstring(ruleJ.value("description", "").begin(), ruleJ.value("description", "").end());

                if (ruleJ.contains("aliases") && ruleJ["aliases"].is_array()) {
                    for (const auto& aliasJ : ruleJ["aliases"]) {
                        rule.aliases.push_back(std::wstring(aliasJ.get<std::string>().begin(),
                                                             aliasJ.get<std::string>().end()));
                    }
                }

                if (ruleJ.contains("indicators") && ruleJ["indicators"].is_object()) {
                    const auto& ind = ruleJ["indicators"];
                    rule.hasIndicator.hasProcesses = ind.contains("processes") && !ind["processes"].empty();
                    rule.hasIndicator.hasRegistry = ind.contains("registry") && !ind["registry"].empty();
                    rule.hasIndicator.hasFiles = ind.contains("files") && !ind["files"].empty();
                    rule.hasIndicator.hasServices = ind.contains("services") && !ind["services"].empty();
                    rule.hasIndicator.hasScheduledTasks = ind.contains("scheduled_tasks") && !ind["scheduled_tasks"].empty();
                    rule.hasIndicator.hasBrowserHijack = ind.contains("browser_hijack") && !ind["browser_hijack"].empty();
                }

                if (ruleJ.contains("cleanup_actions") && ruleJ["cleanup_actions"].is_object()) {
                    const auto& act = ruleJ["cleanup_actions"];
                    rule.actions.killProcess = act.value("kill_process", false);
                    rule.actions.stopService = act.value("stop_service", false);
                    rule.actions.deleteRegistry = act.value("delete_registry", false);
                    rule.actions.deleteFiles = act.value("delete_files", false);
                    rule.actions.restoreBrowser = act.value("restore_browser", false);
                }

                outDb.rules.push_back(std::move(rule));
            }
        }

        return !outDb.rules.empty();
    } catch (const json::exception& e) {
        spdlog::error("解析流氓软件规则JSON失败: {}", e.what());
        return false;
    }
}

// ── 本地缓存 ──

bool RogueSoftwareFetcher::HasCachedData() const {
    std::wstring localPath = IceClean::Utils::Win32Util::ExpandEnvVars(
        L"%LOCALAPPDATA%\\IceClean\\cache\\rogue_software.json");
    return fs::exists(localPath);
}

bool RogueSoftwareFetcher::LoadLocalCache(IceClean::Models::RogueSoftwareDB& outDb) {
    std::wstring localPath = IceClean::Utils::Win32Util::ExpandEnvVars(
        L"%LOCALAPPDATA%\\IceClean\\cache\\rogue_software.json");

    std::ifstream ifs(localPath);
    if (!ifs.is_open()) {
        spdlog::warn("本地缓存文件不存在: {}", std::string(localPath.begin(), localPath.end()));
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    ifs.close();

    if (content.empty()) return false;

    if (ParseJson(content, outDb)) {
        spdlog::info("本地缓存规则加载成功，版本: {}", outDb.version);
        return true;
    }

    return false;
}

bool RogueSoftwareFetcher::SaveLocalCache(const IceClean::Models::RogueSoftwareDB& db) {
    try {
        std::wstring cacheDir = IceClean::Utils::Win32Util::ExpandEnvVars(
            L"%LOCALAPPDATA%\\IceClean\\cache");
        fs::create_directories(cacheDir);

        std::wstring localPath = cacheDir + L"\\rogue_software.json";

        json j;
        j["version"] = db.version;
        j["updated_at"] = std::string(db.updatedAt.begin(), db.updatedAt.end());

        json rulesJ = json::array();
        for (const auto& rule : db.rules) {
            json ruleJ;
            ruleJ["id"] = std::string(rule.id.begin(), rule.id.end());
            ruleJ["name"] = std::string(rule.name.begin(), rule.name.end());
            ruleJ["category"] = std::string(rule.category.begin(), rule.category.end());
            ruleJ["safety_level"] = std::string(rule.safetyLevel.begin(), rule.safetyLevel.end());
            ruleJ["description"] = std::string(rule.description.begin(), rule.description.end());
            rulesJ.push_back(ruleJ);
        }
        j["rules"] = rulesJ;

        std::ofstream ofs(localPath);
        if (ofs.is_open()) {
            ofs << j.dump(2);
            ofs.close();
            spdlog::info("规则缓存已保存: {}", std::string(localPath.begin(), localPath.end()));
            return true;
        }
    } catch (const std::exception& e) {
        spdlog::error("保存规则缓存失败: {}", e.what());
    }
    return false;
}

std::wstring RogueSoftwareFetcher::GetRemoteUrl() {
    return std::wstring(L"https://") + kGitHubServer + kGitHubPath;
}

} // namespace IceClean::Core::Safety
