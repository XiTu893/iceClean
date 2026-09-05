#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace IceClean::Models {

struct CpuInfo {
    std::wstring name;
    std::wstring manufacturer;
    int coreCount = 0;
    int logicalProcessorCount = 0;
    double maxClockSpeedGHz = 0.0;
    double currentClockSpeedGHz = 0.0;
    int l2CacheKB = 0;
    int l3CacheKB = 0;
    bool is64Bit = false;
    std::wstring architecture;
    std::wstring socketDesignation;
    std::wstring cpuId;
};

struct GpuInfo {
    std::wstring name;
    std::wstring adapterString;
    std::wstring manufacturer;
    std::wstring videoProcessor;
    std::wstring driverVersion;
    std::wstring driverDate;
    uint64_t dedicatedMemoryMB = 0;
    uint64_t sharedMemoryMB = 0;
    uint64_t totalMemoryMB = 0;
    std::wstring resolution;
    std::wstring videoModeDescription;
    int refreshRate = 0;
    std::wstring pnpDeviceId;
};

struct MemoryInfo {
    uint64_t totalPhysicalMB = 0;
    uint64_t availablePhysicalMB = 0;
    uint64_t totalVirtualMB = 0;
    uint64_t availableVirtualMB = 0;
    int memorySlotCount = 0;
    int memoryModuleCount = 0;
    int memorySpeed = 0;
    std::wstring memoryType;
    std::vector<std::wstring> memoryModules;
};

struct MotherboardInfo {
    std::wstring manufacturer;
    std::wstring product;
    std::wstring version;
    std::wstring serialNumber;
    std::wstring biosVendor;
    std::wstring biosVersion;
    std::wstring biosDate;
};

struct DiskInfo {
    std::wstring driveLetter;
    std::wstring model;
    std::wstring vendor;
    std::wstring interfaceType;
    std::wstring mediaType;
    uint64_t totalGB = 0;
    uint64_t freeGB = 0;
    std::wstring fileSystem;
    bool isSSD = false;
    bool isSystemDisk = false;
    double healthPercent = 100.0;
    uint32_t temperatureC = 0;
    uint64_t powerOnHours = 0;
};

struct NetworkAdapterDetail {
    std::wstring name;
    std::wstring description;
    std::wstring manufacturer;
    std::wstring macAddress;
    std::wstring ipAddress;
    uint64_t speedBps = 0;
    std::wstring connectionType;
    bool dhcpEnabled = false;
};

struct HardwareSummary {
    CpuInfo cpu;
    std::vector<GpuInfo> gpus;
    MemoryInfo memory;
    std::vector<DiskInfo> disks;
    MotherboardInfo motherboard;
    std::vector<NetworkAdapterDetail> networkAdapters;
    std::wstring osVersion;
    std::wstring osBuild;
    std::wstring osInstallDate;
    std::wstring systemType;
    std::wstring locale;
    bool isAdmin = false;
    std::wstring computerName;
    std::wstring userName;
    std::wstring systemUptime;
};

struct CpuSnapshot {
    double usagePercent = 0.0;
    int coreCount = 0;
    int logicalProcessorCount = 0;
    double frequencyGHz = 0.0;
    int l2CacheKB = 0;
    int l3CacheKB = 0;
};

struct MemorySnapshot {
    uint64_t totalBytes = 0;
    uint64_t availableBytes = 0;
    uint64_t usedBytes = 0;
    double usagePercent = 0.0;
};

struct DiskSnapshot {
    wchar_t driveLetter = L'C';
    uint64_t totalBytes = 0;
    uint64_t freeBytes = 0;
    uint64_t usedBytes = 0;
    double usagePercent = 0.0;
    std::wstring volumeLabel;
};

struct NetworkSnapshot {
    uint64_t bytesReceived = 0;
    uint64_t bytesSent = 0;
    uint64_t downloadSpeedBps = 0;
    uint64_t uploadSpeedBps = 0;
};

struct GpuSnapshot {
    double usagePercent = 0.0;
    double memoryUsagePercent = 0.0;
    double temperature = 0.0;
    bool available = false;
};

struct HardwareMonitorAdapterInfo {
    std::wstring name;
    std::wstring description;
    uint64_t speedBps = 0;
    std::wstring macAddress;
    bool isActive = false;
};

struct PerformanceSnapshot {
    std::chrono::system_clock::time_point timestamp;
    CpuSnapshot cpu;
    MemorySnapshot memory;
    std::vector<DiskSnapshot> disks;
    NetworkSnapshot network;
    std::vector<GpuSnapshot> gpus;
    std::vector<HardwareMonitorAdapterInfo> adapters;
};

struct PerformanceStats {
    double cpuAvg = 0.0;
    double cpuPeak = 0.0;
    double memoryAvg = 0.0;
    double memoryPeak = 0.0;
    uint64_t diskTotalBytes = 0;
    uint64_t diskFreeBytes = 0;
    uint64_t networkReceivedBytes = 0;
    uint64_t networkSentBytes = 0;
};

} // namespace IceClean::Models
