#include "PerformanceMonitor.h"
#include "HardwareDetector.h"
#include <spdlog/spdlog.h>
#include <windows.h>
#include <winioctl.h>
#include <iphlpapi.h>
#include <chrono>
#include <thread>
#include <algorithm>

#pragma comment(lib, "iphlpapi.lib")

namespace IceClean::Core::Analyzer {

using namespace IceClean::Models;

PerformanceMonitor::PerformanceMonitor() = default;

PerformanceMonitor::~PerformanceMonitor() {
    StopMonitoring();
}

std::unique_ptr<PerformanceMonitor> PerformanceMonitor::Create() {
    auto monitor = std::unique_ptr<PerformanceMonitor>(new PerformanceMonitor());

    FILETIME idle, kernel, user;
    if (GetSystemTimes(&idle, &kernel, &user)) {
        auto ftToUint64 = [](const FILETIME& ft) -> unsigned __int64 {
            return (static_cast<unsigned __int64>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        };
        monitor->m_prevIdleTime = ftToUint64(idle);
        monitor->m_prevKernelTime = ftToUint64(kernel);
        monitor->m_prevUserTime = ftToUint64(user);
    }
    monitor->m_lastCpuTime = std::chrono::steady_clock::now();
    monitor->UpdateNetworkBaseline();

    return monitor;
}

double PerformanceMonitor::CalculateCpuUsage() {
    FILETIME idle, kernel, user;
    if (!GetSystemTimes(&idle, &kernel, &user)) {
        return 0.0;
    }

    auto ftToUint64 = [](const FILETIME& ft) -> unsigned __int64 {
        return (static_cast<unsigned __int64>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    };

    unsigned __int64 currIdle = ftToUint64(idle);
    unsigned __int64 currKernel = ftToUint64(kernel);
    unsigned __int64 currUser = ftToUint64(user);

    const unsigned __int64 idleDelta = currIdle - m_prevIdleTime;
    const unsigned __int64 kernelDelta = currKernel - m_prevKernelTime;
    const unsigned __int64 userDelta = currUser - m_prevUserTime;
    const unsigned __int64 total = kernelDelta + userDelta;

    m_prevIdleTime = currIdle;
    m_prevKernelTime = currKernel;
    m_prevUserTime = currUser;

    if (total == 0) {
        return 0.0;
    }

    double usage = (1.0 - static_cast<double>(idleDelta) / static_cast<double>(total)) * 100.0;
    return std::clamp(usage, 0.0, 100.0);
}

CpuSnapshot PerformanceMonitor::GetCpuSnapshot() {
    CpuSnapshot snap;
    snap.usagePercent = CalculateCpuUsage();

    HardwareDetector detector;
    CpuInfo info = detector.GetCpuInfo();
    snap.coreCount = info.coreCount;
    snap.logicalProcessorCount = info.logicalProcessorCount;
    snap.frequencyGHz = info.maxClockSpeedGHz;
    snap.l2CacheKB = info.l2CacheKB;
    snap.l3CacheKB = info.l3CacheKB;

    m_lastCpu = snap;
    return snap;
}

MemorySnapshot PerformanceMonitor::GetMemorySnapshot() {
    MemorySnapshot snap;

    MEMORYSTATUSEX memStatus{};
    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus)) {
        snap.totalBytes = memStatus.ullTotalPhys;
        snap.availableBytes = memStatus.ullAvailPhys;
        snap.usedBytes = memStatus.ullTotalPhys - memStatus.ullAvailPhys;
        if (snap.totalBytes > 0) {
            snap.usagePercent = (static_cast<double>(snap.usedBytes) / static_cast<double>(snap.totalBytes)) * 100.0;
        }
    }

    return snap;
}

std::vector<std::wstring> PerformanceMonitor::GetDriveLetters() {
    std::vector<std::wstring> letters;
    DWORD bitmask = GetLogicalDrives();
    for (wchar_t c = L'A'; c <= L'Z'; ++c) {
        if (bitmask & (1u << (c - L'A'))) {
            std::wstring root(1, c);
            root += L":\\";
            UINT type = GetDriveTypeW(root.c_str());
            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
                letters.push_back(root);
            }
        }
    }
    return letters;
}

std::vector<DiskSnapshot> PerformanceMonitor::GetDiskSnapshots() {
    std::vector<DiskSnapshot> snaps;
    auto letters = GetDriveLetters();

    wchar_t volumeName[MAX_PATH + 1] = {};
    for (const auto& root : letters) {
        DiskSnapshot snap;
        snap.driveLetter = root[0];

        ULARGE_INTEGER freeBytesAvail = {};
        ULARGE_INTEGER totalBytes = {};
        if (GetDiskFreeSpaceExW(root.c_str(), nullptr, &totalBytes, &freeBytesAvail)) {
            snap.totalBytes = totalBytes.QuadPart;
            snap.freeBytes = freeBytesAvail.QuadPart;
            snap.usedBytes = totalBytes.QuadPart - freeBytesAvail.QuadPart;
            if (totalBytes.QuadPart > 0) {
                snap.usagePercent = (static_cast<double>(snap.usedBytes) / static_cast<double>(totalBytes.QuadPart)) * 100.0;
            }
        }

        GetVolumeInformationW(root.c_str(), volumeName, MAX_PATH,
                              nullptr, nullptr, nullptr, nullptr, 0);
        snap.volumeLabel = volumeName;

        snaps.push_back(snap);
    }

    return snaps;
}

void PerformanceMonitor::UpdateNetworkBaseline() {
    m_lastNetworkReceived = 0;
    m_lastNetworkSent = 0;

    MIB_IFTABLE* pIfTable = nullptr;
    ULONG dwSize = 0;
    if (GetIfTable(nullptr, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        pIfTable = reinterpret_cast<MIB_IFTABLE*>(new uint8_t[dwSize]);
        if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < pIfTable->dwNumEntries; ++i) {
                MIB_IFROW& row = pIfTable->table[i];
                if (row.dwType > 0 && row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL) {
                    m_lastNetworkReceived += row.dwInOctets;
                    m_lastNetworkSent += row.dwOutOctets;
                }
            }
        }
        delete[] reinterpret_cast<uint8_t*>(pIfTable);
    }
    m_lastNetworkTime = std::chrono::steady_clock::now();
}

NetworkSnapshot PerformanceMonitor::GetNetworkSnapshot() {
    NetworkSnapshot snap{};

    MIB_IFTABLE* pIfTable = nullptr;
    ULONG dwSize = 0;
    if (GetIfTable(nullptr, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        pIfTable = reinterpret_cast<MIB_IFTABLE*>(new uint8_t[dwSize]);
        if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < pIfTable->dwNumEntries; ++i) {
                MIB_IFROW& row = pIfTable->table[i];
                if (row.dwType > 0 && row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL) {
                    snap.bytesReceived += row.dwInOctets;
                    snap.bytesSent += row.dwOutOctets;
                }
            }
        }
        delete[] reinterpret_cast<uint8_t*>(pIfTable);
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastNetworkTime).count();
    if (elapsedMs > 0) {
        if (snap.bytesReceived >= m_lastNetworkReceived) {
            snap.downloadSpeedBps = (snap.bytesReceived - m_lastNetworkReceived) * 1000 / static_cast<uint64_t>(elapsedMs);
        }
        if (snap.bytesSent >= m_lastNetworkSent) {
            snap.uploadSpeedBps = (snap.bytesSent - m_lastNetworkSent) * 1000 / static_cast<uint64_t>(elapsedMs);
        }
    }

    m_lastNetworkReceived = snap.bytesReceived;
    m_lastNetworkSent = snap.bytesSent;
    m_lastNetworkTime = now;

    return snap;
}

std::vector<GpuSnapshot> PerformanceMonitor::GetGpuSnapshots() {
    std::vector<GpuSnapshot> snaps;

    HMODULE hNvml = LoadLibraryW(L"nvml.dll");
    if (!hNvml) {
        GpuSnapshot snap;
        snap.available = false;
        snap.usagePercent = -1.0;
        snaps.push_back(snap);
        return snaps;
    }

    auto nvmlInit = (int(*)())GetProcAddress(hNvml, "nvmlInit_v2");
    if (!nvmlInit || nvmlInit() != 0) {
        FreeLibrary(hNvml);
        GpuSnapshot snap;
        snap.available = false;
        snap.usagePercent = -1.0;
        snaps.push_back(snap);
        return snaps;
    }

    auto getCount = (int(*)(unsigned int*))GetProcAddress(hNvml, "nvmlDeviceGetCount_v2");
    auto getHandle = (int(*)(unsigned int, void**))GetProcAddress(hNvml, "nvmlDeviceGetHandleByIndex_v2");
    auto getUtil = (int(*)(void*, void*))GetProcAddress(hNvml, "nvmlDeviceGetUtilizationRates");
    auto nvmlShutdown = (int(*)())GetProcAddress(hNvml, "nvmlShutdown");

    if (getCount && getHandle && getUtil) {
        unsigned int n = 0;
        if (getCount(&n) == 0 && n > 0) {
            for (unsigned int i = 0; i < n && i < 4; ++i) {
                void* dev = nullptr;
                if (getHandle(i, &dev) == 0) {
                    GpuSnapshot snap;
                    snap.available = true;
                    struct NvmlUtil { unsigned int gpu; unsigned int memory; };
                    NvmlUtil util{};
                    if (getUtil(dev, &util) == 0) {
                        snap.usagePercent = util.gpu;
                        snap.memoryUsagePercent = util.memory;
                    }
                    snaps.push_back(snap);
                }
            }
        }
    }

    if (nvmlShutdown) nvmlShutdown();
    FreeLibrary(hNvml);

    if (snaps.empty()) {
        GpuSnapshot snap;
        snap.available = false;
        snap.usagePercent = -1.0;
        snaps.push_back(snap);
    }

    return snaps;
}

std::vector<HardwareMonitorAdapterInfo> PerformanceMonitor::GetNetworkAdapters() {
    std::vector<HardwareMonitorAdapterInfo> adapters;

    MIB_IFTABLE* pIfTable = nullptr;
    ULONG dwSize = 0;
    if (GetIfTable(nullptr, &dwSize, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        pIfTable = reinterpret_cast<MIB_IFTABLE*>(new uint8_t[dwSize]);
        if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
            for (DWORD i = 0; i < pIfTable->dwNumEntries; ++i) {
                MIB_IFROW& row = pIfTable->table[i];
                if (row.dwType > 0 && row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL) {
                    HardwareMonitorAdapterInfo info;
                    int len = row.dwDescrLen < 128 ? row.dwDescrLen : 128;
                    std::string desc(row.bDescr, row.bDescr + len);
                    info.description = std::wstring(desc.begin(), desc.end());
                    info.name = info.description;
                    info.speedBps = row.dwSpeed;
                    info.isActive = (row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL);
                    adapters.push_back(info);
                }
            }
        }
        delete[] reinterpret_cast<uint8_t*>(pIfTable);
    }

    return adapters;
}

PerformanceSnapshot PerformanceMonitor::GetFullSnapshot() {
    PerformanceSnapshot snap;
    snap.timestamp = std::chrono::system_clock::now();
    snap.cpu = GetCpuSnapshot();
    snap.memory = GetMemorySnapshot();
    snap.disks = GetDiskSnapshots();
    snap.network = GetNetworkSnapshot();
    snap.gpus = GetGpuSnapshots();
    snap.adapters = GetNetworkAdapters();
    return snap;
}

void PerformanceMonitor::StartMonitoring(int intervalMs) {
    if (m_running.exchange(true)) {
        return;
    }
    m_intervalMs = intervalMs;

    UpdateNetworkBaseline();

    m_workerThread = std::jthread([this](std::stop_token st) {
        spdlog::info("PerformanceMonitor started (interval={}ms)", m_intervalMs);
        while (!st.stop_requested()) {
            auto snapshot = GetFullSnapshot();
            if (m_callback) {
                m_callback(snapshot);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(m_intervalMs));
        }
        spdlog::info("PerformanceMonitor stopped");
    });
}

void PerformanceMonitor::StopMonitoring() {
    if (m_running.exchange(false)) {
        if (m_workerThread.joinable()) {
            m_workerThread.request_stop();
            m_workerThread.join();
        }
    }
}

void PerformanceMonitor::SetSnapshotCallback(SnapshotCallback callback) {
    m_callback = std::move(callback);
}

} // namespace IceClean::Core::Analyzer
