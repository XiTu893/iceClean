#pragma once
#include <string>
#include <vector>

namespace IceClean::Models {

// 弹窗类型
enum class PopupType {
    ScheduledTask,       // 计划任务弹窗
    StartupPopup,        // 启动项弹窗
    BrowserNotification, // 浏览器通知弹窗
    AdwarePopup,         // 广告软件弹窗
    SystemNotification,  // 系统通知弹窗
    Unknown
};

// 弹窗项信息
struct PopupBlockerItem {
    std::wstring name;           // 弹窗名称
    std::wstring description;    // 描述
    std::wstring sourcePath;     // 来源路径（注册表路径或文件路径）
    std::wstring processName;    // 关联进程名
    PopupType type = PopupType::Unknown;
    bool isBlocked = false;      // 是否已拦截
    bool isSystem = false;       // 是否系统项（不建议拦截）
    int blockCount = 0;          // 已拦截次数
};

// 弹窗拦截统计
struct PopupBlockerStats {
    int totalBlocked = 0;        // 总拦截次数
    int todayBlocked = 0;        // 今日拦截次数
    int rulesCount = 0;          // 拦截规则数
    bool isEnabled = true;       // 是否启用弹窗拦截
};

} // namespace IceClean::Models
