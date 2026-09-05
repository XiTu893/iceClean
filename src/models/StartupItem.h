#pragma once
#include <string>

namespace IceClean::Models {

// 启动项类型
enum class StartupItemType {
    Registry,          // 注册表启动项
    StartupFolder,     // 启动文件夹
    ScheduledTask,     // 计划任务
    Service            // Windows服务
};

// 启动项
struct StartupItem {
    std::wstring name;             // 名称
    std::wstring path;             // 程序路径/服务名/任务路径
    std::wstring publisher;        // 发布者
    StartupItemType type;          // 类型
    bool isEnabled = true;         // 是否启用
    bool isSystemCritical = false; // 是否系统关键项(不可禁用)
    bool canDisable = true;        // 是否可禁用
    int delaySeconds = 0;          // 延迟启动秒数(0=不延迟)

    // 附加显示信息（供界面展示，由各优化器填充）
    std::wstring description;       // 描述/说明
    std::wstring statusText;        // 当前状态文本(运行中/已停止/已禁用等)
    std::wstring startTypeText;     // 服务启动类型文本(自动/手动/禁用等)
    std::wstring triggerText;       // 计划任务触发条件(登录时/开机时等)
    std::wstring nextRunTime;       // 计划任务下次运行时间
    std::wstring lastRunTime;       // 计划任务上次运行时间
    std::wstring lastRunResultText; // 计划任务上次运行结果
};

} // namespace IceClean::Models
