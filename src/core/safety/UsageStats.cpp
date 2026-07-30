#include "UsageStats.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <chrono>
#include <windows.h>
#include <shlobj.h>

namespace IceClean::Core::Safety {

using json = nlohmann::json;

UsageStats& UsageStats::Instance() {
    static UsageStats instance;
    return instance;
}

std::wstring UsageStats::GetStatsPath() const {
    wchar_t appDataPath[MAX_PATH] = {0};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath))) {
        return L"";
    }
    auto dir = std::wstring(appDataPath) + L"\\IceClean";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir + L"\\usage_stats.json";
}

void UsageStats::Load() {
    auto path = GetStatsPath();
    if (path.empty()) return;

    try {
        std::ifstream file(path);
        if (!file.is_open()) return;

        json j;
        file >> j;

        if (j.contains("totalCleanedBytes")) {
            m_totalCleanedBytes = j["totalCleanedBytes"].get<uint64_t>();
        }
        if (j.contains("cleanCount")) {
            m_cleanCount = j["cleanCount"].get<int>();
        }
        if (j.contains("lastCleanTime")) {
            m_lastCleanTime = j["lastCleanTime"].get<std::wstring>();
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("加载使用统计失败: {}", e.what());
    }

    m_loaded = true;
}

void UsageStats::Save() const {
    auto path = GetStatsPath();
    if (path.empty()) return;

    try {
        json j;
        j["totalCleanedBytes"] = m_totalCleanedBytes;
        j["cleanCount"] = m_cleanCount;
        j["lastCleanTime"] = m_lastCleanTime;

        std::ofstream file(path);
        if (file.is_open()) {
            file << j.dump(2);
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("保存使用统计失败: {}", e.what());
    }
}

void UsageStats::RecordClean(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_loaded) {
        Load();
    }

    m_totalCleanedBytes += bytes;
    m_cleanCount++;

    // 记录当前时间
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    struct tm tmBuf {};
    localtime_s(&tmBuf, &timeT);
    wchar_t timeStr[64] = {};
    wcsftime(timeStr, 64, L"%Y-%m-%dT%H:%M:%S", &tmBuf);
    m_lastCleanTime = timeStr;

    Save();

    spdlog::info("记录清理: {} bytes, 累计: {} bytes, 次数: {}",
                 bytes, m_totalCleanedBytes, m_cleanCount);
}

uint64_t UsageStats::GetTotalCleanedBytes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_loaded) {
        const_cast<UsageStats*>(this)->Load();
    }
    return m_totalCleanedBytes;
}

int UsageStats::GetCleanCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_loaded) {
        const_cast<UsageStats*>(this)->Load();
    }
    return m_cleanCount;
}

std::wstring UsageStats::GetLastCleanTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_loaded) {
        const_cast<UsageStats*>(this)->Load();
    }
    return m_lastCleanTime;
}

} // namespace IceClean::Core::Safety
