#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace IceClean::Utils {

class Win32Util {
public:
    // 检查是否以管理员权限运行
    static bool IsRunningAsAdmin();

    // 获取Windows版本
    static std::wstring GetWindowsVersion();

    // 获取系统盘符(通常是C:)
    static std::wstring GetSystemDrive();

    // 获取所有可用盘符
    static std::vector<std::wstring> GetAvailableDrives();

    // 获取磁盘空间信息
    static bool GetDiskSpace(const std::wstring& drive, uint64_t& totalBytes, uint64_t& freeBytes);

    // 获取特殊文件夹路径
    static std::wstring GetSpecialFolder(int csidl);

    // 展开环境变量(如%TEMP%, %LocalAppData%)
    static std::wstring ExpandEnvVars(const std::wstring& path);

    // 检查进程是否正在运行
    static bool IsProcessRunning(const std::wstring& processName);

    // 获取当前进程ID列表
    static std::vector<DWORD> GetProcessIdsByName(const std::wstring& processName);

    // 终止指定名称的进程（需要管理员权限）
    static bool KillProcessByName(const std::wstring& processName);

    // 终止指定PID的进程
    static bool KillProcessById(DWORD pid);

    // 强制终止指定PID的进程（启用SeDebugPrivilege，可终止受保护进程）
    static bool ForceKillProcessById(DWORD pid);

    // 通过计划任务以SYSTEM身份终止进程（终极手段，可终止几乎所有用户态进程）
    static bool KillProcessAsSystem(const std::wstring& processName);

    // 启用当前进程的SeDebugPrivilege（调试权限）
    static bool EnableDebugPrivilege();

    // 从程序路径中提取进程名（如 "C:\\QQ\\QQProtect.exe" → "QQProtect.exe"）
    static std::wstring ExtractProcessName(const std::wstring& path);

    // 强制删除文件（先终止占用进程再删除）
    static bool ForceDeleteFile(const std::wstring& path);

    // 强制删除目录（递归终止占用进程+删除）
    static bool ForceDeleteDirectory(const std::wstring& path);
};

} // namespace IceClean::Utils
