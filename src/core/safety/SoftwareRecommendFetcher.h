#pragma once
#include "models/RecommendedSoftware.h"
#include <string>
#include <functional>
#include <mutex>

namespace IceClean::Core::Safety {

// 推荐软件数据获取器
// 从 GitHub 仓库拉取推荐软件 JSON 数据
class SoftwareRecommendFetcher {
public:
    // 获取单例
    static SoftwareRecommendFetcher& Instance();

    // 异步获取推荐软件数据
    // callback 在调用线程执行，success 表示是否成功
    void FetchAsync(std::function<void(bool success, const Models::RecommendData& data)> callback);

    // 同步获取推荐软件数据（阻塞调用）
    bool FetchSync(Models::RecommendData& outData);

    // 获取远程 JSON URL
    static std::wstring GetRemoteUrl();

    // 是否正在获取中
    bool IsFetching() const { return m_fetching; }

private:
    SoftwareRecommendFetcher() = default;
    ~SoftwareRecommendFetcher() = default;

    SoftwareRecommendFetcher(const SoftwareRecommendFetcher&) = delete;
    SoftwareRecommendFetcher& operator=(const SoftwareRecommendFetcher&) = delete;

    // 使用 WinHTTP 发送 GET 请求
    bool HttpGet(const std::wstring& server, const std::wstring& path, std::string& response);

    // 解析 JSON 数据为 RecommendData
    bool ParseJson(const std::string& jsonStr, Models::RecommendData& outData);

    std::atomic<bool> m_fetching{false};
    mutable std::mutex m_mutex;

    // GitHub 仓库推荐软件 JSON 的原始内容地址
    static constexpr const wchar_t* kGitHubServer = L"raw.githubusercontent.com";
    static constexpr const wchar_t* kGitHubPath = L"/XiTu893/iceClean/main/docs/recommended_software.json";
};

} // namespace IceClean::Core::Safety
