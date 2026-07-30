#include "ScheduledCleanManager.h"
#include "utils/Win32Util.h"
#include "utils/RegistryUtil.h"
#include "utils/JsonUtil.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <sstream>
#include <windows.h>

namespace IceClean::Core::Optimizer {

using namespace IceClean::Utils;

// wstring -> UTF-8 string
static std::string WstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
                                   nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "";
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()),
                        result.data(), size, nullptr, nullptr);
    return result;
}

// UTF-8 string -> wstring
static std::wstring Utf8ToWstring(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()),
                                   nullptr, 0);
    if (size <= 0) return L"";
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()),
                        result.data(), size);
    return result;
}

// 获取配置目录（不含文件名）
static std::wstring GetConfigDir() {
    // JsonUtil::GetConfigPath() 返回 ...\IceClean\config.json
    // 我们需要的是 ...\IceClean\ 目录
    std::wstring configPath = JsonUtil::GetConfigPath();
    size_t lastSlash = configPath.rfind(L'\\');
    if (lastSlash != std::wstring::npos) {
        return configPath.substr(0, lastSlash);
    }
    return configPath;
}

std::wstring ScheduledCleanManager::GetTaskPrefix() {
    return L"IceClean_AutoClean_";
}

std::vector<ScheduledCleanTask> ScheduledCleanManager::GetScheduledTasks() {
    std::vector<ScheduledCleanTask> tasks;

    // 从配置文件加载任务
    auto configPath = GetConfigDir() + L"\\scheduled_tasks.json";
    auto json = JsonUtil::LoadJson(configPath);
    if (!json.is_array()) return tasks;

    for (const auto& item : json) {
        ScheduledCleanTask task;
        task.taskName = Utf8ToWstring(item.value("taskName", ""));
        task.taskId = Utf8ToWstring(item.value("taskId", ""));
        task.enabled = item.value("enabled", true);
        task.cleanTemp = item.value("cleanTemp", true);
        task.cleanBrowserCache = item.value("cleanBrowserCache", true);
        task.cleanRecycleBin = item.value("cleanRecycleBin", false);
        task.cleanThumbnails = item.value("cleanThumbnails", true);
        task.cleanUpdateCache = item.value("cleanUpdateCache", false);
        task.cleanLogs = item.value("cleanLogs", true);
        task.shutdownAfterClean = item.value("shutdownAfterClean", false);
        task.scheduleType = static_cast<ScheduledCleanTask::ScheduleType>(
            item.value("scheduleType", 0));
        task.hour = item.value("hour", 3);
        task.minute = item.value("minute", 0);
        task.dayOfWeek = item.value("dayOfWeek", 0);
        task.dayOfMonth = item.value("dayOfMonth", 1);
        task.lastCleanSize = item.value("lastCleanSize", (uint64_t)0);
        task.lastRunTime = Utf8ToWstring(item.value("lastRunTime", ""));

        if (!task.taskId.empty()) {
            tasks.push_back(task);
        }
    }

    return tasks;
}

bool ScheduledCleanManager::CreateScheduledTask(const ScheduledCleanTask& task) {
    if (task.taskId.empty()) return false;

    std::wstring taskName = GetTaskPrefix() + task.taskId;
    std::wstring appPath = GetAppPath();

    // 构建命令行参数
    // 格式: IceClean.exe --auto-clean --task <taskId>
    std::wstring arguments = L"\"" + appPath + L"\" --auto-clean --task " + task.taskId;

    // 使用 schtasks 创建计划任务
    std::wstring scheduleArg;
    switch (task.scheduleType) {
    case ScheduledCleanTask::ScheduleType::Daily:
        scheduleArg = L"/SC DAILY";
        break;
    case ScheduledCleanTask::ScheduleType::Weekly:
        scheduleArg = L"/SC WEEKLY /D " + std::to_wstring(task.dayOfWeek == 0 ? 7 : task.dayOfWeek);
        break;
    case ScheduledCleanTask::ScheduleType::Monthly:
        scheduleArg = L"/SC MONTHLY /D " + std::to_wstring(task.dayOfMonth);
        break;
    }

    std::wstring timeStr = std::to_wstring(task.hour);
    if (task.hour < 10) timeStr = L"0" + timeStr;
    std::wstring minStr = std::to_wstring(task.minute);
    if (task.minute < 10) minStr = L"0" + minStr;

    std::wstring command = L"schtasks /Create /TN \"" + taskName +
        L"\" /TR \"" + arguments +
        L"\" " + scheduleArg +
        L" /ST " + timeStr + L":" + minStr +
        L" /F"; // Force overwrite

    // 如果需要最高权限
    command += L" /RL HIGHEST";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    std::wstring cmdLine = L"cmd.exe /c " + command;
    BOOL success = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) return false;

    WaitForSingleObject(pi.hProcess, 10000);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0) return false;

    // 保存任务配置到文件
    SaveTasksToFile(task, true);
    return true;
}

bool ScheduledCleanManager::DeleteScheduledTask(const std::wstring& taskId) {
    std::wstring taskName = GetTaskPrefix() + taskId;

    std::wstring command = L"schtasks /Delete /TN \"" + taskName + L"\" /F";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    std::wstring cmdLine = L"cmd.exe /c " + command;
    BOOL success = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) return false;

    WaitForSingleObject(pi.hProcess, 10000);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 从配置文件中移除
    SaveTasksToFile(ScheduledCleanTask{}, false, taskId);
    return exitCode == 0;
}

bool ScheduledCleanManager::EnableScheduledTask(const std::wstring& taskId, bool enable) {
    std::wstring taskName = GetTaskPrefix() + taskId;
    std::wstring command = enable
        ? L"schtasks /Change /TN \"" + taskName + L"\" /ENABLE"
        : L"schtasks /Change /TN \"" + taskName + L"\" /DISABLE";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    std::wstring cmdLine = L"cmd.exe /c " + command;
    BOOL success = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) return false;

    WaitForSingleObject(pi.hProcess, 10000);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 更新配置文件中的启用状态
    auto tasks = GetScheduledTasks();
    for (auto& t : tasks) {
        if (t.taskId == taskId) {
            t.enabled = enable;
            break;
        }
    }
    SaveAllTasksToFile(tasks);

    return exitCode == 0;
}

bool ScheduledCleanManager::RunNow(const ScheduledCleanTask& task) {
    std::wstring appPath = GetAppPath();
    std::wstring arguments = L"\"" + appPath + L"\" --auto-clean --task " + task.taskId;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    BOOL success = CreateProcessW(nullptr, &arguments[0], nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) return false;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

std::wstring ScheduledCleanManager::GenerateTaskId() const {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return std::to_wstring(millis);
}

std::wstring ScheduledCleanManager::GetAppPath() const {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    return path;
}

std::wstring ScheduledCleanManager::BuildTriggerDescription(const ScheduledCleanTask& task) const {
    std::wstring desc;
    switch (task.scheduleType) {
    case ScheduledCleanTask::ScheduleType::Daily:
        desc = L"每天";
        break;
    case ScheduledCleanTask::ScheduleType::Weekly: {
        const wchar_t* days[] = {L"周日", L"周一", L"周二", L"周三", L"周四", L"周五", L"周六"};
        int idx = (task.dayOfWeek >= 0 && task.dayOfWeek <= 6) ? task.dayOfWeek : 0;
        desc = L"每周" + std::wstring(days[idx]);
        break;
    }
    case ScheduledCleanTask::ScheduleType::Monthly:
        desc = L"每月" + std::to_wstring(task.dayOfMonth) + L"日";
        break;
    }
    return desc;
}

void ScheduledCleanManager::SaveTasksToFile(const ScheduledCleanTask& newTask, bool isAdd,
                                             const std::wstring& removeTaskId) {
    auto tasks = GetScheduledTasks();

    if (isAdd) {
        // 添加或更新
        bool found = false;
        for (auto& t : tasks) {
            if (t.taskId == newTask.taskId) {
                t = newTask;
                found = true;
                break;
            }
        }
        if (!found) {
            tasks.push_back(newTask);
        }
    } else if (!removeTaskId.empty()) {
        // 删除
        tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
            [&removeTaskId](const ScheduledCleanTask& t) {
                return t.taskId == removeTaskId;
            }), tasks.end());
    }

    SaveAllTasksToFile(tasks);
}

void ScheduledCleanManager::SaveAllTasksToFile(const std::vector<ScheduledCleanTask>& tasks) {
    nlohmann::json json = nlohmann::json::array();

    for (const auto& task : tasks) {
        nlohmann::json item;
        item["taskName"] = WstringToUtf8(task.taskName);
        item["taskId"] = WstringToUtf8(task.taskId);
        item["enabled"] = task.enabled;
        item["cleanTemp"] = task.cleanTemp;
        item["cleanBrowserCache"] = task.cleanBrowserCache;
        item["cleanRecycleBin"] = task.cleanRecycleBin;
        item["cleanThumbnails"] = task.cleanThumbnails;
        item["cleanUpdateCache"] = task.cleanUpdateCache;
        item["cleanLogs"] = task.cleanLogs;
        item["shutdownAfterClean"] = task.shutdownAfterClean;
        item["scheduleType"] = static_cast<int>(task.scheduleType);
        item["hour"] = task.hour;
        item["minute"] = task.minute;
        item["dayOfWeek"] = task.dayOfWeek;
        item["dayOfMonth"] = task.dayOfMonth;
        item["lastCleanSize"] = task.lastCleanSize;
        item["lastRunTime"] = WstringToUtf8(task.lastRunTime);
        json.push_back(item);
    }

    auto configPath = GetConfigDir() + L"\\scheduled_tasks.json";
    JsonUtil::SaveJson(configPath, json);
}

} // namespace IceClean::Core::Optimizer
