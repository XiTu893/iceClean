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
    std::wstring path;             // 程序路径
    std::wstring publisher;        // 发布者
    StartupItemType type;          // 类型
    bool isEnabled = true;         // 是否启用
    bool isSystemCritical = false; // 是否系统关键项(不可禁用)
    bool canDisable = true;        // 是否可禁用
    int delaySeconds = 0;          // 延迟启动秒数(0=不延迟)
};

} // namespace IceClean::Models
