#pragma once
#include <cstdint>
#include <string>
#include <mutex>

namespace IceClean::Core::Safety {

// 累计使用统计管理器（线程安全单例）
// 管理 %APPDATA%\IceClean\usage_stats.json
class UsageStats {
public:
    static UsageStats& Instance();

    // 记录一次清理操作
    void RecordClean(uint64_t bytes);

    // 获取累计清理字节数
    uint64_t GetTotalCleanedBytes() const;

    // 获取清理次数
    int GetCleanCount() const;

    // 获取上次清理时间（ISO 8601格式）
    std::wstring GetLastCleanTime() const;

private:
    UsageStats() = default;
    ~UsageStats() = default;

    UsageStats(const UsageStats&) = delete;
    UsageStats& operator=(const UsageStats&) = delete;

    void Load();
    void Save() const;

    std::wstring GetStatsPath() const;

    mutable std::mutex m_mutex;
    uint64_t m_totalCleanedBytes = 0;
    int m_cleanCount = 0;
    std::wstring m_lastCleanTime;
    bool m_loaded = false;
};

} // namespace IceClean::Core::Safety
