#include "DiagnosticReporter.h"
#include "core/safety/OperationLogger.h"
#include "core/safety/RollbackManager.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <chrono>
#include <windows.h>
#include <shlobj.h>
#include <psapi.h>

namespace IceClean::Core::Safety {

using json = nlohmann::json;

// 版本号定义（与 resource.h 保持同步）
#ifndef APP_VERSION_A
#define APP_VERSION_A "1.0.0.0"
#endif

// ── 生成完整报告 ──

std::wstring DiagnosticReporter::GenerateReport() {
    json report;

    // 基本信息
    report["appName"] = "IceClean";
    report["appVersion"] = std::string(APP_VERSION_A);
    auto now = std::chrono::system_clock::now();
    report["timestamp"] = std::chrono::system_clock::to_time_t(now);

    // 系统信息
    report["system"] = json::parse(CollectSystemInfo());

    // 磁盘信息
    report["disk"] = json::parse(CollectDiskInfo());

    // 操作日志
    report["operations"] = json::parse(CollectOperationLog(20));

    // 安全状态
    report["security"] = json::parse(CollectSecurityStatus());

    // 进程信息
    report["processes"] = json::parse(CollectProcessInfo());

    // 启动项
    report["startup"] = json::parse(CollectStartupInfo());

    // 保存报告
    auto reportDir = GetReportDirectory();
    CreateDirectoryW(reportDir.c_str(), NULL);

    auto timeT = std::chrono::system_clock::to_time_t(now);
    struct tm tmBuf;
    localtime_s(&tmBuf, &timeT);
    wchar_t timeStr[64];
    wcsftime(timeStr, 64, L"%Y%m%d_%H%M%S", &tmBuf);

    auto filePath = reportDir + L"\\diagnostic_" + timeStr + L".json";

    std::ofstream file(filePath);
    if (file.is_open()) {
        file << report.dump(2);
        spdlog::info("诊断报告已生成: {}", std::string(filePath.begin(), filePath.end()));
    }

    return filePath;
}

// ── 系统信息 ──

std::wstring DiagnosticReporter::GetSystemInfo() {
    auto info = CollectSystemInfo();
    return std::wstring(info.begin(), info.end());
}

std::string DiagnosticReporter::CollectSystemInfo() {
    json sysInfo;

    // 操作系统版本
    OSVERSIONINFOEXW osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    // 使用 RtlGetVersion (更可靠)
    using RtlGetVersionPtr = NTSTATUS(WINAPI*)(OSVERSIONINFOEXW*);
    auto ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        auto RtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(
            GetProcAddress(ntdll, "RtlGetVersion"));
        if (RtlGetVersion) {
            RtlGetVersion(&osvi);
        }
    }

    sysInfo["osVersion"] = std::to_string(osvi.dwMajorVersion) + "." +
                           std::to_string(osvi.dwMinorVersion) + "." +
                           std::to_string(osvi.dwBuildNumber);
    sysInfo["osArch"] = sizeof(void*) == 8 ? "x64" : "x86";

    // 内存信息
    MEMORYSTATUSEX memInfo = {0};
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);

    sysInfo["totalMemoryGB"] = static_cast<double>(memInfo.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
    sysInfo["availableMemoryGB"] = static_cast<double>(memInfo.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);
    sysInfo["memoryUsagePercent"] = memInfo.dwMemoryLoad;

    // CPU 信息
    SYSTEM_INFO sysInfo2 = {0};
    ::GetSystemInfo(&sysInfo2);
    sysInfo["processorCount"] = static_cast<int>(sysInfo2.dwNumberOfProcessors);

    // 计算机名
    wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1] = {0};
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
    GetComputerNameW(computerName, &size);
    sysInfo["computerName"] = std::string(computerName, computerName + wcslen(computerName));

    return sysInfo.dump();
}

// ── 磁盘信息 ──

std::wstring DiagnosticReporter::GetDiskInfo() {
    auto info = CollectDiskInfo();
    return std::wstring(info.begin(), info.end());
}

std::string DiagnosticReporter::CollectDiskInfo() {
    json diskInfo = json::array();

    // 遍历所有驱动器
    DWORD drives = GetLogicalDrives();
    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        if (!(drives & (1 << (letter - L'A')))) continue;

        std::wstring rootPath = std::wstring(1, letter) + L":\\";

        UINT type = GetDriveTypeW(rootPath.c_str());
        if (type != DRIVE_FIXED && type != DRIVE_REMOTE) continue;

        uint64_t totalBytes = 0, freeBytes = 0;
        ULARGE_INTEGER freeAvail, totalSize, freeSize;
        if (GetDiskFreeSpaceExW(rootPath.c_str(), &freeAvail, &totalSize, &freeSize)) {
            totalBytes = totalSize.QuadPart;
            freeBytes = freeAvail.QuadPart;
        }

        json drive;
        drive["letter"] = std::string(1, static_cast<char>(letter));
        drive["totalGB"] = static_cast<double>(totalBytes) / (1024.0 * 1024.0 * 1024.0);
        drive["freeGB"] = static_cast<double>(freeBytes) / (1024.0 * 1024.0 * 1024.0);
        drive["usagePercent"] = totalBytes > 0 ?
            static_cast<int>((totalBytes - freeBytes) * 100 / totalBytes) : 0;

        diskInfo.push_back(drive);
    }

    return diskInfo.dump();
}

// ── 操作日志 ──

std::wstring DiagnosticReporter::GetRecentOperations(int count) {
    auto info = CollectOperationLog(count);
    return std::wstring(info.begin(), info.end());
}

std::string DiagnosticReporter::CollectOperationLog(int count) {
    auto records = OperationLogger::GetRecentOperations(count);
    json ops = json::array();

    for (const auto& record : records) {
        json op;
        op["type"] = static_cast<int>(record.type);
        op["description"] = std::string(record.description.begin(), record.description.end());
        op["size"] = record.size;
        op["success"] = record.success;

        auto timeT = std::chrono::system_clock::to_time_t(record.timestamp);
        op["timestamp"] = timeT;

        ops.push_back(op);
    }

    return ops.dump();
}

// ── 安全状态 ──

std::wstring DiagnosticReporter::GetSecurityStatus() {
    auto info = CollectSecurityStatus();
    return std::wstring(info.begin(), info.end());
}

std::string DiagnosticReporter::CollectSecurityStatus() {
    json security;

    // 回滚事务统计
    auto& rollbackMgr = RollbackManager::Instance();
    rollbackMgr.LoadRollbackLog();
    security["rollbackableTransactions"] = rollbackMgr.GetRollableTransactionCount();
    security["totalTransactions"] = rollbackMgr.GetTotalTransactionCount();

    return security.dump();
}

// ── 进程信息 ──

std::string DiagnosticReporter::CollectProcessInfo() {
    json processes = json::array();

    // 简单列出进程数量
    DWORD processesArray[1024];
    DWORD bytesNeeded = 0;

    if (EnumProcesses(processesArray, sizeof(processesArray), &bytesNeeded)) {
        int processCount = bytesNeeded / sizeof(DWORD);
        processes.push_back({{"runningProcessCount", processCount}});
    }

    return processes.dump();
}

// ── 启动项信息 ──

std::string DiagnosticReporter::CollectStartupInfo() {
    json startup = json::array();

    try {
        // 通过注册表读取启动项（简化实现，避免依赖 StartupOptimizer）
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0, KEY_READ, &hKey) == ERROR_SUCCESS) {

            DWORD index = 0;
            wchar_t name[256] = {0};
            DWORD nameSize = 256;

            while (RegEnumValueW(hKey, index, name, &nameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                json si;
                si["name"] = std::string(name, name + wcslen(name));
                si["enabled"] = true;
                startup.push_back(si);
                index++;
                nameSize = 256;
            }
            RegCloseKey(hKey);
        }
    }
    catch (...) {
        // 忽略启动项收集失败
    }

    return startup.dump();
}

// ── 导出报告 ──

bool DiagnosticReporter::ExportReport(const std::wstring& reportContent, const std::wstring& filePath) {
    try {
        std::wofstream file(filePath);
        if (!file.is_open()) return false;
        file << reportContent;
        return true;
    }
    catch (...) {
        return false;
    }
}

std::wstring DiagnosticReporter::GetReportDirectory() {
    wchar_t appDataPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return std::wstring(appDataPath) + L"\\IceClean\\reports";
    }
    return L"reports";
}

} // namespace IceClean::Core::Safety
