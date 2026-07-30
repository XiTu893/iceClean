#pragma once
#include <string>
#include <vector>

namespace IceClean::Models {

struct CpuInfo {
    std::wstring name;
    std::wstring manufacturer;
    int coreCount = 0;
    int logicalProcessorCount = 0;
    double maxClockSpeedGHz = 0.0;
    bool is64Bit = false;
    std::wstring architecture;
};

struct GpuInfo {
    std::wstring name;
    std::wstring adapterString;
    uint64_t dedicatedMemoryMB = 0;
    uint64_t sharedMemoryMB = 0;
    std::wstring driverVersion;
    std::wstring resolution;
};

struct MemoryInfo {
    uint64_t totalPhysicalMB = 0;
    uint64_t availablePhysicalMB = 0;
    uint64_t totalVirtualMB = 0;
    uint64_t availableVirtualMB = 0;
    int memorySlotCount = 0;
    std::vector<std::wstring> memoryModules;
};

struct DiskInfo {
    std::wstring driveLetter;
    std::wstring model;
    uint64_t totalGB = 0;
    uint64_t freeGB = 0;
    std::wstring fileSystem;
    bool isSSD = false;
    bool isSystemDisk = false;
    double healthPercent = 100.0;
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

struct HardwareSummary {
    CpuInfo cpu;
    GpuInfo gpu;
    MemoryInfo memory;
    std::vector<DiskInfo> disks;
    MotherboardInfo motherboard;
    std::wstring osVersion;
    std::wstring osBuild;
    bool isAdmin = false;
    std::wstring computerName;
    std::wstring userName;
    std::wstring systemUptime;
};

} // namespace IceClean::Models
