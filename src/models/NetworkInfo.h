#pragma once
#include <string>
#include <vector>

namespace IceClean::Models {

// 网络适配器信息
struct NetworkAdapterInfo {
    std::wstring adapterName;       // 适配器名称
    std::wstring connectionName;     // 连接名称
    std::wstring macAddress;         // MAC地址
    std::wstring ipAddress;         // IP地址
    std::wstring subnetMask;        // 子网掩码
    std::wstring defaultGateway;    // 默认网关
    std::wstring dnsServers;        // DNS服务器
    bool isEnabled = true;          // 是否启用
    bool isVirtual = false;         // 是否虚拟适配器
};

// 网络优化项
struct NetworkOptimizeItem {
    std::wstring name;              // 优化项名称
    std::wstring description;       // 优化项描述
    std::wstring currentValue;      // 当前值
    std::wstring recommendedValue;  // 推荐值
    std::wstring registryPath;      // 注册表路径
    std::wstring valueName;         // 值名称
    bool needsOptimize = false;     // 是否需要优化
    bool isApplied = false;         // 是否已应用
};

// DNS配置
struct DnsConfig {
    std::wstring preferredDns;      // 首选DNS
    std::wstring alternateDns;      // 备用DNS
    std::wstring providerName;      // DNS提供商名称
};

} // namespace IceClean::Models
