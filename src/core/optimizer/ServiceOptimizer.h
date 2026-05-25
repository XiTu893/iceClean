#pragma once
#include "models/StartupItem.h"
#include <vector>
#include <string>

namespace IceClean::Core::Optimizer {

// 服务信息
struct ServiceInfo {
    std::wstring serviceName;
    std::wstring displayName;
    DWORD status;           // SERVICE_*
    DWORD startType;        // SERVICE_*
    bool canDisable;
    bool isCritical;
};

class ServiceOptimizer {
public:
    // 获取所有服务列表
    std::vector<ServiceInfo> GetServices();

    // 获取可安全禁用的服务列表
    std::vector<Models::StartupItem> GetDisablableServices();

    // 禁用服务
    bool DisableService(const std::wstring& serviceName);

    // 启用服务(设为自动启动)
    bool EnableService(const std::wstring& serviceName);

    // 设置服务启动类型
    bool SetServiceStartType(const std::wstring& serviceName, DWORD startType);

private:
    // 判断服务是否可以安全禁用
    bool IsSafeToDisable(const std::wstring& serviceName) const;

    // 判断服务是否为关键服务(不可禁用)
    bool IsCriticalService(const std::wstring& serviceName) const;

    // 可安全禁用的服务列表
    static const std::vector<std::wstring>& GetSafeToDisableList();

    // 关键服务列表
    static const std::vector<std::wstring>& GetCriticalList();
};

} // namespace IceClean::Core::Optimizer
