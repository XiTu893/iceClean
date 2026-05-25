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
};

} // namespace IceClean::Utils
