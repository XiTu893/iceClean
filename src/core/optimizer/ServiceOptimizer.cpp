#include "ServiceOptimizer.h"
#include <windows.h>
#include <winsvc.h>
#include <algorithm>

#pragma comment(lib, "advapi32.lib")

namespace IceClean::Core::Optimizer {

const std::vector<std::wstring>& ServiceOptimizer::GetSafeToDisableList() {
    static const std::vector<std::wstring> services = {
        // 诊断跟踪
        L"DiagTrack",                    // Connected User Experiences and Telemetry
        L"dmwappushservice",             // WAP Push Message Routing
        // 错误报告
        L"WerSvc",                       // Windows Error Reporting
        // Xbox相关
        L"XblAuthManager",              // Xbox Live Auth Manager
        L"XblGameSave",                 // Xbox Live Game Save
        L"XboxNetApiSvc",               // Xbox Live Networking
        L"XboxGipSvc",                  // Xbox Accessory Management
        // 打印相关(如无打印机)
        L"Spooler",                     // Print Spooler
        L"PrintNotify",                 // Printer Extensions and Notifications
        // 传真
        L"Fax",                         // Fax
        // Windows搜索(可按需禁用)
        L"WSearch",                     // Windows Search
        // 超级预取(SSD可禁用)
        L"SysMain",                     // SysMain (Superfetch)
        // Windows Insider
        L"wisvc",                       // Windows Insider Service
        // 地理位置服务
        L"lfsvc",                       // Geolocation Service
        // 远程注册表
        L"RemoteRegistry",              // Remote Registry
        // 二次登录
        L"seclogon",                    // Secondary Logon
        // Telnet
        L"TlnkSvr",                     // Telnet
        // 家庭组
        L"HomeGroupListener",           // HomeGroup Listener
        L"HomeGroupProvider",           // HomeGroup Provider
        // 生物识别(如无指纹/面部识别)
        L"WbioService",                 // Windows Biometric Service
        // 下载地图
        L"MapsBroker",                  // Downloaded Maps Manager
        // 零售演示
        L"RetailDemo",                  // Retail Demo
        // 支付和NFC
        L"SEMgrSvc",                    // Payments and NFC/SE Manager
    };
    return services;
}

const std::vector<std::wstring>& ServiceOptimizer::GetCriticalList() {
    static const std::vector<std::wstring> services = {
        // 系统核心
        L"EventLog",                    // Windows Event Log
        L"Winmgmt",                     // Windows Management Instrumentation
        L"RpcSs",                       // Remote Procedure Call (RPC)
        L"DcomLaunch",                  // DCOM Server Process Launcher
        L"RpcEptMapper",                // RPC Endpoint Mapper
        L"LSM",                         // Local Session Manager
        L"WinInit",                     // Windows Start-Up Application
        L"Winlogon",                    // Windows Logon
        L"Schedule",                    // Task Scheduler
        L"ProfSvc",                     // User Profile Service
        L"PlugPlay",                    // Plug and Play
        L"mpssvc",                      // Windows Firewall
        // 安全相关
        L"WinDefend",                   // Windows Defender
        L"WdNisSvc",                    // Windows Defender Network Inspection
        L"SecurityHealthService",       // Windows Security Service
        L"MsMpEng",                     // Windows Defender Antivirus
        // 网络
        L"Dhcp",                        // DHCP Client
        L"Dnscache",                    // DNS Client
        L"NSI",                         // Network Store Interface
        L"LanmanServer",                // Server
        L"LanmanWorkstation",           // Workstation
        L"Browser",                     // Computer Browser
        // 存储
        L"vds",                         // Virtual Disk
        L"MountMgr",                    // Mount Manager
        // 其他关键
        L"Power",                       // Power
        L"SpbClassifier",              // Storage Service
        L"StorSvc",                     // Storage Service
        L"Themes",                      // Themes
        L"UserManager",                 // User Manager
        L"FontCache",                   // Windows Font Cache Service
    };
    return services;
}

bool ServiceOptimizer::IsSafeToDisable(const std::wstring& serviceName) const {
    std::wstring lower = serviceName;
    std::transform(lower.begin(), lower.end(), lower.begin(), towlower);

    for (const auto& safe : GetSafeToDisableList()) {
        if (_wcsicmp(lower.c_str(), safe.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

bool ServiceOptimizer::IsCriticalService(const std::wstring& serviceName) const {
    std::wstring lower = serviceName;
    std::transform(lower.begin(), lower.end(), lower.begin(), towlower);

    for (const auto& critical : GetCriticalList()) {
        if (_wcsicmp(lower.c_str(), critical.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

std::vector<ServiceInfo> ServiceOptimizer::GetServices() {
    std::vector<ServiceInfo> services;

    SC_HANDLE hScManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!hScManager) return services;

    // 先获取所需缓冲区大小
    DWORD bytesNeeded = 0;
    DWORD servicesReturned = 0;
    DWORD resumeHandle = 0;

    BOOL result = EnumServicesStatusExW(
        hScManager,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_STATE_ALL,
        nullptr,
        0,
        &bytesNeeded,
        &servicesReturned,
        &resumeHandle,
        nullptr);

    if (!result && GetLastError() != ERROR_MORE_DATA) {
        CloseServiceHandle(hScManager);
        return services;
    }

    // 分配缓冲区并枚举服务
    std::vector<BYTE> buffer(bytesNeeded);
    servicesReturned = 0;
    resumeHandle = 0;

    result = EnumServicesStatusExW(
        hScManager,
        SC_ENUM_PROCESS_INFO,
        SERVICE_WIN32,
        SERVICE_STATE_ALL,
        buffer.data(),
        bytesNeeded,
        &bytesNeeded,
        &servicesReturned,
        &resumeHandle,
        nullptr);

    CloseServiceHandle(hScManager);

    if (!result) return services;

    auto* serviceEntries = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW*>(buffer.data());

    for (DWORD i = 0; i < servicesReturned; ++i) {
        ServiceInfo info;
        info.serviceName = serviceEntries[i].lpServiceName;
        info.displayName = serviceEntries[i].lpDisplayName;
        info.status = serviceEntries[i].ServiceStatusProcess.dwCurrentState;
        info.isCritical = IsCriticalService(info.serviceName);
        info.canDisable = !info.isCritical;

        // 获取启动类型
        SC_HANDLE hScManager2 = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (hScManager2) {
            SC_HANDLE hService = OpenServiceW(hScManager2, info.serviceName.c_str(), SERVICE_QUERY_CONFIG);
            if (hService) {
                DWORD bytesNeeded2 = 0;
                QueryServiceConfigW(hService, nullptr, 0, &bytesNeeded2);
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    std::vector<BYTE> configBuffer(bytesNeeded2);
                    auto* config = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(configBuffer.data());
                    if (QueryServiceConfigW(hService, config, bytesNeeded2, &bytesNeeded2)) {
                        info.startType = config->dwStartType;
                    }
                }
                CloseServiceHandle(hService);
            }
            CloseServiceHandle(hScManager2);
        }

        services.push_back(info);
    }

    return services;
}

std::vector<Models::StartupItem> ServiceOptimizer::GetDisablableServices() {
    std::vector<Models::StartupItem> items;

    auto services = GetServices();
    for (const auto& svc : services) {
        // 只显示自动启动且可安全禁用的服务
        if (svc.startType != SERVICE_AUTO_START) continue;
        if (svc.isCritical) continue;
        if (!IsSafeToDisable(svc.serviceName)) continue;

        Models::StartupItem item;
        item.name = svc.displayName;
        item.path = svc.serviceName;
        item.type = Models::StartupItemType::Service;
        item.isEnabled = (svc.status == SERVICE_RUNNING);
        item.isSystemCritical = false;
        item.canDisable = true;

        items.push_back(item);
    }

    return items;
}

bool ServiceOptimizer::SetServiceStartType(const std::wstring& serviceName, DWORD startType) {
    SC_HANDLE hScManager = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hScManager) return false;

    SC_HANDLE hService = OpenServiceW(hScManager, serviceName.c_str(), SERVICE_CHANGE_CONFIG);
    if (!hService) {
        CloseServiceHandle(hScManager);
        return false;
    }

    BOOL result = ChangeServiceConfigW(
        hService,
        SERVICE_NO_CHANGE,     // dwServiceType
        startType,             // dwStartType
        SERVICE_NO_CHANGE,     // dwErrorControl
        nullptr,               // lpBinaryPathName
        nullptr,               // lpLoadOrderGroup
        nullptr,               // lpdwTagId
        nullptr,               // lpDependencies
        nullptr,               // lpServiceStartName
        nullptr,               // lpPassword
        nullptr                // lpDisplayName
    );

    CloseServiceHandle(hService);
    CloseServiceHandle(hScManager);

    return result != 0;
}

bool ServiceOptimizer::DisableService(const std::wstring& serviceName) {
    if (IsCriticalService(serviceName)) return false;
    return SetServiceStartType(serviceName, SERVICE_DISABLED);
}

bool ServiceOptimizer::EnableService(const std::wstring& serviceName) {
    return SetServiceStartType(serviceName, SERVICE_AUTO_START);
}

} // namespace IceClean::Core::Optimizer
