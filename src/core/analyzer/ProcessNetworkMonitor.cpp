#include "ProcessNetworkMonitor.h"
#include <spdlog/spdlog.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <unordered_set>
#pragma comment(lib, "psapi.lib")

namespace IceClean::Core::Analyzer {

void ProcessNetworkMonitor::SetSnapshotCallback(SnapshotCallback callback) {
    m_callback = std::move(callback);
}

void ProcessNetworkMonitor::StartMonitoring(uint32_t intervalMs) {
    if (m_monitoring.exchange(true)) return;
    m_intervalMs = intervalMs;
    m_lastSample = std::chrono::steady_clock::now();
    m_lastTcpRecv.clear();
    m_lastTcpSent.clear();
    m_lastUdpRecv.clear();
    m_lastUdpSent.clear();
    m_sessionDown.clear();
    m_sessionUp.clear();
    m_processNameCache.clear();
    m_thread = std::thread(&ProcessNetworkMonitor::MonitoringLoop, this);
}

void ProcessNetworkMonitor::StopMonitoring() {
    if (!m_monitoring.exchange(false)) return;
    if (m_thread.joinable()) m_thread.join();
}

std::wstring ProcessNetworkMonitor::GetProcessName(DWORD pid) {
    auto it = m_processNameCache.find(pid);
    if (it != m_processNameCache.end()) return it->second;

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) {
        m_processNameCache[pid] = L"PID:" + std::to_wstring(pid);
        return m_processNameCache[pid];
    }

    std::wstring result = L"PID:" + std::to_wstring(pid);
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                result = std::wstring(pe.szExeFile);
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
    m_processNameCache[pid] = result;
    return result;
}

void ProcessNetworkMonitor::MonitoringLoop() {
    spdlog::info("[ProcessNetwork] monitoring started");
    while (m_monitoring.load(std::memory_order_relaxed)) {
        try {
            auto stats = GetProcessNetworkStats();
            if (m_callback) m_callback(stats);
        } catch (const std::exception& e) {
            spdlog::error("[ProcessNetwork] error: {}", e.what());
        }
        for (uint32_t i = 0; i < m_intervalMs / 100; ++i) {
            if (!m_monitoring.load(std::memory_order_relaxed)) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    spdlog::info("[ProcessNetwork] monitoring stopped");
}

std::vector<ProcessNetworkStats> ProcessNetworkMonitor::GetProcessNetworkStats() {
    std::unordered_map<uint32_t, std::pair<uint64_t, uint64_t>> currentDownUp;

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return {};

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    if (!Process32FirstW(hSnap, &pe)) {
        CloseHandle(hSnap);
        return {};
    }

    do {
        DWORD pid = pe.th32ProcessID;
        if (pid == 0) continue;

        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProc) continue;

        IO_COUNTERS io{};
        if (GetProcessIoCounters(hProc, &io)) {
            currentDownUp[pid] = {io.ReadTransferCount, io.WriteTransferCount};
            m_processNameCache[pid] = pe.szExeFile;
        }
        CloseHandle(hProc);
    } while (Process32NextW(hSnap, &pe));
    CloseHandle(hSnap);

    auto now = std::chrono::steady_clock::now();
    double elapsed = std::chrono::duration<double>(now - m_lastSample).count();
    if (elapsed < 0.1) elapsed = 0.1;
    m_lastSample = now;

    std::vector<ProcessNetworkStats> result;
    result.reserve(currentDownUp.size());

    for (const auto& [pid, bytes] : currentDownUp) {
        ProcessNetworkStats s;
        s.pid = pid;
        s.processName = GetProcessName(pid);

        uint64_t lastDown = 0, lastUp = 0;
        auto itDown = m_lastTcpRecv.find(pid);
        if (itDown != m_lastTcpRecv.end()) lastDown = itDown->second;
        auto itUp = m_lastTcpSent.find(pid);
        if (itUp != m_lastTcpSent.end()) lastUp = itUp->second;

        if (bytes.first >= lastDown && bytes.second >= lastUp) {
            s.currentDownBps = static_cast<uint64_t>((bytes.first - lastDown) / elapsed);
            s.currentUpBps = static_cast<uint64_t>((bytes.second - lastUp) / elapsed);
        } else {
            s.currentDownBps = 0;
            s.currentUpBps = 0;
        }
        m_lastTcpRecv[pid] = bytes.first;
        m_lastTcpSent[pid] = bytes.second;

        s.sessionTotalDown = bytes.first;
        s.sessionTotalUp = bytes.second;
        m_sessionDown[pid] = bytes.first;
        m_sessionUp[pid] = bytes.second;

        result.push_back(std::move(s));
    }

    // 只保留有活动流量的进程 (current rate > 0 或 session total > 1MB)
    std::vector<ProcessNetworkStats> active;
    for (auto& s : result) {
        if (s.currentDownBps > 0 || s.currentUpBps > 0 || s.sessionTotalDown > 1024 * 1024 || s.sessionTotalUp > 1024 * 1024) {
            active.push_back(std::move(s));
        }
    }
    std::sort(active.begin(), active.end(), [](const ProcessNetworkStats& a, const ProcessNetworkStats& b) {
        return (a.currentDownBps + a.currentUpBps) > (b.currentDownBps + b.currentUpBps);
    });

    return active;
}

} // namespace IceClean::Core::Analyzer
