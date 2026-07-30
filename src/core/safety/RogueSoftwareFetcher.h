#pragma once
#include "models/RogueSoftwareInfo.h"
#include <string>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>

namespace IceClean::Core::Safety {

// 流氓软件规则获取器
// 从 GitHub 远程拉取规则，支持本地缓存
class RogueSoftwareFetcher {
public:
    static RogueSoftwareFetcher& Instance();

    // 异步获取规则
    void FetchAsync(std::function<void(bool success, const IceClean::Models::RogueSoftwareDB& db)> callback);

    // 同步获取规则
    bool FetchSync(IceClean::Models::RogueSoftwareDB& outDb);

    // 获取远程 URL
    static std::wstring GetRemoteUrl();

    bool IsFetching() const { return m_fetching; }

    bool HasCachedData() const;

private:
    RogueSoftwareFetcher() = default;
    ~RogueSoftwareFetcher() = default;
    RogueSoftwareFetcher(const RogueSoftwareFetcher&) = delete;
    RogueSoftwareFetcher& operator=(const RogueSoftwareFetcher&) = delete;

    bool HttpGet(const std::wstring& server, const std::wstring& path, std::string& response);
    bool ParseJson(const std::string& jsonStr, IceClean::Models::RogueSoftwareDB& outDb);
    bool LoadLocalCache(IceClean::Models::RogueSoftwareDB& outDb);
    bool SaveLocalCache(const IceClean::Models::RogueSoftwareDB& db);

    std::atomic<bool> m_fetching{false};
    mutable std::mutex m_mutex;

    static constexpr const wchar_t* kGitHubServer = L"raw.githubusercontent.com";
    static constexpr const wchar_t* kGitHubPath = L"/XiTu893/iceClean/main/docs/softdetail.json";
};

} // namespace IceClean::Core::Safety
