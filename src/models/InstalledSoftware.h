#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <windows.h>

namespace IceClean::Models {

// 已安装软件信息
struct InstalledSoftware {
    std::wstring displayName;        // 软件名称
    std::wstring publisher;          // 发布者
    std::wstring version;            // 版本号
    std::wstring installDate;        // 安装日期
    std::wstring installLocation;    // 安装路径
    std::wstring uninstallString;    // 卸载命令
    std::wstring modifyPath;         // 修改命令(可选)
    uint64_t estimatedSize = 0;      // 估计大小(字节)
    std::wstring registryKeyPath;   // 注册表路径(用于定位)
    bool isSystemComponent = false;  // 是否系统组件(不应卸载)
    bool isUpdate = false;           // 是否是更新补丁
    HKEY rootKey = HKEY_LOCAL_MACHINE; // 注册表根键
};

} // namespace IceClean::Models
