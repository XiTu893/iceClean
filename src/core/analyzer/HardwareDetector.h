#pragma once
#include "models/HardwareInfo.h"
#include <string>

namespace IceClean::Core::Analyzer {

class HardwareDetector {
public:
    // 获取完整硬件摘要
    IceClean::Models::HardwareSummary GetSummary();

    // CPU 信息
    IceClean::Models::CpuInfo GetCpuInfo();

    // GPU 信息
    IceClean::Models::GpuInfo GetGpuInfo();

    // 内存信息
    IceClean::Models::MemoryInfo GetMemoryInfo();

    // 磁盘信息
    std::vector<IceClean::Models::DiskInfo> GetDiskInfo();

    // 主板信息
    IceClean::Models::MotherboardInfo GetMotherboardInfo();

    // 操作系统信息
    void GetOsInfo(std::wstring& osVersion, std::wstring& osBuild);

    // 系统运行时间
    std::wstring GetSystemUptime();

private:
    // 通过 WMI 查询获取信息
    bool QueryWmi(const std::wstring& wmiClass, const std::wstring& property,
                   std::wstring& result);

    // 判断磁盘是否为 SSD
    bool IsSSD(const std::wstring& driveLetter);

    // 获取磁盘健康度（简化：通过 SMART 基础判断）
    double EstimateDiskHealth(const std::wstring& driveLetter);

    // 获取注册表硬件信息
    std::wstring GetRegistryHardwareInfo(const std::wstring& keyPath, const std::wstring& valueName);
};

} // namespace IceClean::Core::Analyzer
