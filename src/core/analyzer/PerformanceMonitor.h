#pragma once
#include "models/HardwareInfo.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <chrono>

namespace IceClean::Core::Analyzer {

class PerformanceMonitor {
public:
    using SnapshotCallback = std::function<void(const Models::PerformanceSnapshot&)>;

    PerformanceMonitor();
    ~PerformanceMonitor();

    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;

    Models::CpuSnapshot GetCpuSnapshot();
    Models::MemorySnapshot GetMemorySnapshot();
    std::vector<Models::DiskSnapshot> GetDiskSnapshots();
    Models::NetworkSnapshot GetNetworkSnapshot();
    std::vector<Models::GpuSnapshot> GetGpuSnapshots();
    std::vector<Models::HardwareMonitorAdapterInfo> GetNetworkAdapters();
    Models::PerformanceSnapshot GetFullSnapshot();

    void StartMonitoring(int intervalMs = 1000);
    void StopMonitoring();

    bool IsRunning() const { return m_running.load(); }

    void SetSnapshotCallback(SnapshotCallback callback);

    static std::unique_ptr<PerformanceMonitor> Create();

private:
    double CalculateCpuUsage();
    void UpdateNetworkBaseline();

    Models::CpuSnapshot m_lastCpu;
    uint64_t m_lastNetworkReceived = 0;
    uint64_t m_lastNetworkSent = 0;
    std::chrono::steady_clock::time_point m_lastCpuTime;
    std::chrono::steady_clock::time_point m_lastNetworkTime;

    unsigned __int64 m_prevIdleTime = 0;
    unsigned __int64 m_prevKernelTime = 0;
    unsigned __int64 m_prevUserTime = 0;

    std::atomic<bool> m_running{false};
    std::jthread m_workerThread;
    SnapshotCallback m_callback;
    int m_intervalMs = 1000;

    std::vector<std::wstring> GetDriveLetters();
};

} // namespace IceClean::Core::Analyzer
