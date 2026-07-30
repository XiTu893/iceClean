#pragma once
#include "models/NetworkInfo.h"
#include <vector>
#include <string>
#include <functional>

namespace IceClean::Core::Optimizer {

// 网络优化器
class NetworkOptimizer {
public:
    // 获取网络适配器列表
    std::vector<Models::NetworkAdapterInfo> GetNetworkAdapters();

    // 获取可优化项列表
    std::vector<Models::NetworkOptimizeItem> GetOptimizeItems();

    // 应用优化
    bool ApplyOptimize(const Models::NetworkOptimizeItem& item);

    // 批量应用优化
    int ApplyAllOptimizes(const std::vector<Models::NetworkOptimizeItem>& items);

    // 还原优化
    bool RevertOptimize(const Models::NetworkOptimizeItem& item);

    // 获取推荐DNS配置列表
    std::vector<Models::DnsConfig> GetRecommendedDnsConfigs();

    // 设置DNS服务器
    bool SetDnsServers(const std::wstring& adapterName,
                       const std::wstring& preferredDns,
                       const std::wstring& alternateDns);

    // 设置为自动获取DNS
    bool SetAutoDns(const std::wstring& adapterName);

    // 获取当前DNS设置
    std::wstring GetCurrentDns(const std::wstring& adapterName);

    // 网络测速(简单ping测试)
    int PingTest(const std::wstring& host);

    // 刷新DNS缓存
    bool FlushDnsCache();

    // 重置网络栈
    bool ResetNetworkStack();

    // 获取网络连接状态
    bool IsNetworkAvailable();

    // ── HOSTS 文件管理 ──
    struct HostsEntry {
        std::wstring ipAddress;
        std::wstring hostname;
        bool enabled = true;
        bool isSystemEntry = false;
    };

    // 获取 HOSTS 文件所有条目
    std::vector<HostsEntry> GetHostsEntries();

    // 添加 HOSTS 条目
    bool AddHostsEntry(const std::wstring& ipAddress, const std::wstring& hostname);

    // 删除 HOSTS 条目
    bool RemoveHostsEntry(const std::wstring& hostname);

    // 切换 HOSTS 条目启用状态（注释/取消注释）
    bool ToggleHostsEntry(const std::wstring& hostname, bool enable);

    // 阻止遥测的 HOSTS 条目
    bool AddTelemetryBlockEntries();

    // ── TCP 优化 ──
    // 优化 TCP/IP 参数
    bool OptimizeTcpParams();
};

} // namespace IceClean::Core::Optimizer
