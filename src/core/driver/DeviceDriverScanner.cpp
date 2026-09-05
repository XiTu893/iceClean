#include "DeviceDriverScanner.h"
#include <windows.h>
#include <initguid.h>
#include <devpkey.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <algorithm>

// Fallback definitions for SDKs missing these
#ifndef DN_DISABLED
#define DN_DISABLED 0x00000001
#endif

// DEVPROPKEY for DriverSignature property (not in all SDK headers)
// {A45C254E-DF1C-4EFD-8020-67D146A850E0}, 13
// Uses direct struct init to avoid macro compatibility issues across SDK versions
const DEVPROPKEY DEVPKEY_Device_DriverSignature = {
    {0xa45c254e, 0xdf1c, 0x4efd, {0x80, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0}}, 13 };

namespace IceClean::Core::Driver {

namespace {

std::wstring ToWide(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                                      nullptr, 0);
    if (n == 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()),
                        out.data(), n);
    return out;
}

std::wstring Trim(const std::wstring& s) {
    const size_t b = s.find_first_not_of(L" \t\r\n");
    if (b == std::wstring::npos) return {};
    const size_t e = s.find_last_not_of(L" \t\r\n");
    return s.substr(b, e - b + 1);
}

} // namespace

std::wstring DeviceDriverScanner::GetStringProperty(HDEVINFO hdev,
                                                    PSP_DEVINFO_DATA devData,
                                                    const DEVPROPKEY& key) {
    DEVPROPTYPE type = 0;
    wchar_t buf[512] = {};
    DWORD needed = sizeof(buf);
    if (!SetupDiGetDevicePropertyW(hdev, devData, &key, &type,
                                   reinterpret_cast<PBYTE>(buf), sizeof(buf), &needed, 0)) {
        return {};
    }
    if (type != DEVPROP_TYPE_STRING || needed < sizeof(wchar_t)) return {};
    const DWORD len = (needed >= 2 && buf[needed / sizeof(wchar_t) - 1] == L'\0')
                      ? (needed / sizeof(wchar_t) - 1) : (needed / sizeof(wchar_t));
    return Trim(std::wstring(buf, len));
}

bool DeviceDriverScanner::GetFileTimeProperty(HDEVINFO hdev, PSP_DEVINFO_DATA devData,
                                              const DEVPROPKEY& key, FILETIME& out) {
    DEVPROPTYPE type = 0;
    DWORD needed = sizeof(out);
    if (!SetupDiGetDevicePropertyW(hdev, devData, &key, &type,
                                   reinterpret_cast<PBYTE>(&out), sizeof(out), &needed, 0)) {
        return false;
    }
    return type == DEVPROP_TYPE_FILETIME && needed == sizeof(FILETIME);
}

bool DeviceDriverScanner::GetUint32Property(HDEVINFO hdev, PSP_DEVINFO_DATA devData,
                                            const DEVPROPKEY& key, uint32_t& out) {
    DEVPROPTYPE type = 0;
    DWORD needed = sizeof(out);
    if (!SetupDiGetDevicePropertyW(hdev, devData, &key, &type,
                                   reinterpret_cast<PBYTE>(&out), sizeof(out), &needed, 0)) {
        return false;
    }
    return type == DEVPROP_TYPE_UINT32 && needed == sizeof(uint32_t);
}

std::wstring DeviceDriverScanner::FormatFileTime(const FILETIME& ft) {
    SYSTEMTIME st{};
    if (!FileTimeToSystemTime(&ft, &st)) return {};
    wchar_t buf[32] = {};
    swprintf_s(buf, L"%04d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
    return buf;
}

void DeviceDriverScanner::ResolveStatus(DeviceDriverInfo& info) {
    const bool disabled = (info.devStatus & DN_DISABLED) != 0;
    if (info.devProblem != 0) {
        switch (info.devProblem) {
        case CM_PROB_DISABLED: info.statusText = L"已禁用"; break;
        case CM_PROB_HARDWARE_DISABLED: info.statusText = L"硬件已禁用"; break;
        case CM_PROB_FAILED_INSTALL: info.statusText = L"安装失败"; break;
        case CM_PROB_FAILED_START: info.statusText = L"启动失败"; break;
        case CM_PROB_NEED_RESTART: info.statusText = L"需重启生效"; break;
        case CM_PROB_REINSTALL: info.statusText = L"需重新安装"; break;
        case CM_PROB_REGISTRY: info.statusText = L"注册表错误"; break;
        case CM_PROB_DRIVER_FAILED_PRIOR_UNLOAD:
        case CM_PROB_FAILED_DRIVER_ENTRY: info.statusText = L"驱动加载失败"; break;
        case CM_PROB_NOT_CONFIGURED:
        case CM_PROB_PARTIAL_LOG_CONF:
        case CM_PROB_NO_VALID_LOG_CONF: info.statusText = L"配置异常"; break;
        default: info.statusText = L"驱动异常"; break;
        }
    } else if (disabled) {
        info.statusText = L"已禁用";
    } else {
        info.statusText = L"正常";
    }

    // 第三方判定：非微软提供
    std::wstring p = info.provider;
    std::transform(p.begin(), p.end(), p.begin(), ::towlower);
    info.isThirdParty = p.find(L"microsoft") == std::wstring::npos && !p.empty();
}

std::vector<DeviceDriverInfo> DeviceDriverScanner::Enumerate() {
    std::vector<DeviceDriverInfo> result;

    const HDEVINFO hdev = SetupDiGetClassDevsW(
        nullptr, nullptr, nullptr, DIGCF_PRESENT | DIGCF_ALLCLASSES);
    if (hdev == INVALID_HANDLE_VALUE) {
        return result;
    }

    SP_DEVINFO_DATA devData{};
    devData.cbSize = sizeof(devData);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hdev, i, &devData); ++i) {
        DeviceDriverInfo info;

        info.deviceName = GetStringProperty(hdev, &devData, DEVPKEY_Device_FriendlyName);
        if (info.deviceName.empty()) {
            info.deviceName = GetStringProperty(hdev, &devData, DEVPKEY_Device_DeviceDesc);
        }
        info.manufacturer = GetStringProperty(hdev, &devData, DEVPKEY_Device_Manufacturer);
        info.deviceClass = GetStringProperty(hdev, &devData, DEVPKEY_Device_Class);
        info.driverDesc = GetStringProperty(hdev, &devData, DEVPKEY_Device_DriverDesc);
        info.provider = GetStringProperty(hdev, &devData, DEVPKEY_Device_DriverProvider);
        info.version = GetStringProperty(hdev, &devData, DEVPKEY_Device_DriverVersion);
        info.infPath = GetStringProperty(hdev, &devData, DEVPKEY_Device_DriverInfPath);

        FILETIME ft{};
        if (GetFileTimeProperty(hdev, &devData, DEVPKEY_Device_DriverDate, ft)) {
            info.date = FormatFileTime(ft);
        }

        uint32_t sig = 0;
        if (GetUint32Property(hdev, &devData, DEVPKEY_Device_DriverSignature, sig)) {
            switch (sig) {
            case 1: info.signature = DriverSignature::Unsigned; break;
            case 2: info.signature = DriverSignature::Signed; break;
            case 3: info.signature = DriverSignature::MicrosoftSigned; break;
            default: info.signature = DriverSignature::Unknown; break;
            }
        }

        // 从 infPath 提取 oemXX.inf
        const size_t sep = info.infPath.find_last_of(L"\\/");
        const std::wstring fileName = (sep == std::wstring::npos)
            ? info.infPath : info.infPath.substr(sep + 1);
        if (fileName.size() > 7 && fileName.substr(0, 3) == L"oem" &&
            fileName.substr(fileName.size() - 4) == L".inf") {
            info.oemInf = fileName;
        }

        // 设备状态/问题码
        wchar_t instanceId[256] = {};
        DWORD idSize = 256;
        if (SetupDiGetDeviceInstanceIdW(hdev, &devData, instanceId, idSize, &idSize)) {
            DEVINST devInst = 0;
            if (CM_Locate_DevNodeW(&devInst, instanceId, CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS) {
                CM_Get_DevNode_Status(reinterpret_cast<PULONG>(&info.devStatus),
                                       reinterpret_cast<PULONG>(&info.devProblem),
                                       devInst, 0);
            }
        }

        ResolveStatus(info);

        // 设备名与驱动描述至少其一非空才纳入
        if (!info.deviceName.empty() || !info.driverDesc.empty()) {
            result.push_back(std::move(info));
        }
    }

    SetupDiDestroyDeviceInfoList(hdev);
    return result;
}

} // namespace IceClean::Core::Driver
