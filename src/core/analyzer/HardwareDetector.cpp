#include "HardwareDetector.h"
#include "utils/RegistryUtil.h"
#include "utils/Win32Util.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <tlhelp32.h>
#include <winternl.h>
#include <intrin.h>
#include <iphlpapi.h>
#include <ipifcons.h>
#pragma comment(lib, "iphlpapi.lib")

namespace IceClean::Core::Analyzer {

using namespace IceClean::Models;
using namespace IceClean::Utils;

// ── 完整摘要 ──

HardwareSummary HardwareDetector::GetSummary() {
    HardwareSummary summary;
    summary.cpu = GetCpuInfo();

    auto gpus = GetGpuInfo();
    summary.gpus.push_back(gpus);

    summary.memory = GetMemoryInfo();
    summary.disks = GetDiskInfo();
    summary.motherboard = GetMotherboardInfo();
    GetOsInfo(summary.osVersion, summary.osBuild);

    summary.osInstallDate = GetRegistryHardwareInfo(
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"InstallDate");
    summary.systemType = GetRegistryHardwareInfo(
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OEMInformation", L"Model");
    if (summary.systemType.empty()) {
        summary.systemType = GetRegistryHardwareInfo(
            L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemProductName");
    }
    summary.locale = GetRegistryHardwareInfo(
        L"SYSTEM\\CurrentControlSet\\Control\\Nls\\Language", L"InstallLanguage");

    summary.isAdmin = Win32Util::IsRunningAsAdmin();
    summary.computerName = GetRegistryHardwareInfo(
        L"SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ActiveComputerName", L"ComputerName");
    summary.userName = GetRegistryHardwareInfo(
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer", L"LogonUserName");
    summary.systemUptime = GetSystemUptime();

    summary.networkAdapters = GetNetworkAdaptersDetail();

    return summary;
}

// ── CPU 信息 ──

CpuInfo HardwareDetector::GetCpuInfo() {
    CpuInfo info;
    info.name = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString");
    info.manufacturer = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"VendorIdentifier");
    info.cpuId = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"Identifier");
    info.socketDesignation = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"SocketDesignation");

    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);
    info.logicalProcessorCount = sysInfo.dwNumberOfProcessors;
    info.is64Bit = (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
                    sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64);
    info.architecture = info.is64Bit ? L"x64" : L"x86";

    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = nullptr;
    DWORD bufferSize = 0;
    if (!GetLogicalProcessorInformation(buffer, &bufferSize) && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        buffer = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION>(malloc(bufferSize));
        if (buffer && GetLogicalProcessorInformation(buffer, &bufferSize)) {
            DWORD offset = 0;
            while (offset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= bufferSize) {
                if (buffer[offset / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)].Relationship == RelationProcessorCore) {
                    info.coreCount++;
                }
                offset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            }
        }
        free(buffer);
    }
    if (info.coreCount == 0) info.coreCount = info.logicalProcessorCount / 2;

    DWORD mhz = RegistryUtil::ReadDwordValue(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"~MHz");
    if (mhz > 0) {
        info.maxClockSpeedGHz = static_cast<double>(mhz) / 1000.0;
        info.currentClockSpeedGHz = info.maxClockSpeedGHz;
    }

    DWORD l2Size = RegistryUtil::ReadDwordValue(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"L2CacheSize");
    if (l2Size > 0) info.l2CacheKB = l2Size;

    DWORD l3Size = RegistryUtil::ReadDwordValue(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"L3CacheSize");
    if (l3Size > 0) info.l3CacheKB = l3Size;

    return info;
}

// ── GPU 信息 ──

GpuInfo HardwareDetector::GetGpuInfo() {
    GpuInfo info;

    auto subKeys = RegistryUtil::EnumSubKeys(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}");
    for (const auto& key : subKeys) {
        std::wstring keyPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\" + key;
        std::wstring driverDesc = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, keyPath, L"DriverDesc");
        if (!driverDesc.empty() && driverDesc.find(L"Microsoft") == std::wstring::npos) {
            info.name = driverDesc;
            info.adapterString = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, keyPath, L"HardwareInformation.AdapterString");
            info.driverVersion = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, keyPath, L"DriverVersion");
            info.driverDate = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, keyPath, L"DriverDate");
            info.pnpDeviceId = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, keyPath, L"DriverDesc");

            DWORD memSize = RegistryUtil::ReadDwordValue(HKEY_LOCAL_MACHINE, keyPath, L"HardwareInformation.DedicatedMemorySize");
            if (memSize > 0) {
                info.dedicatedMemoryMB = memSize;
                info.totalMemoryMB = memSize;
            }
            info.sharedMemoryMB = 0;

            info.videoProcessor = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, keyPath, L"HardwareInformation.VChipType");

            DEVMODEW devMode = {};
            devMode.dmSize = sizeof(devMode);
            if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &devMode)) {
                info.resolution = std::to_wstring(devMode.dmPelsWidth) + L" x " + std::to_wstring(devMode.dmPelsHeight);
                info.refreshRate = devMode.dmDisplayFrequency;
                info.videoModeDescription = std::to_wstring(devMode.dmPelsWidth) + L"x" +
                    std::to_wstring(devMode.dmPelsHeight) + L" " +
                    std::to_wstring(devMode.dmDisplayFrequency) + L"Hz " +
                    ((devMode.dmDisplayFlags & DM_INTERLACED) ? L"隔行" : L"逐行");
            }

            std::wstring mfg = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, keyPath, L"ProviderName");
            if (mfg.find(L"NVIDIA") != std::wstring::npos) info.manufacturer = L"NVIDIA";
            else if (mfg.find(L"AMD") != std::wstring::npos || mfg.find(L"Radeon") != std::wstring::npos) info.manufacturer = L"AMD";
            else if (mfg.find(L"Intel") != std::wstring::npos) info.manufacturer = L"Intel";
            break;
        }
    }

    return info;
}

// ── 内存信息 ──

MemoryInfo HardwareDetector::GetMemoryInfo() {
    MemoryInfo info;

    MEMORYSTATUSEX memStatus = { sizeof(memStatus) };
    if (GlobalMemoryStatusEx(&memStatus)) {
        info.totalPhysicalMB = memStatus.ullTotalPhys / (1024 * 1024);
        info.availablePhysicalMB = memStatus.ullAvailPhys / (1024 * 1024);
        info.totalVirtualMB = memStatus.ullTotalVirtual / (1024 * 1024);
        info.availableVirtualMB = memStatus.ullAvailVirtual / (1024 * 1024);
    }

    DWORD memSpeed = RegistryUtil::ReadDwordValue(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemMemorySpeed");
    if (memSpeed > 0) info.memorySpeed = memSpeed;

    std::wstring memType = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemMemoryType");
    info.memoryType = memType;

    auto memorySubKeys = RegistryUtil::EnumSubKeys(HKEY_LOCAL_MACHINE,
        L"HARDWARE\\Resources\\System\\Memory\\Memory Device");
    if (!memorySubKeys.empty()) {
        info.memorySlotCount = static_cast<int>(memorySubKeys.size());
        for (const auto& slot : memorySubKeys) {
            std::wstring slotPath = L"HARDWARE\\Resources\\System\\Memory\\Memory Device\\" + slot;
            std::wstring capacity = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, slotPath, L"Capacity");
            std::wstring speed = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, slotPath, L"Speed");
            std::wstring manufacturer = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, slotPath, L"Manufacturer");

            if (!capacity.empty()) {
                uint64_t capBytes = _wtoi64(capacity.c_str());
                if (capBytes > 0) {
                    info.memoryModuleCount++;
                    std::wstring moduleInfo;
                    if (capBytes >= 1024ULL * 1024 * 1024) {
                        moduleInfo = std::to_wstring(capBytes / (1024ULL * 1024 * 1024)) + L" GB";
                    } else {
                        moduleInfo = std::to_wstring(capBytes / (1024 * 1024)) + L" MB";
                    }
                    if (!speed.empty() && speed != L"0") {
                        moduleInfo += L" @ " + speed + L" MHz";
                    }
                    if (!manufacturer.empty() && manufacturer.find(L"Not Specified") == std::wstring::npos) {
                        moduleInfo += L"  " + manufacturer;
                    }
                    info.memoryModules.push_back(moduleInfo);
                }
            }
        }
    }

    return info;
}

// ── 磁盘信息 ──

std::vector<DiskInfo> HardwareDetector::GetDiskInfo() {
    std::vector<DiskInfo> disks;

    auto drives = Win32Util::GetAvailableDrives();
    std::wstring systemDrive = Win32Util::GetSystemDrive();

    for (const auto& drive : drives) {
        if (GetDriveTypeW(drive.c_str()) != DRIVE_FIXED) continue;

        DiskInfo disk;
        disk.driveLetter = drive.empty() ? L'C' : drive[0];

        std::wstring root = drive;
        if (!root.empty() && root.back() != L'\\') root += L'\\';
        disk.model = L"本地磁盘";

        wchar_t volumeName[MAX_PATH + 1] = {};
        if (GetVolumeInformationW(root.c_str(), volumeName, MAX_PATH, nullptr, nullptr, nullptr, nullptr, 0)) {
            if (volumeName[0] != L'\0') {
                disk.model = volumeName;
            }
        }

        disk.isSystemDisk = (_wcsicmp(drive.c_str(), systemDrive.c_str()) == 0);

        uint64_t totalBytes = 0, freeBytes = 0;
        if (Win32Util::GetDiskSpace(drive, totalBytes, freeBytes)) {
            disk.totalGB = totalBytes / (1024ULL * 1024 * 1024);
            disk.freeGB = freeBytes / (1024ULL * 1024 * 1024);
        }

        wchar_t fsName[32] = {};
        if (GetVolumeInformationW(root.c_str(), nullptr, 0, nullptr, nullptr, nullptr, fsName, 32)) {
            disk.fileSystem = fsName;
        }

        disk.isSSD = IsSSD(drive);
        disk.healthPercent = EstimateDiskHealth(drive);

        if (disk.isSSD) {
            disk.mediaType = L"SSD";
        } else {
            disk.mediaType = L"HDD";
        }
        disk.interfaceType = L"SATA / NVMe";

        disks.push_back(disk);
    }

    return disks;
}

// ── 主板信息 ──

MotherboardInfo HardwareDetector::GetMotherboardInfo() {
    MotherboardInfo info;
    info.manufacturer = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardManufacturer");
    info.product = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardProduct");
    info.version = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardVersion");
    info.serialNumber = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BaseBoardSerialNumber");
    info.biosVendor = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVendor");
    info.biosVersion = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVersion");
    info.biosDate = GetRegistryHardwareInfo(
        L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSReleaseDate");
    return info;
}

// ── 操作系统信息 ──

void HardwareDetector::GetOsInfo(std::wstring& osVersion, std::wstring& osBuild) {
    osVersion = Win32Util::GetWindowsVersion();
    osBuild = GetRegistryHardwareInfo(
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"CurrentBuild");
    std::wstring displayVersion = GetRegistryHardwareInfo(
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"DisplayVersion");
    if (!displayVersion.empty()) {
        osBuild += L" (" + displayVersion + L")";
    }
}

// ── 系统运行时间 ──

std::wstring HardwareDetector::GetSystemUptime() {
    uint64_t uptimeMs = GetTickCount64();
    uint64_t seconds = uptimeMs / 1000;
    uint64_t days = seconds / 86400;
    uint64_t hours = (seconds % 86400) / 3600;
    uint64_t minutes = (seconds % 3600) / 60;

    std::wstring result;
    if (days > 0) result += std::to_wstring(days) + L" 天 ";
    result += std::to_wstring(hours) + L" 小时 " + std::to_wstring(minutes) + L" 分钟";
    return result;
}

// ── WMI 查询 ──

bool HardwareDetector::QueryWmi(const std::wstring& wmiClass, const std::wstring& property,
                                 std::wstring& result) {
    // 简化：通过 PowerShell WMI 查询
    std::wstring cmd = L"powershell -NoProfile -Command \"(Get-CimInstance " + wmiClass +
                       L")." + property + L"\"";

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return false;

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, TRUE,
                         CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return false;
    }

    CloseHandle(hWritePipe);

    char buffer[4096] = {};
    DWORD bytesRead = 0;
    std::string output;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }

    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (!output.empty()) {
        size_t start = output.find_first_not_of(" \r\n\t");
        size_t end = output.find_last_not_of(" \r\n\t");
        if (start != std::wstring::npos && end != std::wstring::npos) {
            std::string trimmed = output.substr(start, end - start + 1);
            result = std::wstring(trimmed.begin(), trimmed.end());
            return !result.empty();
        }
    }

    return false;
}

// ── SSD 检测 ──

bool HardwareDetector::IsSSD(const std::wstring& driveLetter) {
    std::wstring volumePath = driveLetter;
    if (volumePath.back() != L'\\') volumePath += L'\\';

    // 通过查询磁盘属性判断是否 SSD
    HANDLE hVolume = CreateFileW(volumePath.c_str(), 0,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  nullptr, OPEN_EXISTING, 0, nullptr);
    if (hVolume == INVALID_HANDLE_VALUE) return false;

    STORAGE_PROPERTY_QUERY query = {};
    query.PropertyId = StorageDeviceSeekPenaltyProperty;
    query.QueryType = PropertyStandardQuery;

    DEVICE_SEEK_PENALTY_DESCRIPTOR descriptor = {};
    DWORD bytesReturned = 0;

    bool result = DeviceIoControl(hVolume, IOCTL_STORAGE_QUERY_PROPERTY,
                                   &query, sizeof(query),
                                   &descriptor, sizeof(descriptor),
                                   &bytesReturned, nullptr);
    CloseHandle(hVolume);

    // IncursSeekPenalty=0 means no seek penalty = SSD
    return result && !descriptor.IncursSeekPenalty;
}

// ── 磁盘健康度 ──

double HardwareDetector::EstimateDiskHealth(const std::wstring& driveLetter) {
    // 简化估算：基于磁盘使用时间和 SMART 状态
    // 真实实现应读取 SMART 属性
    return 95.0; // 默认返回 95%
}

// ── 注册表硬件信息 ──

std::wstring HardwareDetector::GetRegistryHardwareInfo(const std::wstring& keyPath,
                                                        const std::wstring& valueName) {
    return RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE, keyPath, valueName);
}

std::vector<NetworkAdapterDetail> HardwareDetector::GetNetworkAdaptersDetail() {
    std::vector<NetworkAdapterDetail> adapters;

    ULONG bufLen = 0;
    if (GetAdaptersInfo(nullptr, &bufLen) == ERROR_BUFFER_OVERFLOW) {
        auto* pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new uint8_t[bufLen]);
        if (GetAdaptersInfo(pAdapterInfo, &bufLen) == ERROR_SUCCESS) {
            for (auto* p = pAdapterInfo; p; p = p->Next) {
                if (p->Type != MIB_IF_TYPE_ETHERNET && p->Type != IF_TYPE_IEEE80211) continue;

                NetworkAdapterDetail info;
                info.description = std::wstring(p->Description, p->Description + strlen(p->Description));
                info.name = info.description;
                info.connectionType = (p->Type == IF_TYPE_IEEE80211) ? L"Wi-Fi" : L"以太网";

                if (p->AddressLength > 0) {
                    wchar_t mac[32];
                    swprintf_s(mac, L"%02X-%02X-%02X-%02X-%02X-%02X",
                        p->Address[0], p->Address[1], p->Address[2],
                        p->Address[3], p->Address[4], p->Address[5]);
                    info.macAddress = mac;
                }

                info.ipAddress = std::wstring(p->IpAddressList.IpAddress.String,
                    p->IpAddressList.IpAddress.String + strlen(p->IpAddressList.IpAddress.String));
                info.dhcpEnabled = (p->DhcpEnabled != 0);

                if (info.description.find(L"Realtek") != std::wstring::npos) info.manufacturer = L"Realtek";
                else if (info.description.find(L"Intel") != std::wstring::npos) info.manufacturer = L"Intel";
                else if (info.description.find(L"Broadcom") != std::wstring::npos) info.manufacturer = L"Broadcom";
                else if (info.description.find(L"Qualcomm") != std::wstring::npos) info.manufacturer = L"Qualcomm";
                else if (info.description.find(L"MediaTek") != std::wstring::npos) info.manufacturer = L"MediaTek";
                else info.manufacturer = L"Unknown";

                adapters.push_back(info);
            }
        }
        delete[] reinterpret_cast<uint8_t*>(pAdapterInfo);
    }

    return adapters;
}

} // namespace IceClean::Core::Analyzer
