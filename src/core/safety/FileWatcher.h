#pragma once
#include <string>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <windows.h>
#include "models/FileWatchInfo.h"

namespace IceClean::Core::Safety {

// 文件系统监控器
class FileWatcher {
public:
    FileWatcher();
    ~FileWatcher();

    // 启动监控
    bool Start(const std::vector<Models::FileWatchConfig>& configs);

    // 停止监控
    void Stop();

    // 是否正在运行
    bool IsRunning() const { return m_running.load(); }

    // 获取最近的变更记录
    std::vector<Models::FileChangeRecord> GetRecentChanges(int count = 100);

    // 清空变更记录
    void ClearChanges();

    // 设置变更回调
    using ChangeCallback = std::function<void(const Models::FileChangeRecord&)>;
    void SetChangeCallback(ChangeCallback callback);

    // 获取统计
    Models::FileWatchStats GetStats();

    // 获取变更类型显示名
    static std::wstring GetChangeTypeName(Models::FileChangeType type);

private:
    // 监控线程函数
    void WatchThread(const Models::FileWatchConfig& config, HANDLE stopEvent);

    // 添加变更记录
    void AddChangeRecord(const Models::FileChangeRecord& record);

    std::atomic<bool> m_running{false};
    std::mutex m_changesMutex;
    std::vector<Models::FileChangeRecord> m_changes;
    ChangeCallback m_callback;
    int m_totalChanges = 0;
    int m_todayChanges = 0;

    // 线程管理
    std::vector<HANDLE> m_threadHandles;
    std::vector<HANDLE> m_stopEvents;  // 用于通知线程停止
};

} // namespace IceClean::Core::Safety
