#include "ScheduledTaskOptimizer.h"
#include <windows.h>
#include <comdef.h>
#include <taskschd.h>
#include <algorithm>
#include <functional>

#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsupp.lib")
#pragma comment(lib, "ole32.lib")

namespace IceClean::Core::Optimizer {

const std::vector<std::wstring>& ScheduledTaskOptimizer::GetCriticalTaskNames() {
    static const std::vector<std::wstring> names = {
        L"\\Microsoft\\Windows\\Defrag\\",
        L"\\Microsoft\\Windows\\DiskDiagnostic\\",
        L"\\Microsoft\\Windows\\Maintenance\\",
        L"\\Microsoft\\Windows\\SystemRestore\\",
        L"\\Microsoft\\Windows\\Windows Defender\\",
        L"\\Microsoft\\Windows\\WindowsBackup\\",
        L"\\Microsoft\\Windows\\WindowsUpdate\\",
    };
    return names;
}

bool ScheduledTaskOptimizer::IsCriticalTask(const std::wstring& taskName) const {
    std::wstring lower = taskName;
    std::transform(lower.begin(), lower.end(), lower.begin(), towlower);

    for (const auto& critical : GetCriticalTaskNames()) {
        if (lower.find(critical) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

ScheduledTaskOptimizer::ScheduledTaskOptimizer() : comInitialized_(false) {
    // 初始化COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {
        comInitialized_ = true;
    }
}

ScheduledTaskOptimizer::~ScheduledTaskOptimizer() {
    if (comInitialized_) {
        CoUninitialize();
    }
}

std::vector<ScheduledTaskInfo> ScheduledTaskOptimizer::GetStartupTasks() {
    std::vector<ScheduledTaskInfo> tasks;

    // 创建TaskService COM对象
    ITaskService* pService = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TaskScheduler,
                                   nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ITaskService,
                                   reinterpret_cast<void**>(&pService));
    if (FAILED(hr)) return tasks;

    // 连接到任务计划服务
    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        pService->Release();
        return tasks;
    }

    // 获取根任务文件夹
    ITaskFolder* pRootFolder = nullptr;
    hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
    if (FAILED(hr)) {
        pService->Release();
        return tasks;
    }

    // 递归获取所有任务
    std::function<void(ITaskFolder*, const std::wstring&)> enumerateFolder;
    enumerateFolder = [&](ITaskFolder* pFolder, const std::wstring& folderPath) {
        IRegisteredTaskCollection* pTaskCollection = nullptr;
        HRESULT hr2 = pFolder->GetTasks(TASK_ENUM_HIDDEN, &pTaskCollection);
        if (FAILED(hr2)) return;

        LONG taskCount = 0;
        pTaskCollection->get_Count(&taskCount);

        for (LONG i = 1; i <= taskCount; ++i) {
            IRegisteredTask* pTask = nullptr;
            hr2 = pTaskCollection->get_Item(_variant_t(i), &pTask);
            if (FAILED(hr2)) continue;

            BSTR taskName = nullptr;
            pTask->get_Name(&taskName);

            BSTR taskPath = nullptr;
            pTask->get_Path(&taskPath);

            TASK_STATE taskState = TASK_STATE_UNKNOWN;
            pTask->get_State(&taskState);

            // 检查是否有登录/启动触发器
            bool triggersAtLogon = false;
            bool triggersAtStartup = false;

            ITaskDefinition* pDefinition = nullptr;
            hr2 = pTask->get_Definition(&pDefinition);
            if (SUCCEEDED(hr2)) {
                ITriggerCollection* pTriggers = nullptr;
                hr2 = pDefinition->get_Triggers(&pTriggers);
                if (SUCCEEDED(hr2)) {
                    LONG triggerCount = 0;
                    pTriggers->get_Count(&triggerCount);

                    for (LONG j = 1; j <= triggerCount; ++j) {
                        ITrigger* pTrigger = nullptr;
                        hr2 = pTriggers->get_Item(j, &pTrigger);
                        if (FAILED(hr2)) continue;

                        TASK_TRIGGER_TYPE2 triggerType;
                        pTrigger->get_Type(&triggerType);

                        if (triggerType == TASK_TRIGGER_LOGON) {
                            triggersAtLogon = true;
                        } else if (triggerType == TASK_TRIGGER_BOOT) {
                            triggersAtStartup = true;
                        }

                        pTrigger->Release();
                    }
                    pTriggers->Release();
                }
                pDefinition->Release();
            }

            // 只收集开机启动相关的任务
            if (triggersAtLogon || triggersAtStartup) {
                ScheduledTaskInfo info;
                info.name = taskName ? taskName : L"";
                info.path = taskPath ? taskPath : L"";
                info.isEnabled = (taskState != TASK_STATE_DISABLED);
                info.triggersAtLogon = triggersAtLogon;
                info.triggersAtStartup = triggersAtStartup;
                info.isCritical = IsCriticalTask(info.path);
                info.canDisable = !info.isCritical;

                tasks.push_back(info);
            }

            if (taskName) SysFreeString(taskName);
            if (taskPath) SysFreeString(taskPath);
            pTask->Release();
        }

        pTaskCollection->Release();

        // 递归子文件夹
        ITaskFolderCollection* pSubFolders = nullptr;
        hr2 = pFolder->GetFolders(0, &pSubFolders);
        if (SUCCEEDED(hr2)) {
            LONG folderCount = 0;
            pSubFolders->get_Count(&folderCount);

            for (LONG i = 1; i <= folderCount; ++i) {
                ITaskFolder* pSubFolder = nullptr;
                hr2 = pSubFolders->get_Item(_variant_t(i), &pSubFolder);
                if (SUCCEEDED(hr2)) {
                    BSTR subFolderName = nullptr;
                    pSubFolder->get_Name(&subFolderName);

                    std::wstring subPath = folderPath;
                    if (subPath.back() != L'\\') subPath += L'\\';
                    subPath += (subFolderName ? subFolderName : L"");

                    enumerateFolder(pSubFolder, subPath);

                    if (subFolderName) SysFreeString(subFolderName);
                    pSubFolder->Release();
                }
            }
            pSubFolders->Release();
        }
    };

    enumerateFolder(pRootFolder, L"\\");

    pRootFolder->Release();
    pService->Release();

    return tasks;
}

std::vector<Models::StartupItem> ScheduledTaskOptimizer::GetDisablableTasks() {
    std::vector<Models::StartupItem> items;

    auto tasks = GetStartupTasks();
    for (const auto& task : tasks) {
        if (task.isCritical) continue;
        if (!task.isEnabled) continue;

        Models::StartupItem item;
        item.name = task.name;
        item.path = task.path;
        item.type = Models::StartupItemType::ScheduledTask;
        item.isEnabled = true;
        item.isSystemCritical = false;
        item.canDisable = true;

        items.push_back(item);
    }

    return items;
}

bool ScheduledTaskOptimizer::DisableTask(const std::wstring& taskPath, const std::wstring& taskName) {
    ITaskService* pService = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TaskScheduler,
                                   nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ITaskService,
                                   reinterpret_cast<void**>(&pService));
    if (FAILED(hr)) return false;

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        pService->Release();
        return false;
    }

    // 获取任务所在文件夹
    std::wstring folderPath = taskPath;
    auto lastSlash = folderPath.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        folderPath = folderPath.substr(0, lastSlash);
        if (folderPath.empty()) folderPath = L"\\";
    } else {
        folderPath = L"\\";
    }

    ITaskFolder* pFolder = nullptr;
    hr = pService->GetFolder(_bstr_t(folderPath.c_str()), &pFolder);
    if (FAILED(hr)) {
        pService->Release();
        return false;
    }

    IRegisteredTask* pTask = nullptr;
    hr = pFolder->GetTask(_bstr_t(taskName.c_str()), &pTask);
    if (FAILED(hr)) {
        pFolder->Release();
        pService->Release();
        return false;
    }

    hr = pTask->put_Enabled(VARIANT_FALSE);

    pTask->Release();
    pFolder->Release();
    pService->Release();

    return SUCCEEDED(hr);
}

bool ScheduledTaskOptimizer::EnableTask(const std::wstring& taskPath, const std::wstring& taskName) {
    ITaskService* pService = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TaskScheduler,
                                   nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_ITaskService,
                                   reinterpret_cast<void**>(&pService));
    if (FAILED(hr)) return false;

    hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
    if (FAILED(hr)) {
        pService->Release();
        return false;
    }

    std::wstring folderPath = taskPath;
    auto lastSlash = folderPath.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos) {
        folderPath = folderPath.substr(0, lastSlash);
        if (folderPath.empty()) folderPath = L"\\";
    } else {
        folderPath = L"\\";
    }

    ITaskFolder* pFolder = nullptr;
    hr = pService->GetFolder(_bstr_t(folderPath.c_str()), &pFolder);
    if (FAILED(hr)) {
        pService->Release();
        return false;
    }

    IRegisteredTask* pTask = nullptr;
    hr = pFolder->GetTask(_bstr_t(taskName.c_str()), &pTask);
    if (FAILED(hr)) {
        pFolder->Release();
        pService->Release();
        return false;
    }

    hr = pTask->put_Enabled(VARIANT_TRUE);

    pTask->Release();
    pFolder->Release();
    pService->Release();

    return SUCCEEDED(hr);
}

} // namespace IceClean::Core::Optimizer
