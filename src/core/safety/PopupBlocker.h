#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <windows.h>
#include "models/PopupBlockerInfo.h"

namespace IceClean::Core::Safety {

// 弹窗拦截器
class PopupBlocker {
public:
    // 扫描所有弹窗源
    std::vector<Models::PopupBlockerItem> ScanPopupSources();

    // 拦截指定弹窗项
    bool BlockPopup(const Models::PopupBlockerItem& item);

    // 解除拦截
    bool UnblockPopup(const Models::PopupBlockerItem& item);

    // 获取拦截统计
    Models::PopupBlockerStats GetStats();

    // 增加拦截计数
    void IncrementBlockCount(const std::wstring& name);

    // 获取弹窗类型的显示名称
    static std::wstring GetPopupTypeName(Models::PopupType type);

    // 检查弹窗拦截是否启用
    static bool IsBlockerEnabled();

    // 设置弹窗拦截启用/禁用
    static void SetBlockerEnabled(bool enabled);

private:
    // 扫描计划任务弹窗源
    void ScanScheduledTaskPopups(std::vector<Models::PopupBlockerItem>& items);

    // 扫描启动项弹窗源
    void ScanStartupPopups(std::vector<Models::PopupBlockerItem>& items);

    // 扫描广告/弹窗注册表项
    void ScanAdwarePopups(std::vector<Models::PopupBlockerItem>& items);

    // 扫描浏览器通知弹窗
    void ScanBrowserNotificationPopups(std::vector<Models::PopupBlockerItem>& items);

    // 检查是否为系统项
    bool IsSystemPopup(const std::wstring& name, const std::wstring& path) const;

    // 配置文件路径
    static std::wstring GetConfigFilePath();

    // 已知弹窗软件黑名单（进程名）
    static const std::vector<std::wstring> s_knownAdwareProcesses;

    // 配置文件读写互斥锁
    std::mutex m_configMutex;
};

} // namespace IceClean::Core::Safety
