#include "RollbackManager.h"
#include "SystemBackupManager.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <windows.h>
#include <shlobj.h>
#include <comdef.h>
#include <taskschd.h>
#pragma comment(lib, "taskschd.lib")

namespace IceClean::Core::Safety {

using json = nlohmann::json;

// ── 单例 ──

RollbackManager& RollbackManager::Instance() {
    static RollbackManager instance;
    return instance;
}

// ── 事务管理 ──

std::wstring RollbackManager::BeginTransaction(const std::wstring& description) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto transaction = std::make_shared<Models::RollbackTransaction>();
    transaction->transactionId = GenerateTransactionId();
    transaction->description = description;
    transaction->createTime = std::chrono::system_clock::now();

    m_transactions.push_back(transaction);

    spdlog::info("开始事务: {} ({})", std::string(description.begin(), description.end()),
                 std::string(transaction->transactionId.begin(), transaction->transactionId.end()));

    return transaction->transactionId;
}

void RollbackManager::AddStep(const std::wstring& transactionId,
                               Models::RollbackStepType type,
                               const std::wstring& description,
                               const std::wstring& originalData) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& tx : m_transactions) {
        if (tx->transactionId == transactionId) {
            Models::RollbackStep step;
            step.type = type;
            step.description = description;
            step.originalData = originalData;
            step.timestamp = std::chrono::system_clock::now();
            tx->steps.push_back(step);
            return;
        }
    }

    spdlog::warn("添加回滚步骤失败：未找到事务 {}", std::string(transactionId.begin(), transactionId.end()));
}

void RollbackManager::MarkStepExecuted(const std::wstring& transactionId, int stepIndex) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& tx : m_transactions) {
        if (tx->transactionId == transactionId) {
            if (stepIndex >= 0 && stepIndex < static_cast<int>(tx->steps.size())) {
                tx->steps[stepIndex].executed = true;
            }
            return;
        }
    }
}

void RollbackManager::CommitTransaction(const std::wstring& transactionId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& tx : m_transactions) {
        if (tx->transactionId == transactionId) {
            tx->committed = true;
            spdlog::info("提交事务: {}", std::string(tx->description.begin(), tx->description.end()));
            SaveRollbackLog();
            return;
        }
    }
}

bool RollbackManager::RollbackTransaction(const std::wstring& transactionId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& tx : m_transactions) {
        if (tx->transactionId == transactionId && !tx->committed && !tx->rolledBack) {
            spdlog::info("回滚事务: {}", std::string(tx->description.begin(), tx->description.end()));

            // 逆序执行回滚步骤
            bool allSuccess = true;
            for (int i = static_cast<int>(tx->steps.size()) - 1; i >= 0; --i) {
                auto& step = tx->steps[i];
                if (step.executed && !step.rolledBack) {
                    bool success = ExecuteRollbackStep(step);
                    step.rolledBack = true;
                    if (!success) {
                        allSuccess = false;
                        spdlog::error("回滚步骤失败: {}", std::string(step.description.begin(), step.description.end()));
                    }
                }
            }

            tx->rolledBack = true;
            SaveRollbackLog();
            return allSuccess;
        }
    }

    spdlog::warn("回滚事务失败：未找到可回滚的事务 {}", std::string(transactionId.begin(), transactionId.end()));
    return false;
}

int RollbackManager::RollbackRecentTransactions(int count) {
    std::lock_guard<std::mutex> lock(m_mutex);

    int rolledBack = 0;
    for (int i = static_cast<int>(m_transactions.size()) - 1; i >= 0 && rolledBack < count; --i) {
        auto& tx = m_transactions[i];
        if (!tx->committed && !tx->rolledBack) {
            for (int j = static_cast<int>(tx->steps.size()) - 1; j >= 0; --j) {
                auto& step = tx->steps[j];
                if (step.executed && !step.rolledBack) {
                    ExecuteRollbackStep(step);
                    step.rolledBack = true;
                }
            }
            tx->rolledBack = true;
            rolledBack++;
        }
    }

    if (rolledBack > 0) {
        SaveRollbackLog();
    }
    return rolledBack;
}

// ── 便捷注册方法 ──

void RollbackManager::RegisterFileDelete(const std::wstring& transactionId,
                                          const std::wstring& filePath,
                                          const std::wstring& backupPath) {
    json data;
    data["filePath"] = std::string(filePath.begin(), filePath.end());
    data["backupPath"] = std::string(backupPath.begin(), backupPath.end());

    // 如果有备份路径，记录备份信息；否则使用回收站
    if (backupPath.empty()) {
        data["useRecycleBin"] = true;
    }

    AddStep(transactionId, Models::RollbackStepType::FileDelete,
            L"删除文件: " + filePath,
            std::wstring(data.dump().begin(), data.dump().end()));
}

void RollbackManager::RegisterRegistryDelete(const std::wstring& transactionId,
                                              HKEY rootKey,
                                              const std::wstring& subKey) {
    // 先导出注册表项到临时文件作为备份
    std::wstring backupPath = SystemBackupManager::GetBackupDirectory() + L"\\rb_" + GenerateTransactionId() + L".reg";
    SystemBackupManager::ExportRegistryKey(rootKey, subKey, backupPath);

    json data;
    data["rootKey"] = reinterpret_cast<intptr_t>(rootKey);
    data["subKey"] = std::string(subKey.begin(), subKey.end());
    data["backupFile"] = std::string(backupPath.begin(), backupPath.end());

    AddStep(transactionId, Models::RollbackStepType::RegistryDelete,
            L"删除注册表: " + subKey,
            std::wstring(data.dump().begin(), data.dump().end()));
}

void RollbackManager::RegisterRegistryModify(const std::wstring& transactionId,
                                              HKEY rootKey,
                                              const std::wstring& subKey,
                                              const std::wstring& valueName,
                                              DWORD type,
                                              const std::vector<BYTE>& originalData) {
    json data;
    data["rootKey"] = reinterpret_cast<intptr_t>(rootKey);
    data["subKey"] = std::string(subKey.begin(), subKey.end());
    data["valueName"] = std::string(valueName.begin(), valueName.end());
    data["type"] = type;
    data["data"] = json::binary(originalData);

    AddStep(transactionId, Models::RollbackStepType::RegistryModify,
            L"修改注册表值: " + subKey + L"\\" + valueName,
            std::wstring(data.dump().begin(), data.dump().end()));
}

void RollbackManager::RegisterServiceDisable(const std::wstring& transactionId,
                                              const std::wstring& serviceName,
                                              DWORD originalStartType) {
    json data;
    data["serviceName"] = std::string(serviceName.begin(), serviceName.end());
    data["originalStartType"] = originalStartType;

    AddStep(transactionId, Models::RollbackStepType::ServiceDisable,
            L"禁用服务: " + serviceName,
            std::wstring(data.dump().begin(), data.dump().end()));
}

void RollbackManager::RegisterStartupDisable(const std::wstring& transactionId,
                                              const std::wstring& itemName,
                                              const std::wstring& originalValue,
                                              const std::wstring& registryPath) {
    json data;
    data["itemName"] = std::string(itemName.begin(), itemName.end());
    data["originalValue"] = std::string(originalValue.begin(), originalValue.end());
    data["registryPath"] = std::string(registryPath.begin(), registryPath.end());

    AddStep(transactionId, Models::RollbackStepType::StartupDisable,
            L"禁用启动项: " + itemName,
            std::wstring(data.dump().begin(), data.dump().end()));
}

void RollbackManager::RegisterTaskDisable(const std::wstring& transactionId,
                                           const std::wstring& taskName,
                                           const std::wstring& taskPath) {
    json data;
    data["taskName"] = std::string(taskName.begin(), taskName.end());
    data["taskPath"] = std::string(taskPath.begin(), taskPath.end());

    AddStep(transactionId, Models::RollbackStepType::TaskDisable,
            L"禁用计划任务: " + taskName,
            std::wstring(data.dump().begin(), data.dump().end()));
}

void RollbackManager::RegisterCustomRollback(const std::wstring& transactionId,
                                              const std::wstring& description,
                                              std::function<bool()> rollbackFunc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 查找事务
    int stepIndex = -1;
    for (auto& tx : m_transactions) {
        if (tx->transactionId == transactionId) {
            Models::RollbackStep step;
            step.type = Models::RollbackStepType::Custom;
            step.description = description;
            step.originalData = L"custom";
            step.timestamp = std::chrono::system_clock::now();
            tx->steps.push_back(step);
            stepIndex = static_cast<int>(tx->steps.size()) - 1;
            break;
        }
    }

    if (stepIndex >= 0) {
        m_customRollbacks.push_back({transactionId, stepIndex, std::move(rollbackFunc)});
    }
}

// ── 查询方法 ──

std::vector<Models::RollbackTransaction> RollbackManager::GetRecentTransactions(int count) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<Models::RollbackTransaction> result;
    int start = (std::max)(0, static_cast<int>(m_transactions.size()) - count);
    for (int i = start; i < static_cast<int>(m_transactions.size()); ++i) {
        result.push_back(*m_transactions[i]);
    }
    return result;
}

std::shared_ptr<Models::RollbackTransaction> RollbackManager::GetTransaction(const std::wstring& transactionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& tx : m_transactions) {
        if (tx->transactionId == transactionId) {
            return tx;
        }
    }
    return nullptr;
}

int RollbackManager::GetRollableTransactionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    int count = 0;
    for (const auto& tx : m_transactions) {
        if (!tx->committed && !tx->rolledBack) {
            count++;
        }
    }
    return count;
}

int RollbackManager::GetTotalTransactionCount() const {
    return static_cast<int>(m_transactions.size());
}

// ── 回滚步骤执行 ──

bool RollbackManager::ExecuteRollbackStep(const Models::RollbackStep& step) const {
    switch (step.type) {
        case Models::RollbackStepType::FileDelete:
            return RestoreFile(step.originalData);
        case Models::RollbackStepType::RegistryDelete:
        case Models::RollbackStepType::RegistryModify:
            return RestoreRegistry(step.originalData);
        case Models::RollbackStepType::ServiceDisable:
            return RestoreService(step.originalData);
        case Models::RollbackStepType::StartupDisable:
            return RestoreStartup(step.originalData);
        case Models::RollbackStepType::TaskDisable:
            return RestoreTask(step.originalData);
        case Models::RollbackStepType::Custom:
            // 自定义回滚通过存储的函数执行
            for (const auto& cr : m_customRollbacks) {
                if (cr.func) {
                    return cr.func();
                }
            }
            return false;
        default:
            return false;
    }
}

bool RollbackManager::RestoreFile(const std::wstring& originalData) const {
    try {
        auto dataStr = std::string(originalData.begin(), originalData.end());
        auto j = json::parse(dataStr);

        std::string backupPathStr = j.value("backupPath", "");
        std::string filePathStr = j.value("filePath", "");
        bool useRecycleBin = j.value("useRecycleBin", false);

        std::wstring filePath(filePathStr.begin(), filePathStr.end());

        if (useRecycleBin) {
            // 从回收站恢复 - 使用 SHFileOperation 的 FOF_ALLOWUNDO
            // 注意：移入回收站的文件可通过 SHFileOperation 恢复
            // 这里简单记录，实际恢复需要用户通过系统回收站操作
            spdlog::info("文件已移入回收站，可通过系统回收站恢复: {}", filePathStr);
            return true;
        }

        if (!backupPathStr.empty()) {
            std::wstring backupPath(backupPathStr.begin(), backupPathStr.end());
            // 从备份路径恢复文件
            if (MoveFileW(backupPath.c_str(), filePath.c_str())) {
                spdlog::info("文件恢复成功: {} -> {}", backupPathStr, filePathStr);
                return true;
            }
            // MoveFile 失败，尝试 CopyFile
            if (CopyFileW(backupPath.c_str(), filePath.c_str(), FALSE)) {
                spdlog::info("文件复制恢复成功: {} -> {}", backupPathStr, filePathStr);
                return true;
            }
        }

        spdlog::error("文件恢复失败: {}", filePathStr);
        return false;
    }
    catch (const std::exception& e) {
        spdlog::error("文件恢复异常: {}", e.what());
        return false;
    }
}

bool RollbackManager::RestoreRegistry(const std::wstring& originalData) const {
    try {
        auto dataStr = std::string(originalData.begin(), originalData.end());
        auto j = json::parse(dataStr);

        std::string backupFileStr = j.value("backupFile", "");
        if (!backupFileStr.empty()) {
            std::wstring backupFile(backupFileStr.begin(), backupFileStr.end());
            if (SystemBackupManager::ImportRegistryFile(backupFile)) {
                spdlog::info("注册表恢复成功: {}", backupFileStr);
                return true;
            }
        }

        // 对于 RegistryModify，直接恢复值
        if (j.contains("rootKey") && j.contains("subKey") && j.contains("valueName")) {
            auto rootKey = reinterpret_cast<HKEY>(j["rootKey"].get<intptr_t>());
            std::wstring subKey(j["subKey"].get<std::string>().begin(), j["subKey"].get<std::string>().end());
            std::wstring valueName(j["valueName"].get<std::string>().begin(), j["valueName"].get<std::string>().end());
            DWORD type = j.value("type", REG_SZ);

            HKEY hKey = nullptr;
            if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                if (j.contains("data") && j["data"].is_binary()) {
                    auto& binData = j["data"].get_binary();
                    LSTATUS status = RegSetValueExW(hKey, valueName.c_str(), 0, type,
                        binData.data(), static_cast<DWORD>(binData.size()));
                    RegCloseKey(hKey);
                    return status == ERROR_SUCCESS;
                }
                RegCloseKey(hKey);
            }
        }

        spdlog::error("注册表恢复失败");
        return false;
    }
    catch (const std::exception& e) {
        spdlog::error("注册表恢复异常: {}", e.what());
        return false;
    }
}

bool RollbackManager::RestoreService(const std::wstring& originalData) const {
    try {
        auto dataStr = std::string(originalData.begin(), originalData.end());
        auto j = json::parse(dataStr);

        std::string serviceNameStr = j.value("serviceName", "");
        DWORD originalStartType = j.value("originalStartType", SERVICE_DEMAND_START);

        std::wstring serviceName(serviceNameStr.begin(), serviceNameStr.end());

        // 打开服务管理器
        SC_HANDLE hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
        if (!hSCManager) return false;

        SC_HANDLE hService = OpenServiceW(hSCManager, serviceName.c_str(), SERVICE_CHANGE_CONFIG);
        if (hService) {
            BOOL success = ChangeServiceConfigW(
                hService,
                SERVICE_NO_CHANGE,
                originalStartType,
                SERVICE_NO_CHANGE,
                NULL, NULL, NULL, NULL, NULL, NULL, NULL);

            CloseServiceHandle(hService);
            CloseServiceHandle(hSCManager);

            if (success) {
                spdlog::info("服务恢复成功: {}", serviceNameStr);
            }
            return success;
        }

        CloseServiceHandle(hSCManager);
        return false;
    }
    catch (const std::exception& e) {
        spdlog::error("服务恢复异常: {}", e.what());
        return false;
    }
}

bool RollbackManager::RestoreStartup(const std::wstring& originalData) const {
    try {
        auto dataStr = std::string(originalData.begin(), originalData.end());
        auto j = json::parse(dataStr);

        std::string registryPathStr = j.value("registryPath", "");
        std::string itemNameStr = j.value("itemName", "");
        std::string originalValueStr = j.value("originalValue", "");

        std::wstring registryPath(registryPathStr.begin(), registryPathStr.end());
        std::wstring itemName(itemNameStr.begin(), itemNameStr.end());
        std::wstring originalValue(originalValueStr.begin(), originalValueStr.end());

        // 解析注册表路径，恢复启动项值
        HKEY rootKey = HKEY_CURRENT_USER;
        std::wstring subKey = registryPath;

        // 处理 HKLM 路径
        if (registryPath.find(L"HKLM") == 0 || registryPath.find(L"HKEY_LOCAL_MACHINE") == 0) {
            rootKey = HKEY_LOCAL_MACHINE;
            size_t slashPos = registryPath.find(L'\\');
            if (slashPos != std::wstring::npos) {
                subKey = registryPath.substr(slashPos + 1);
            }
        } else if (registryPath.find(L"HKCU") == 0 || registryPath.find(L"HKEY_CURRENT_USER") == 0) {
            rootKey = HKEY_CURRENT_USER;
            size_t slashPos = registryPath.find(L'\\');
            if (slashPos != std::wstring::npos) {
                subKey = registryPath.substr(slashPos + 1);
            }
        }

        HKEY hKey = nullptr;
        if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
            LSTATUS status = RegSetValueExW(hKey, itemName.c_str(), 0, REG_SZ,
                reinterpret_cast<const BYTE*>(originalValue.c_str()),
                static_cast<DWORD>((originalValue.size() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
            return status == ERROR_SUCCESS;
        }

        return false;
    }
    catch (const std::exception& e) {
        spdlog::error("启动项恢复异常: {}", e.what());
        return false;
    }
}

bool RollbackManager::RestoreTask(const std::wstring& originalData) const {
    try {
        auto dataStr = std::string(originalData.begin(), originalData.end());
        auto j = json::parse(dataStr);

        std::string taskNameStr = j.value("taskName", "");
        std::string taskPathStr = j.value("taskPath", "");

        std::wstring taskName(taskNameStr.begin(), taskNameStr.end());
        std::wstring taskPath(taskPathStr.begin(), taskPathStr.end());

        // 启用计划任务
        CoInitializeEx(NULL, COINIT_MULTITHREADED);

        ITaskService* pService = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER,
            IID_ITaskService, reinterpret_cast<void**>(&pService));

        if (SUCCEEDED(hr)) {
            hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
            if (SUCCEEDED(hr)) {
                ITaskFolder* pFolder = nullptr;
                hr = pService->GetFolder(_bstr_t(taskPath.c_str()), &pFolder);
                if (SUCCEEDED(hr)) {
                    IRegisteredTask* pTask = nullptr;
                    hr = pFolder->GetTask(_bstr_t(taskName.c_str()), &pTask);
                    if (SUCCEEDED(hr)) {
                        pTask->put_Enabled(VARIANT_TRUE);
                        pTask->Release();
                    }
                    pFolder->Release();
                }
            }
            pService->Release();
        }

        CoUninitialize();
        return SUCCEEDED(hr);
    }
    catch (const std::exception& e) {
        spdlog::error("计划任务恢复异常: {}", e.what());
        return false;
    }
}

// ── 持久化 ──

std::wstring RollbackManager::GenerateTransactionId() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return L"TX_" + std::to_wstring(ms);
}

std::wstring RollbackManager::GetLogFilePath() {
    wchar_t appDataPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        return std::wstring(appDataPath) + L"\\IceClean\\" + kLogFileName;
    }
    return kLogFileName;
}

void RollbackManager::SaveRollbackLog() const {
    auto path = GetLogFilePath();

    // 确保目录存在
    auto dir = path.substr(0, path.find_last_of(L'\\'));
    CreateDirectoryW(dir.c_str(), NULL);

    try {
        json transactions = json::array();

        for (const auto& tx : m_transactions) {
            json jTx;
            jTx["transactionId"] = std::string(tx->transactionId.begin(), tx->transactionId.end());
            jTx["description"] = std::string(tx->description.begin(), tx->description.end());
            jTx["createTime"] = std::chrono::system_clock::to_time_t(tx->createTime);
            jTx["committed"] = tx->committed;
            jTx["rolledBack"] = tx->rolledBack;
            jTx["totalSize"] = tx->totalSize;

            json steps = json::array();
            for (const auto& step : tx->steps) {
                json jStep;
                jStep["type"] = static_cast<int>(step.type);
                jStep["description"] = std::string(step.description.begin(), step.description.end());
                jStep["originalData"] = std::string(step.originalData.begin(), step.originalData.end());
                jStep["executed"] = step.executed;
                jStep["rolledBack"] = step.rolledBack;
                steps.push_back(jStep);
            }
            jTx["steps"] = steps;
            transactions.push_back(jTx);
        }

        std::ofstream file(path);
        if (file.is_open()) {
            file << transactions.dump(2);
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("保存回滚日志失败: {}", e.what());
    }
}

void RollbackManager::LoadRollbackLog() {
    auto path = GetLogFilePath();
    try {
        std::ifstream file(path);
        if (!file.is_open()) return;

        json transactions;
        file >> transactions;

        std::lock_guard<std::mutex> lock(m_mutex);
        m_transactions.clear();

        for (const auto& jTx : transactions) {
            auto tx = std::make_shared<Models::RollbackTransaction>();
            auto idStr = jTx.value("transactionId", "");
            tx->transactionId = std::wstring(idStr.begin(), idStr.end());
            auto descStr = jTx.value("description", "");
            tx->description = std::wstring(descStr.begin(), descStr.end());
            tx->createTime = std::chrono::system_clock::from_time_t(jTx.value("createTime", time_t(0)));
            tx->committed = jTx.value("committed", false);
            tx->rolledBack = jTx.value("rolledBack", false);
            tx->totalSize = jTx.value("totalSize", int64_t(0));

            if (jTx.contains("steps")) {
                for (const auto& jStep : jTx["steps"]) {
                    Models::RollbackStep step;
                    step.type = static_cast<Models::RollbackStepType>(jStep.value("type", 0));
                    auto stepDesc = jStep.value("description", "");
                    step.description = std::wstring(stepDesc.begin(), stepDesc.end());
                    auto stepData = jStep.value("originalData", "");
                    step.originalData = std::wstring(stepData.begin(), stepData.end());
                    step.executed = jStep.value("executed", false);
                    step.rolledBack = jStep.value("rolledBack", false);
                    tx->steps.push_back(step);
                }
            }

            m_transactions.push_back(tx);
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("加载回滚日志失败: {}", e.what());
    }
}

int RollbackManager::CleanupOldTransactions(int keepCount) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 只清理已提交或已回滚的事务
    int removed = 0;
    while (static_cast<int>(m_transactions.size()) > keepCount) {
        // 找最旧的已完成事务
        for (auto it = m_transactions.begin(); it != m_transactions.end(); ++it) {
            if ((*it)->committed || (*it)->rolledBack) {
                m_transactions.erase(it);
                removed++;
                break;
            }
        }
        if (removed == 0) break; // 没有可清理的事务
    }

    if (removed > 0) {
        SaveRollbackLog();
    }
    return removed;
}

} // namespace IceClean::Core::Safety
