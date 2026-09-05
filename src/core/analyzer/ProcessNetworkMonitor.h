#pragma once
#include <wx/wx.h>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>
#include "models/HardwareInfo.h"

namespace IceClean::Core::Analyzer {

struct ProcessNetworkStats {
    uint32_t pid = 0;
    std::wstring processName;
    uint64_t currentDownBps = 0;
    uint64_t currentUpBps = 0;
    uint64_t sessionTotalDown = 0;
    uint64_t sessionTotalUp = 0;
};

class ProcessNetworkMonitor {
public:
    ProcessNetworkMonitor() = default;
    ~ProcessNetworkMonitor() = default;

    using SnapshotCallback = std::function<void(const std::vector<ProcessNetworkStats>&)>;

    void SetSnapshotCallback(SnapshotCallback callback);
    void StartMonitoring(uint32_t intervalMs = 1000);
    void StopMonitoring();
    bool IsMonitoring() const { return m_monitoring.load(std::memory_order_relaxed); }

    static std::unique_ptr<ProcessNetworkMonitor> Create() {
        return std::make_unique<ProcessNetworkMonitor>();
    }

private:
    void MonitoringLoop();
    std::vector<ProcessNetworkStats> GetProcessNetworkStats();
    std::wstring GetProcessName(DWORD pid);

    std::atomic<bool> m_monitoring{false};
    std::thread m_thread;
    SnapshotCallback m_callback;
    uint32_t m_intervalMs = 1000;

    std::unordered_map<uint32_t, uint64_t> m_lastTcpRecv;
    std::unordered_map<uint32_t, uint64_t> m_lastTcpSent;
    std::unordered_map<uint32_t, uint64_t> m_lastUdpRecv;
    std::unordered_map<uint32_t, uint64_t> m_lastUdpSent;
    std::unordered_map<uint32_t, uint64_t> m_sessionDown;
    std::unordered_map<uint32_t, uint64_t> m_sessionUp;
    std::unordered_map<uint32_t, std::wstring> m_processNameCache;
    std::chrono::steady_clock::time_point m_lastSample;
};

} // namespace IceClean::Core::Analyzer
