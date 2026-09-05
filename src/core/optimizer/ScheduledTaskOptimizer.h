#pragma once
#include "models/StartupItem.h"
#include <vector>
#include <string>

namespace IceClean::Core::Optimizer {

// 计划任务信息
struct ScheduledTaskInfo {
    std::wstring name;              // 任务名称
    std::wstring path;              // 完整路径(\Microsoft\...\任务名)
    std::wstring description;       // 任务描述
    std::wstring actionCommand;     // 执行动作(程序路径+参数)
    std::wstring nextRunTime;       // 下次运行时间(本地时间字符串)
    std::wstring lastRunTime;       // 上次运行时间
    long lastRunResult = 0;         // 上次运行结果代码
    bool isEnabled = false;         // 是否启用
    bool triggersAtLogon = false;   // 登录时触发
    bool triggersAtStartup = false; // 开机时触发
    bool canDisable = true;         // 是否可禁用
    bool isCritical = false;        // 是否系统关键任务
};

class ScheduledTaskOptimizer {
public:
    ScheduledTaskOptimizer();
    ~ScheduledTaskOptimizer();

    // 获取所有开机启动的计划任务
    std::vector<ScheduledTaskInfo> GetStartupTasks();

    // 获取可禁用的计划任务列表(StartupItem格式)
    std::vector<Models::StartupItem> GetDisablableTasks();

    // 禁用计划任务
    bool DisableTask(const std::wstring& taskPath, const std::wstring& taskName);

    // 启用计划任务
    bool EnableTask(const std::wstring& taskPath, const std::wstring& taskName);

private:
    bool comInitialized_;

    // 判断计划任务是否为关键任务
    bool IsCriticalTask(const std::wstring& taskName) const;

    // 关键任务列表
    static const std::vector<std::wstring>& GetCriticalTaskNames();
};

} // namespace IceClean::Core::Optimizer
