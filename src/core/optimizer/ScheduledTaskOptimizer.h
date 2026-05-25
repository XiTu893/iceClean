#pragma once
#include "models/StartupItem.h"
#include <vector>
#include <string>

namespace IceClean::Core::Optimizer {

// 计划任务信息
struct ScheduledTaskInfo {
    std::wstring name;
    std::wstring path;
    std::wstring description;
    bool isEnabled;
    bool triggersAtLogon;
    bool triggersAtStartup;
    bool canDisable;
    bool isCritical;
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
