#include "FileWatcher.h"
#include <algorithm>
#include <mutex>
#include <thread>

namespace IceClean::Core::Safety {

// ── Constructor / Destructor ──

FileWatcher::FileWatcher() = default;

FileWatcher::~FileWatcher() {
    Stop();
}

// ── 启动监控 ──

bool FileWatcher::Start(const std::vector<Models::FileWatchConfig>& configs) {
    if (m_running.load()) return false;

    m_running.store(true);
    m_changes.clear();

    for (const auto& config : configs) {
        if (!config.isEnabled) continue;

        // 创建停止事件
        HANDLE stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        m_stopEvents.push_back(stopEvent);

        // 启动监控线程
        std::thread([this, config, stopEvent]() {
            WatchThread(config, stopEvent);
        }).detach();
    }

    return true;
}

// ── 停止监控 ──

void FileWatcher::Stop() {
    if (!m_running.load()) return;

    m_running.store(false);

    // 通知所有线程停止
    for (auto& event : m_stopEvents) {
        SetEvent(event);
    }

    // 等待一段时间让线程退出
    Sleep(500);

    // 清理
    for (auto& event : m_stopEvents) {
        CloseHandle(event);
    }
    m_stopEvents.clear();
}

// ── 监控线程 ──

void FileWatcher::WatchThread(const Models::FileWatchConfig& config, HANDLE stopEvent) {
    HANDLE hDir = CreateFileW(
        config.watchPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (hDir == INVALID_HANDLE_VALUE) return;

    // 构建过滤标志
    DWORD notifyFilter = 0;
    if (config.watchAdded || config.watchRemoved) notifyFilter |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
    if (config.watchModified) notifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE;
    if (config.watchRenamed) notifyFilter |= FILE_NOTIFY_CHANGE_FILE_NAME;

    if (notifyFilter == 0) {
        CloseHandle(hDir);
        return;
    }

    const DWORD bufferSize = 64 * 1024;
    auto buffer = std::make_unique<BYTE[]>(bufferSize);

    OVERLAPPED overlapped = {};
    overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

    while (m_running.load()) {
        DWORD bytesReturned = 0;
        ResetEvent(overlapped.hEvent);

        BOOL success = ReadDirectoryChangesW(
            hDir,
            buffer.get(),
            bufferSize,
            config.watchSubtree,
            notifyFilter,
            &bytesReturned,
            &overlapped,
            nullptr);

        if (!success) break;

        // 等待变更或停止信号
        HANDLE waitHandles[] = { overlapped.hEvent, stopEvent };
        DWORD waitCount = 2;

        DWORD waitResult = WaitForMultipleObjects(waitCount, waitHandles, FALSE, 5000);

        if (!m_running.load()) break;

        if (waitResult == WAIT_OBJECT_0) {
            // 有变更
            if (GetOverlappedResult(hDir, &overlapped, &bytesReturned, FALSE) && bytesReturned > 0) {
                BYTE* ptr = buffer.get();
                while (true) {
                    auto* info = reinterpret_cast<PFILE_NOTIFY_INFORMATION>(ptr);

                    Models::FileChangeRecord record;
                    record.fileName = std::wstring(info->FileName, info->FileNameLength / sizeof(wchar_t));
                    record.filePath = config.watchPath + L"\\" + record.fileName;

                    switch (info->Action) {
                    case FILE_ACTION_ADDED:       record.changeType = Models::FileChangeType::Added; break;
                    case FILE_ACTION_REMOVED:     record.changeType = Models::FileChangeType::Removed; break;
                    case FILE_ACTION_MODIFIED:    record.changeType = Models::FileChangeType::Modified; break;
                    case FILE_ACTION_RENAMED_OLD_NAME: record.changeType = Models::FileChangeType::RenamedOld; break;
                    case FILE_ACTION_RENAMED_NEW_NAME: record.changeType = Models::FileChangeType::RenamedNew; break;
                    }

                    SYSTEMTIME st;
                    GetSystemTime(&st);
                    SystemTimeToFileTime(&st, &record.timestamp);

                    // 获取文件大小
                    WIN32_FILE_ATTRIBUTE_DATA fileAttr;
                    if (GetFileAttributesExW(record.filePath.c_str(), GetFileExInfoStandard, &fileAttr)) {
                        record.fileSize = (static_cast<uint64_t>(fileAttr.nFileSizeHigh) << 32) | fileAttr.nFileSizeLow;
                    }

                    AddChangeRecord(record);

                    if (info->NextEntryOffset == 0) break;
                    ptr += info->NextEntryOffset;
                }
            }
        } else if (waitResult == WAIT_TIMEOUT) {
            continue;
        } else {
            break;
        }
    }

    CloseHandle(overlapped.hEvent);
    CloseHandle(hDir);
}

// ── 添加变更记录 ──

void FileWatcher::AddChangeRecord(const Models::FileChangeRecord& record) {
    std::lock_guard<std::mutex> lock(m_changesMutex);
    m_changes.push_back(record);
    m_totalChanges++;
    m_todayChanges++;

    // 限制记录数量
    if (m_changes.size() > 1000) {
        m_changes.erase(m_changes.begin(), m_changes.begin() + 100);
    }

    if (m_callback) {
        m_callback(record);
    }
}

// ── 获取变更记录 ──

std::vector<Models::FileChangeRecord> FileWatcher::GetRecentChanges(int count) {
    std::lock_guard<std::mutex> lock(m_changesMutex);
    std::vector<Models::FileChangeRecord> result;

    int startIdx = (std::max)(0, static_cast<int>(m_changes.size()) - count);
    for (int i = startIdx; i < static_cast<int>(m_changes.size()); ++i) {
        result.push_back(m_changes[i]);
    }

    return result;
}

void FileWatcher::ClearChanges() {
    std::lock_guard<std::mutex> lock(m_changesMutex);
    m_changes.clear();
    m_todayChanges = 0;
}

void FileWatcher::SetChangeCallback(ChangeCallback callback) {
    m_callback = std::move(callback);
}

Models::FileWatchStats FileWatcher::GetStats() {
    Models::FileWatchStats stats;
    stats.totalChanges = m_totalChanges;
    stats.todayChanges = m_todayChanges;
    stats.watchedPaths = static_cast<int>(m_stopEvents.size());
    stats.isRunning = m_running.load();
    return stats;
}

std::wstring FileWatcher::GetChangeTypeName(Models::FileChangeType type) {
    switch (type) {
    case Models::FileChangeType::Added:     return L"新增";
    case Models::FileChangeType::Removed:   return L"删除";
    case Models::FileChangeType::Modified:  return L"修改";
    case Models::FileChangeType::RenamedOld: return L"重命名";
    case Models::FileChangeType::RenamedNew: return L"重命名为";
    default:                                return L"未知";
    }
}

} // namespace IceClean::Core::Safety
