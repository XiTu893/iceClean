#include "WorkerQueueManager.h"

namespace IceClean::Core::Utils {

WorkerQueueManager& WorkerQueueManager::Instance() {
    static WorkerQueueManager instance;
    return instance;
}

int WorkerQueueManager::Enqueue(
    std::function<void(const std::function<bool()>& isCancelled,
                        const std::function<void(int)>& reportProgress)> task,
    const std::wstring& taskName)
{
    auto queued = std::make_shared<QueuedTask>();
    queued->id = m_nextId.fetch_add(1);
    queued->name = taskName;
    queued->func = std::move(task);
    queued->status.store(TaskStatus::Pending);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(queued);
    }

    m_cv.notify_one();
    return queued->id;
}

void WorkerQueueManager::Cancel(int taskId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 检查是否在队列中
    std::queue<std::shared_ptr<QueuedTask>> remaining;
    bool found = false;
    while (!m_queue.empty()) {
        auto task = m_queue.front();
        m_queue.pop();
        if (task->id == taskId) {
            task->status.store(TaskStatus::Cancelled);
            found = true;
        } else {
            remaining.push(task);
        }
    }
    m_queue = std::move(remaining);

    // 如果是当前任务
    if (m_currentTask && m_currentTask->id == taskId) {
        m_cancelCurrent.store(true);
    }
}

void WorkerQueueManager::CancelAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cancelCurrent.store(true);

    while (!m_queue.empty()) {
        auto task = m_queue.front();
        task->status.store(TaskStatus::Cancelled);
        m_queue.pop();
    }
}

TaskInfo WorkerQueueManager::GetTaskInfo(int taskId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_currentTask && m_currentTask->id == taskId) {
        return { m_currentTask->id, m_currentTask->name,
                 m_currentTask->status.load(), m_currentTask->progress.load(),
                 m_currentTask->statusText };
    }

    // 搜索队列
    auto q = m_queue;
    while (!q.empty()) {
        auto task = q.front();
        q.pop();
        if (task->id == taskId) {
            return { task->id, task->name, task->status.load(), 0, {} };
        }
    }

    return { taskId, {}, TaskStatus::Completed, 100, {} };
}

std::vector<TaskInfo> WorkerQueueManager::GetAllTasks() const {
    std::vector<TaskInfo> result;
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_currentTask) {
        result.push_back({ m_currentTask->id, m_currentTask->name,
                           m_currentTask->status.load(), m_currentTask->progress.load(),
                           m_currentTask->statusText });
    }

    auto q = m_queue;
    while (!q.empty()) {
        auto task = q.front();
        q.pop();
        result.push_back({ task->id, task->name, task->status.load(), 0, {} });
    }

    return result;
}

bool WorkerQueueManager::IsRunning(int taskId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_currentTask && m_currentTask->id == taskId) {
        return m_currentTask->status.load() == TaskStatus::Running;
    }
    return false;
}

bool WorkerQueueManager::HasRunningTasks() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_currentTask && m_currentTask->status.load() == TaskStatus::Running) {
        return true;
    }
    return !m_queue.empty();
}

void WorkerQueueManager::Start() {
    if (m_running.load()) return;
    m_running.store(true);
    m_worker = std::thread(&WorkerQueueManager::WorkerLoop, this);
}

void WorkerQueueManager::Stop() {
    m_running.store(false);
    m_cv.notify_all();
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

WorkerQueueManager::~WorkerQueueManager() {
    Stop();
}

void WorkerQueueManager::WorkerLoop() {
    while (m_running.load()) {
        std::shared_ptr<QueuedTask> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() {
                return !m_queue.empty() || !m_running.load();
            });

            if (!m_running.load()) return;

            task = m_queue.front();
            m_queue.pop();
            m_currentTask = task;
            m_cancelCurrent.store(false);
        }

        if (!task) continue;

        task->status.store(TaskStatus::Running);

        auto isCancelled = [this]() -> bool {
            return m_cancelCurrent.load() || !m_running.load();
        };

        auto reportProgress = [task](int percent) {
            task->progress.store(percent);
        };

        try {
            task->func(isCancelled, reportProgress);

            if (m_cancelCurrent.load()) {
                task->status.store(TaskStatus::Cancelled);
            } else {
                task->progress.store(100);
                task->status.store(TaskStatus::Completed);
            }
        } catch (...) {
            task->status.store(TaskStatus::Failed);
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_currentTask.reset();
        }
    }
}

} // namespace IceClean::Core::Utils
