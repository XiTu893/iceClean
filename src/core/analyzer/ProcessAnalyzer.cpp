#include "ProcessAnalyzer.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shlobj.h>
#include <algorithm>

namespace IceClean::Core::Analyzer {

std::vector<ProcessInfo> ProcessAnalyzer::GetRunningProcesses() {
    std::vector<ProcessInfo> processes;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return processes;

    PROCESSENTRY32W pe32{};
    pe32.dwSize = sizeof(pe32);

    if (Process32FirstW(snapshot, &pe32)) {
        do {
            ProcessInfo info;
            info.pid = pe32.th32ProcessID;
            info.name = pe32.szExeFile;
            info.memoryUsage = GetProcessMemoryUsage(pe32.th32ProcessID);

            // 获取进程完整路径
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                WCHAR pathBuffer[MAX_PATH] = {};
                DWORD pathSize = MAX_PATH;
                if (QueryFullProcessImageNameW(hProcess, 0, pathBuffer, &pathSize)) {
                    info.path = pathBuffer;
                    info.companyName = GetProcessCompanyName(info.path);
                }
                CloseHandle(hProcess);
            }

            // 判断是否系统进程
            info.isSystemProcess = IsSystemProcess(info.path);

            // 判断安全等级
            info.safety = GetProcessSafety(info.name, info.isSystemProcess);

            processes.push_back(info);
        } while (Process32NextW(snapshot, &pe32));
    }

    CloseHandle(snapshot);

    // 排序：非系统进程在前，系统进程在后
    std::sort(processes.begin(), processes.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        if (a.isSystemProcess != b.isSystemProcess) return !a.isSystemProcess;
        return a.memoryUsage > b.memoryUsage;
    });

    return processes;
}

bool ProcessAnalyzer::IsSystemProcess(const std::wstring& processPath) {
    if (processPath.empty()) return false;

    // 获取 Windows 目录路径
    WCHAR windowsDir[MAX_PATH] = {};
    GetWindowsDirectoryW(windowsDir, MAX_PATH);
    std::wstring winDir = windowsDir;
    std::transform(winDir.begin(), winDir.end(), winDir.begin(), ::towlower);

    std::wstring lowerPath = processPath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);

    // 路径在 Windows 目录下的是系统进程
    if (lowerPath.find(winDir) == 0) return true;

    // Program Files 下的 Microsoft 子目录也是系统进程
    WCHAR progFiles[MAX_PATH] = {};
    SHGetFolderPathW(nullptr, CSIDL_PROGRAM_FILES, nullptr, 0, progFiles);
    std::wstring msDir = progFiles;
    msDir += L"\\Microsoft";
    std::transform(msDir.begin(), msDir.end(), msDir.begin(), ::towlower);
    if (lowerPath.find(msDir) == 0) return true;

    return false;
}

ProcessSafety ProcessAnalyzer::GetProcessSafety(const std::wstring& name, bool isSystemProcess) {
    // 系统关键进程
    if (IsCriticalProcessName(name)) return ProcessSafety::Critical;

    // 系统进程但非关键
    if (isSystemProcess) return ProcessSafety::Caution;

    // 已知第三方守护进程（可安全终止）
    if (IsKnownThirdPartyGuard(name)) return ProcessSafety::Safe;

    // 其他第三方进程
    return ProcessSafety::Caution;
}

std::wstring ProcessAnalyzer::GetProcessCompanyName(const std::wstring& processPath) {
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(processPath.c_str(), &handle);
    if (size == 0) return L"";

    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoW(processPath.c_str(), 0, size, buffer.data())) return L"";

    WCHAR* companyName = nullptr;
    UINT len = 0;
    // 先尝试中文
    std::wstring subBlock = L"\\StringFileInfo\\080404b0\\CompanyName";
    if (!VerQueryValueW(buffer.data(), subBlock.c_str(), reinterpret_cast<void**>(&companyName), &len)) {
        // 尝试英文
        subBlock = L"\\StringFileInfo\\040904b0\\CompanyName";
        if (!VerQueryValueW(buffer.data(), subBlock.c_str(), reinterpret_cast<void**>(&companyName), &len)) {
            return L"";
        }
    }

    return companyName ? companyName : L"";
}

uint64_t ProcessAnalyzer::GetProcessMemoryUsage(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) return 0;

    PROCESS_MEMORY_COUNTERS_EX pmc{};
    pmc.cb = sizeof(pmc);
    BOOL result = GetProcessMemoryInfo(hProcess, reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc), sizeof(pmc));
    CloseHandle(hProcess);

    return result ? pmc.WorkingSetSize : 0;
}

bool ProcessAnalyzer::IsCriticalProcessName(const std::wstring& name) {
    // 系统关键进程列表（不可终止）
    static const std::vector<std::wstring> criticalProcesses = {
        L"smss.exe", L"csrss.exe", L"wininit.exe", L"services.exe",
        L"lsass.exe", L"svchost.exe", L"winlogon.exe", L"dwm.exe",
        L"explorer.exe", L"taskhostw.exe", L"RuntimeBroker.exe",
        L"SearchIndexer.exe", L"SearchHost.exe", L"ShellExperienceHost.exe",
        L"sihost.exe", L"taskeng.exe", L"conhost.exe", L"ctfmon.exe",
        L"fontdrvhost.exe", L"lsaiso.exe", L"MemCompression",
        L"Registry", L"System", L"System Idle Process",
        L"Secure System", L"vmmem",
    };

    std::wstring lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

    for (const auto& cp : criticalProcesses) {
        std::wstring lowerCp = cp;
        std::transform(lowerCp.begin(), lowerCp.end(), lowerCp.begin(), ::towlower);
        if (lowerName == lowerCp) return true;
    }
    return false;
}

bool ProcessAnalyzer::IsKnownThirdPartyGuard(const std::wstring& name) {
    // 已知第三方守护/保护进程（可安全终止）
    static const std::vector<std::wstring> guardProcesses = {
        L"QQProtect.exe",       // QQ安全防护
        L"QQPCRTP.exe",         // 腾讯电脑管家实时防护
        L"TXPlatform.exe",      // 腾讯平台
        L"TPHelper.exe",        // 腾讯TP
        L"TenSafe.exe",         // 腾讯安全
        L"AliProtect.exe",      // 阿里安全
        L"AlibabaProtect.exe",  // 阿里巴巴安全
        L"Alidetect.exe",       // 阿里检测
        L"TaobaoProtect.exe",   // 淘宝安全
        L"BaiduProtect.exe",    // 百度安全
        L"BaiduSdSvc.exe",      // 百度杀毒服务
        L"BDKitSvc.exe",        // 百度服务
        L"QiyiService.exe",     // 爱奇艺服务
        L"QyClient.exe",       // 爱奇艺客户端
        L"KwMusic.exe",         // 酷我音乐
        L"KuGou.exe",           // 酷狗音乐
        L"KuGouService.exe",    // 酷狗服务
        L"SogouCloud.exe",      // 搜狗云
        L"SogouSvc.exe",        // 搜狗服务
        L"SGTool.exe",          // 搜狗工具
        L"2345Explorer.exe",    // 2345浏览器
        L"2345PicSvc.exe",      // 2345看图服务
        L"HaoZipSvc.exe",       // 好压服务
        L"WPSOffice.exe",       // WPS
        L"wpscloudsvr.exe",     // WPS云服务
        L"wpscenter.exe",       // WPS中心
    };

    std::wstring lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

    for (const auto& gp : guardProcesses) {
        std::wstring lowerGp = gp;
        std::transform(lowerGp.begin(), lowerGp.end(), lowerGp.begin(), ::towlower);
        if (lowerName == lowerGp) return true;
    }
    return false;
}

} // namespace IceClean::Core::Analyzer
