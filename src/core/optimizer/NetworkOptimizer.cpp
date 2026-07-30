// winsock2.h MUST be included before windows.h to avoid redefinition errors
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include "NetworkOptimizer.h"
#include "utils/RegistryUtil.h"

#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>

namespace IceClean::Core::Optimizer {

std::vector<Models::NetworkAdapterInfo> NetworkOptimizer::GetNetworkAdapters() {
    std::vector<Models::NetworkAdapterInfo> adapters;

    ULONG bufLen = 0;
    GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_GATEWAYS, nullptr, nullptr, &bufLen);

    auto buffer = std::make_unique<BYTE[]>(bufLen);
    auto addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.get());

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_GATEWAYS, nullptr,
                              addresses, &bufLen) != ERROR_SUCCESS) {
        return adapters;
    }

    for (auto addr = addresses; addr; addr = addr->Next) {
        // 跳过回环接口
        if (addr->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

        Models::NetworkAdapterInfo info;
        // AdapterName is char*, convert to wstring
        if (addr->AdapterName) {
            std::string ansiName(addr->AdapterName);
            info.adapterName = std::wstring(ansiName.begin(), ansiName.end());
        }
        info.connectionName = addr->FriendlyName ? addr->FriendlyName : L"";
        info.isEnabled = addr->OperStatus == IfOperStatusUp;
        info.isVirtual = (addr->IfType == IF_TYPE_PPP ||
                          addr->PhysicalAddressLength == 0);

        // MAC地址
        if (addr->PhysicalAddressLength > 0 && addr->PhysicalAddressLength <= 6) {
            wchar_t macBuf[18] = {};
            for (ULONG i = 0; i < addr->PhysicalAddressLength; ++i) {
                swprintf_s(&macBuf[i * 3], _countof(macBuf) - i * 3, L"%02X-", addr->PhysicalAddress[i]);
            }
            macBuf[addr->PhysicalAddressLength * 3 - 1] = L'\0';
            info.macAddress = macBuf;
        }

        // IP地址
        for (auto ua = addr->FirstUnicastAddress; ua; ua = ua->Next) {
            wchar_t ipBuf[64] = {};
            if (ua->Address.lpSockaddr->sa_family == AF_INET) {
                auto sa = reinterpret_cast<SOCKADDR_IN*>(ua->Address.lpSockaddr);
                InetNtopW(AF_INET, &sa->sin_addr, ipBuf, 64);
                info.ipAddress = ipBuf;
                info.subnetMask = L"255.255.255.0"; // 简化
            }
        }

        // 默认网关
        for (auto gw = addr->FirstGatewayAddress; gw; gw = gw->Next) {
            wchar_t gwBuf[64] = {};
            if (gw->Address.lpSockaddr->sa_family == AF_INET) {
                auto sa = reinterpret_cast<SOCKADDR_IN*>(gw->Address.lpSockaddr);
                InetNtopW(AF_INET, &sa->sin_addr, gwBuf, 64);
                info.defaultGateway = gwBuf;
            }
        }

        // DNS服务器
        for (auto dns = addr->FirstDnsServerAddress; dns; dns = dns->Next) {
            wchar_t dnsBuf[64] = {};
            if (dns->Address.lpSockaddr->sa_family == AF_INET) {
                auto sa = reinterpret_cast<SOCKADDR_IN*>(dns->Address.lpSockaddr);
                InetNtopW(AF_INET, &sa->sin_addr, dnsBuf, 64);
                if (!info.dnsServers.empty()) info.dnsServers += L", ";
                info.dnsServers += dnsBuf;
            }
        }

        adapters.push_back(info);
    }

    return adapters;
}

std::vector<Models::NetworkOptimizeItem> NetworkOptimizer::GetOptimizeItems() {
    std::vector<Models::NetworkOptimizeItem> items;

    // 1. TCP自动调谐级别
    {
        Models::NetworkOptimizeItem item;
        item.name = L"TCP 接收窗口自动调谐";
        item.description = L"优化TCP接收窗口大小，提升网络传输效率";
        item.registryPath = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";
        item.valueName = L"AutoTuningLevel";
        item.recommendedValue = L"Normal";

        // 读取当前值
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, item.registryPath.c_str(),
                           0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t buf[256] = {};
            DWORD size = sizeof(buf);
            if (RegQueryValueExW(hKey, L"AutoTuningLevel", nullptr, nullptr,
                                  (LPBYTE)buf, &size) == ERROR_SUCCESS) {
                item.currentValue = buf;
            } else {
                item.currentValue = L"Normal (默认)";
            }
            RegCloseKey(hKey);
        }
        item.needsOptimize = (item.currentValue != L"Normal" && item.currentValue != L"Normal (默认)");
        items.push_back(item);
    }

    // 2. Nagle算法(禁用以降低延迟)
    {
        Models::NetworkOptimizeItem item;
        item.name = L"禁用 Nagle 算法";
        item.description = L"降低小包传输延迟，对游戏和实时应用有显著改善";
        item.registryPath = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces";
        item.valueName = L"TcpAckFrequency";
        item.recommendedValue = L"1";
        item.currentValue = L"默认(启用)";
        item.needsOptimize = true;
        items.push_back(item);
    }

    // 3. TCP Timestamps
    {
        Models::NetworkOptimizeItem item;
        item.name = L"TCP 时间戳";
        item.description = L"启用TCP时间戳有助于精确计算RTT，优化重传";
        item.registryPath = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";
        item.valueName = L"Tcp1323Opts";
        item.recommendedValue = L"2";
        item.currentValue = L"默认";
        item.needsOptimize = true;
        items.push_back(item);
    }

    // 4. DNS缓存优化
    {
        Models::NetworkOptimizeItem item;
        item.name = L"DNS 缓存优化";
        item.description = L"增大DNS缓存，减少域名解析时间";
        item.registryPath = L"SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters";
        item.valueName = L"CacheHashTableSize";
        item.recommendedValue = L"64";
        item.currentValue = L"默认";
        item.needsOptimize = true;
        items.push_back(item);
    }

    // 5. 网络节流指数
    {
        Models::NetworkOptimizeItem item;
        item.name = L"网络节流优化";
        item.description = L"降低系统对多媒体应用的网络节流限制";
        item.registryPath = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile";
        item.valueName = L"NetworkThrottlingIndex";
        item.recommendedValue = L"0xFFFFFFFF";
        item.currentValue = L"默认";

        HKEY hKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, item.registryPath.c_str(),
                           0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD val = 0;
            DWORD size = sizeof(val);
            if (RegQueryValueExW(hKey, L"NetworkThrottlingIndex", nullptr, nullptr,
                                  (LPBYTE)&val, &size) == ERROR_SUCCESS) {
                item.currentValue = std::to_wstring(val);
            }
            RegCloseKey(hKey);
        }
        item.needsOptimize = (item.currentValue != L"4294967295" && item.currentValue != L"0xFFFFFFFF");
        items.push_back(item);
    }

    return items;
}

bool NetworkOptimizer::ApplyOptimize(const Models::NetworkOptimizeItem& item) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, item.registryPath.c_str(),
                       0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        // 尝试创建键
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, item.registryPath.c_str(),
                             0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
            return false;
        }
    }

    DWORD value = 0;
    // 尝试将推荐值转为DWORD
    try {
        if (item.recommendedValue.find(L"0x") == 0 || item.recommendedValue.find(L"0X") == 0) {
            value = std::stoul(item.recommendedValue, nullptr, 16);
        } else {
            value = std::stoul(item.recommendedValue);
        }
    } catch (...) {
        // 字符串值
        RegSetValueExW(hKey, item.valueName.c_str(), 0, REG_SZ,
                        (const BYTE*)item.recommendedValue.c_str(),
                        static_cast<DWORD>((item.recommendedValue.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
        return true;
    }

    bool ok = RegSetValueExW(hKey, item.valueName.c_str(), 0, REG_DWORD,
                              (const BYTE*)&value, sizeof(value)) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return ok;
}

int NetworkOptimizer::ApplyAllOptimizes(const std::vector<Models::NetworkOptimizeItem>& items) {
    int count = 0;
    for (const auto& item : items) {
        if (item.needsOptimize && ApplyOptimize(item)) {
            count++;
        }
    }
    return count;
}

bool NetworkOptimizer::RevertOptimize(const Models::NetworkOptimizeItem& item) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, item.registryPath.c_str(),
                       0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    bool ok = RegDeleteValueW(hKey, item.valueName.c_str()) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return ok;
}

std::vector<Models::DnsConfig> NetworkOptimizer::GetRecommendedDnsConfigs() {
    return {
        { L"223.5.5.5",   L"223.6.6.6",     L"阿里 DNS" },
        { L"119.29.29.29", L"182.254.116.116", L"腾讯 DNS" },
        { L"180.76.76.76", L"114.114.114.114", L"百度 DNS" },
        { L"8.8.8.8",     L"8.8.4.4",       L"Google DNS" },
        { L"1.1.1.1",     L"1.0.0.1",       L"Cloudflare DNS" },
    };
}

bool NetworkOptimizer::SetDnsServers(const std::wstring& adapterName,
                                      const std::wstring& preferredDns,
                                      const std::wstring& alternateDns) {
    // 使用 netsh 命令设置DNS
    std::wstring cmd = L"netsh interface ip set dns \"" + adapterName +
                       L"\" static " + preferredDns + L" primary";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                         FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (!alternateDns.empty()) {
        cmd = L"netsh interface ip add dns \"" + adapterName +
              L"\" " + alternateDns + L" index=2";
        if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                             FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
            return false;
        }
        WaitForSingleObject(pi.hProcess, 10000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return true;
}

bool NetworkOptimizer::SetAutoDns(const std::wstring& adapterName) {
    std::wstring cmd = L"netsh interface ip set dns \"" + adapterName + L"\" dhcp";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                         FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

std::wstring NetworkOptimizer::GetCurrentDns(const std::wstring& adapterName) {
    // 通过 netsh 显示DNS配置
    std::wstring cmd = L"netsh interface ip show dns \"" + adapterName + L"\"";

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hReadPipe, hWritePipe;
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                         TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return L"";
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

    // 转换为宽字符串并解析DNS
    int wlen = MultiByteToWideChar(CP_ACP, 0, output.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return L"";
    auto wstr = std::make_unique<wchar_t[]>(wlen);
    MultiByteToWideChar(CP_ACP, 0, output.c_str(), -1, wstr.get(), wlen);

    // 简化：返回整个输出
    return wstr.get();
}

int NetworkOptimizer::PingTest(const std::wstring& host) {
    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) return -1;

    IPAddr destAddr = 0;
    ADDRINFOW* addrInfo = nullptr;
    if (GetAddrInfoW(host.c_str(), nullptr, nullptr, &addrInfo) == 0) {
        for (auto p = addrInfo; p; p = p->ai_next) {
            if (p->ai_family == AF_INET) {
                auto sa = reinterpret_cast<SOCKADDR_IN*>(p->ai_addr);
                destAddr = sa->sin_addr.S_un.S_addr;
                break;
            }
        }
        FreeAddrInfoW(addrInfo);
    }

    if (destAddr == 0) {
        IcmpCloseHandle(hIcmp);
        return -1;
    }

    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + 8;
    auto replyBuf = std::make_unique<BYTE[]>(replySize);

    auto start = std::chrono::high_resolution_clock::now();
    DWORD result = IcmpSendEcho(hIcmp, destAddr, nullptr, 0, nullptr,
                                 replyBuf.get(), replySize, 3000);
    auto end = std::chrono::high_resolution_clock::now();

    IcmpCloseHandle(hIcmp);

    if (result == 0) return -1;

    auto reply = reinterpret_cast<PICMP_ECHO_REPLY>(replyBuf.get());
    if (reply->Status == IP_SUCCESS) {
        return static_cast<int>(reply->RoundTripTime);
    }

    return -1;
}

bool NetworkOptimizer::FlushDnsCache() {
    std::wstring cmd = L"ipconfig /flushdns";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                         FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

bool NetworkOptimizer::ResetNetworkStack() {
    std::wstring cmd = L"netsh int ip reset";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                         FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, 30000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

bool NetworkOptimizer::IsNetworkAvailable() {
    // Use Network API instead of InternetGetConnectedState (requires wininet.h)
    ULONG outBufLen = 0;
    DWORD ret = GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, nullptr, &outBufLen);
    if (ret != ERROR_BUFFER_OVERFLOW) return false;

    auto buffer = std::make_unique<BYTE[]>(outBufLen);
    auto addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.get());

    if (GetAdaptersAddresses(AF_UNSPEC, 0, nullptr, addresses, &outBufLen) != ERROR_SUCCESS) {
        return false;
    }

    for (auto addr = addresses; addr; addr = addr->Next) {
        if (addr->OperStatus == IfOperStatusUp && addr->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
            return true;
        }
    }
    return false;
}

// ── HOSTS 文件管理 ──

std::vector<NetworkOptimizer::HostsEntry> NetworkOptimizer::GetHostsEntries() {
    std::vector<HostsEntry> entries;
    std::wstring hostsPath = L"C:\\Windows\\System32\\drivers\\etc\\hosts";

    std::ifstream ifs(hostsPath);
    if (!ifs.is_open()) return entries;

    std::string line;
    int lineNum = 0;
    while (std::getline(ifs, line)) {
        lineNum++;
        HostsEntry entry;

        std::string trimmed = line;
        // 移除首尾空格
        size_t start = trimmed.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            // 空行
            continue;
        }

        if (trimmed[start] == '#') {
            entry.enabled = false;
            trimmed = trimmed.substr(start + 1);
        }

        // 解析 IP 和 hostname
        std::istringstream iss(trimmed);
        std::string ip, hostname;
        iss >> ip >> hostname;

        if (!ip.empty() && !hostname.empty()) {
            entry.ipAddress = std::wstring(ip.begin(), ip.end());
            entry.hostname = std::wstring(hostname.begin(), hostname.end());
            entry.isSystemEntry = (hostname == "localhost" || hostname == "localhost.localdomain");
            entries.push_back(entry);
        }
    }

    return entries;
}

bool NetworkOptimizer::AddHostsEntry(const std::wstring& ipAddress, const std::wstring& hostname) {
    std::wstring hostsPath = L"C:\\Windows\\System32\\drivers\\etc\\hosts";

    std::wofstream ofs(hostsPath, std::ios::app);
    if (!ofs.is_open()) return false;

    ofs << std::endl << ipAddress << L"\t" << hostname;
    ofs.close();
    return true;
}

bool NetworkOptimizer::RemoveHostsEntry(const std::wstring& hostname) {
    std::wstring hostsPath = L"C:\\Windows\\System32\\drivers\\etc\\hosts";

    std::ifstream ifs(hostsPath);
    if (!ifs.is_open()) return false;

    std::vector<std::wstring> lines;
    std::string line;
    while (std::getline(ifs, line)) {
        std::wstring wline(line.begin(), line.end());
        if (wline.find(hostname) == std::wstring::npos) {
            lines.push_back(wline);
        }
    }
    ifs.close();

    std::wofstream ofs(hostsPath);
    if (!ofs.is_open()) return false;

    for (const auto& l : lines) {
        ofs << l << std::endl;
    }
    ofs.close();
    return true;
}

bool NetworkOptimizer::ToggleHostsEntry(const std::wstring& hostname, bool enable) {
    std::wstring hostsPath = L"C:\\Windows\\System32\\drivers\\etc\\hosts";

    std::ifstream ifs(hostsPath);
    if (!ifs.is_open()) return false;

    std::vector<std::wstring> lines;
    std::string line;
    while (std::getline(ifs, line)) {
        std::wstring wline(line.begin(), line.end());
        if (wline.find(hostname) != std::wstring::npos) {
            if (enable) {
                // 取消注释
                if (!wline.empty() && wline[0] == L'#') {
                    wline = wline.substr(1);
                }
            } else {
                // 注释
                if (wline.empty() || wline[0] != L'#') {
                    wline = L"#" + wline;
                }
            }
        }
        lines.push_back(wline);
    }
    ifs.close();

    std::wofstream ofs(hostsPath);
    if (!ofs.is_open()) return false;

    for (const auto& l : lines) {
        ofs << l << std::endl;
    }
    ofs.close();
    return true;
}

bool NetworkOptimizer::AddTelemetryBlockEntries() {
    std::vector<std::pair<std::wstring, std::wstring>> telemetryHosts = {
        { L"0.0.0.0", L"vortex.data.microsoft.com" },
        { L"0.0.0.0", L"vortex10.data.microsoft.com" },
        { L"0.0.0.0", L"vortex15.data.microsoft.com" },
        { L"0.0.0.0", L"settings-win.data.microsoft.com" },
        { L"0.0.0.0", L"watson.telemetry.microsoft.com" },
        { L"0.0.0.0", L"watson.telemetry.microsoft.com.nsatc.net" },
        { L"0.0.0.0", L"sqm.telemetry.microsoft.com" },
        { L"0.0.0.0", L"sqm.telemetry.microsoft.com.nsatc.net" },
        { L"0.0.0.0", L"telecommand.telemetry.microsoft.com" },
        { L"0.0.0.0", L"telecommand.telemetry.microsoft.com.nsatc.net" },
        { L"0.0.0.0", L"oca.telemetry.microsoft.com" },
        { L"0.0.0.0", L"oca.telemetry.microsoft.com.nsatc.net" },
        { L"0.0.0.0", L"compatexchange.cloud.microsoft.com" },
        { L"0.0.0.0", L"ceuswatcab01.blob.core.windows.net" },
        { L"0.0.0.0", L"ceuswatcab02.blob.core.windows.net" },
        { L"0.0.0.0", L"eauswatcab01.blob.core.windows.net" },
        { L"0.0.0.0", L"eauswatcab02.blob.core.windows.net" },
        { L"0.0.0.0", L"weuswatcab01.blob.core.windows.net" },
        { L"0.0.0.0", L"weuswatcab02.blob.core.windows.net" },
        { L"0.0.0.0", L"pingma.azureedge.net" },
        { L"0.0.0.0", L"client.wns.windows.com" },
        { L"0.0.0.0", L"dns.msftncsi.com" },
        { L"0.0.0.0", L"www.msftncsi.com" },
        { L"0.0.0.0", L"cdp.cloud.microsoft.com" },
    };

    auto existing = GetHostsEntries();
    int added = 0;

    for (const auto& [ip, host] : telemetryHosts) {
        bool exists = false;
        for (const auto& entry : existing) {
            if (entry.hostname == host) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            if (AddHostsEntry(ip, host)) added++;
        }
    }

    return added > 0;
}

// ── TCP 优化 ──

bool NetworkOptimizer::OptimizeTcpParams() {
    bool allOk = true;

    // 1. 设置 TCP 自动调谐级别为 Normal
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD val = 0; // Normal
        if (RegSetValueExW(hKey, L"AutoTuningLevel", 0, REG_DWORD,
                            (const BYTE*)&val, sizeof(val)) != ERROR_SUCCESS) {
            allOk = false;
        }
        RegCloseKey(hKey);
    } else {
        allOk = false;
    }

    // 2. TCP 时间戳启用
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD val = 2; // 启用时间戳和 PAWS
        if (RegSetValueExW(hKey, L"Tcp1323Opts", 0, REG_DWORD,
                            (const BYTE*)&val, sizeof(val)) != ERROR_SUCCESS) {
            allOk = false;
        }
        RegCloseKey(hKey);
    } else {
        allOk = false;
    }

    // 3. 网络节流索引优化
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Multimedia\\SystemProfile",
        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD val = 0xFFFFFFFF;
        if (RegSetValueExW(hKey, L"NetworkThrottlingIndex", 0, REG_DWORD,
                            (const BYTE*)&val, sizeof(val)) != ERROR_SUCCESS) {
            allOk = false;
        }
        RegCloseKey(hKey);
    } else {
        allOk = false;
    }

    // 4. DNS 缓存大小优化
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters",
        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD val = 64;
        if (RegSetValueExW(hKey, L"CacheHashTableSize", 0, REG_DWORD,
                            (const BYTE*)&val, sizeof(val)) != ERROR_SUCCESS) {
            allOk = false;
        }
        RegCloseKey(hKey);
    }

    return allOk;
}

} // namespace IceClean::Core::Optimizer
