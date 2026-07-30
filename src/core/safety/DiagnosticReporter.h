#pragma once
#include <string>
#include <vector>
#include <chrono>

namespace IceClean::Core::Safety {

// 诊断报告生成器
// 收集系统信息、磁盘状态、最近操作等，生成诊断报告
class DiagnosticReporter {
public:
    // 生成诊断报告并返回文件路径
    static std::wstring GenerateReport();

    // 获取系统信息摘要
    static std::wstring GetSystemInfo();

    // 获取磁盘信息摘要
    static std::wstring GetDiskInfo();

    // 获取最近操作摘要
    static std::wstring GetRecentOperations(int count = 20);

    // 获取安全状态摘要
    static std::wstring GetSecurityStatus();

    // 导出报告到文件
    static bool ExportReport(const std::wstring& reportContent, const std::wstring& filePath);

private:
    // 收集系统信息
    static std::string CollectSystemInfo();

    // 收集磁盘信息
    static std::string CollectDiskInfo();

    // 收集操作日志
    static std::string CollectOperationLog(int count);

    // 收集安全状态
    static std::string CollectSecurityStatus();

    // 收集进程信息
    static std::string CollectProcessInfo();

    // 收集启动项信息
    static std::string CollectStartupInfo();

    // 获取报告目录
    static std::wstring GetReportDirectory();
};

} // namespace IceClean::Core::Safety
