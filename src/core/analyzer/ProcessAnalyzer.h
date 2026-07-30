#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <windows.h>

namespace IceClean::Core::Analyzer {

// 进程安全等级
enum class ProcessSafety {
    Safe,       // 可安全终止（第三方守护进程）
    Caution,    // 谨慎终止（可能有用的后台程序）
    Critical,   // 不可终止（系统关键进程）
};

// 进程信息
struct ProcessInfo {
    DWORD pid = 0;
    std::wstring name;           // 进程名 (QQProtect.exe)
    std::wstring path;           // 完整路径
    std::wstring companyName;    // 公司名称
    uint64_t memoryUsage = 0;    // 内存占用(bytes)
    ProcessSafety safety = ProcessSafety::Safe;
    bool isSystemProcess = false; // 是否系统自带
};

// 进程分析器
class ProcessAnalyzer {
public:
    // 获取所有运行中的进程
    static std::vector<ProcessInfo> GetRunningProcesses();

    // 判断进程是否为系统自带
    static bool IsSystemProcess(const std::wstring& processPath);

    // 获取进程安全等级
    static ProcessSafety GetProcessSafety(const std::wstring& processName, bool isSystemProcess);

    // 获取进程的公司名
    static std::wstring GetProcessCompanyName(const std::wstring& processPath);

    // 获取进程内存占用
    static uint64_t GetProcessMemoryUsage(DWORD pid);

private:
    // 系统关键进程白名单
    static bool IsCriticalProcessName(const std::wstring& name);

    // 已知第三方守护进程
    static bool IsKnownThirdPartyGuard(const std::wstring& name);
};

} // namespace IceClean::Core::Analyzer
