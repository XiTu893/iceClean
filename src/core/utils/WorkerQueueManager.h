#pragma once
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <string>
#include <vector>

namespace IceClean::Core::Utils {

enum class TaskStatus {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled
};

struct TaskInfo {
    int id = 0;
    std::wstring name;
    TaskStatus status = TaskStatus::Pending;
    int progress = 0;
    std::wstring statusText;
};

class WorkerQueueManager {
public:
    static WorkerQueueManager& Instance();

    // 提交任务
    int Enqueue(std::function<void(const std::function<bool()>& isCancelled,
                                    const std::function<void(int)>& reportProgress)> task,
                const std::wstring& taskName);

    // 取消任务
    void Cancel(int taskId);
    void CancelAll();

    // 查询状态
    TaskInfo GetTaskInfo(int taskId) const;
    std::vector<TaskInfo> GetAllTasks() const;
    bool IsRunning(int taskId) const;
    bool HasRunningTasks() const;

    // 启动/停止工作线程
    void Start();
    void Stop();

    ~WorkerQueueManager();

private:
    WorkerQueueManager() = default;
    WorkerQueueManager(const WorkerQueueManager&) = delete;
    WorkerQueueManager& operator=(const WorkerQueueManager&) = delete;

    void WorkerLoop();

    struct QueuedTask {
        int id;
        std::wstring name;
        std::function<void(const std::function<bool()>&, const std::function<void(int)>&)> func;
        std::atomic<TaskStatus> status{TaskStatus::Pending};
        std::atomic<int> progress{0};
        std::wstring statusText;
    };

    std::queue<std::shared_ptr<QueuedTask>> m_queue;
    std::shared_ptr<QueuedTask> m_currentTask;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_worker;
    std::atomic<bool> m_running{false};
    std::atomic<int> m_nextId{1};

    // 取消标志
    std::atomic<bool> m_cancelCurrent{false};
};

} // namespace IceClean::Core::Utils
