#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace IceClean::Core::Optimizer {

// 定时清理计划
struct ScheduledCleanTask {
    std::wstring taskName;         // 任务名称
    std::wstring taskId;          // 唯一ID
    bool enabled = true;          // 是否启用
    bool cleanTemp = true;        // 清理临时文件
    bool cleanBrowserCache = true; // 清理浏览器缓存
    bool cleanRecycleBin = false;  // 清理回收站
    bool cleanThumbnails = true;  // 清理缩略图缓存
    bool cleanUpdateCache = false; // 清理更新缓存
    bool cleanLogs = true;        // 清理日志
    bool shutdownAfterClean = false; // 清理后关机
    enum class ScheduleType { Daily, Weekly, Monthly };
    ScheduleType scheduleType = ScheduleType::Daily;
    int hour = 3;                 // 执行时间(小时, 0-23)
    int minute = 0;               // 执行时间(分钟, 0-59)
    int dayOfWeek = 0;            // 星期几(0=周日, 1=周一, ...仅Weekly有效)
    int dayOfMonth = 1;           // 每月几号(仅Monthly有效)
    uint64_t lastCleanSize = 0;   // 上次清理大小
    std::wstring lastRunTime;      // 上次运行时间
};

class ScheduledCleanManager {
public:
    // 获取所有定时清理任务
    std::vector<ScheduledCleanTask> GetScheduledTasks();

    // 创建定时清理任务（注册Windows计划任务）
    bool CreateScheduledTask(const ScheduledCleanTask& task);

    // 删除定时清理任务
    bool DeleteScheduledTask(const std::wstring& taskId);

    // 启用/禁用定时清理任务
    bool EnableScheduledTask(const std::wstring& taskId, bool enable);

    // 立即执行一次清理任务
    bool RunNow(const ScheduledCleanTask& task);

    // 获取任务计划名称前缀
    static std::wstring GetTaskPrefix();

private:
    // 生成唯一任务ID
    std::wstring GenerateTaskId() const;

    // 获取本程序路径
    std::wstring GetAppPath() const;

    // 构建计划任务触发器描述
    std::wstring BuildTriggerDescription(const ScheduledCleanTask& task) const;

    // 保存任务到配置文件
    void SaveTasksToFile(const ScheduledCleanTask& newTask, bool isAdd,
                          const std::wstring& removeTaskId = L"");
    void SaveAllTasksToFile(const std::vector<ScheduledCleanTask>& tasks);
};

} // namespace IceClean::Core::Optimizer
