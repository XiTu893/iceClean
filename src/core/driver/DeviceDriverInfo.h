#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace IceClean::Core::Driver {

// 驱动签名状态（对应 DRIVER_SIGNATURE）
enum class DriverSignature {
    Unknown,        // 未知
    Unsigned,       // 未签名
    Signed,         // 已签名
    MicrosoftSigned // 微软签名
};

// 设备&驱动视图条目（来自 SetupDi 设备节点枚举）
struct DeviceDriverInfo {
    std::wstring deviceName;      // 设备友好名（DEVPKEY_Device_FriendlyName / DeviceDesc）
    std::wstring manufacturer;    // 厂商（DEVPKEY_Device_Manufacturer）
    std::wstring deviceClass;     // 设备类别（DEVPKEY_Device_Class）
    std::wstring driverDesc;      // 驱动描述（DEVPKEY_Device_DriverDesc）
    std::wstring provider;        // 提供商（DEVPKEY_Device_DriverProvider）
    std::wstring version;         // 版本（DEVPKEY_Device_DriverVersion）
    std::wstring date;            // 日期 yyyy-MM-dd（DEVPKEY_Device_DriverDate）
    std::wstring infPath;         // INF 完整路径（DEVPKEY_Device_DriverInfPath）
    std::wstring oemInf;          // 发布名称 oemXX.inf（从 infPath 提取）
    DriverSignature signature = DriverSignature::Unknown; // 签名状态
    uint32_t devStatus = 0;       // CM_Get_DevNode_Status 返回的状态标志
    uint32_t devProblem = 0;      // CM 问题码（CM_PROB_*）
    std::wstring statusText;      // 翻译后的可读状态（正常/已禁用/驱动异常...）
    bool isThirdParty = false;    // 是否非微软提供（视为第三方）
};

// 驱动包视图条目（来自 pnputil /enum-drivers，DriverStore 包视角）
struct DriverPackageInfo {
    std::wstring publishedName;   // 发布名称 oemXX.inf
    std::wstring originalName;    // 原始 INF 名
    std::wstring provider;        // 提供商
    std::wstring className;       // 类别
    std::wstring version;         // 版本
    std::wstring date;            // 日期
    bool signedDriver = false;    // 是否签名
    uint64_t estimatedSize = 0;   // 估算体积（对应 FileRepository 子目录）
};

// 将签名枚举转为可读文本
inline std::wstring SignatureToText(DriverSignature sig) {
    switch (sig) {
    case DriverSignature::Signed: return L"已签名";
    case DriverSignature::Unsigned: return L"未签名";
    case DriverSignature::MicrosoftSigned: return L"微软签名";
    case DriverSignature::Unknown:
    default: return L"未知";
    }
}

} // namespace IceClean::Core::Driver
