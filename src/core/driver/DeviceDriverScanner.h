#pragma once
#include <vector>
#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include "core/driver/DeviceDriverInfo.h"

namespace IceClean::Core::Driver {

// 设备&驱动扫描器：基于 SetupDi + CM 枚举当前已呈现设备节点及其驱动信息。
// 只读、无副作用，是本工具"先显示已安装设备和驱动"的主数据源。
class DeviceDriverScanner {
public:
    // 枚举全部当前已呈现的设备节点及关联驱动信息
    static std::vector<DeviceDriverInfo> Enumerate();

private:
    // 取字符串型设备属性（DEVPROP_TYPE_STRING）
    static std::wstring GetStringProperty(HDEVINFO hdev, PSP_DEVINFO_DATA devData,
                                          const DEVPROPKEY& key);

    // 取 FILETIME 型设备属性（如驱动日期）
    static bool GetFileTimeProperty(HDEVINFO hdev, PSP_DEVINFO_DATA devData,
                                    const DEVPROPKEY& key, FILETIME& out);

    // 取 UINT32 型设备属性（如签名状态）
    static bool GetUint32Property(HDEVINFO hdev, PSP_DEVINFO_DATA devData,
                                  const DEVPROPKEY& key, uint32_t& out);

    // 将 FILETIME 格式化为 yyyy-MM-dd
    static std::wstring FormatFileTime(const FILETIME& ft);

    // 将 CM 状态/问题码翻译为可读文本，并判定是否第三方
    static void ResolveStatus(DeviceDriverInfo& info);
};

} // namespace IceClean::Core::Driver
