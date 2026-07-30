#pragma once
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <memory>
#include <mutex>
#include <windows.h>

namespace IceClean::Models {

// 回滚步骤类型
enum class RollbackStepType {
    FileDelete,         // 文件删除（移入回收站可恢复）
    FileMove,           // 文件移动
    RegistryDelete,     // 注册表项删除
    RegistryModify,     // 注册表值修改
    ServiceDisable,     // 服务禁用
    StartupDisable,     // 启动项禁用
    TaskDisable,        // 计划任务禁用
    Custom              // 自定义操作
};

// 回滚步骤
struct RollbackStep {
    RollbackStepType type;
    std::wstring description;           // 步骤描述
    std::wstring originalData;          // 原始数据（JSON格式）
    bool executed = false;              // 是否已执行
    bool rolledBack = false;            // 是否已回滚
    std::chrono::system_clock::time_point timestamp;
};

// 回滚事务
struct RollbackTransaction {
    std::wstring transactionId;         // 事务ID
    std::wstring description;           // 事务描述
    std::vector<RollbackStep> steps;    // 回滚步骤
    std::chrono::system_clock::time_point createTime;
    bool committed = false;             // 是否已提交（不可回滚）
    bool rolledBack = false;            // 是否已回滚
    int64_t totalSize = 0;             // 涉及的数据大小
};

} // namespace IceClean::Models

namespace IceClean::Core::Safety {

// 回滚管理器
// 提供操作回滚能力，支持事务式操作
// 每个危险操作在执行前注册回滚步骤，失败时可以逐步撤销
class RollbackManager {
public:
    // 获取单例
    static RollbackManager& Instance();

    // ── 事务管理 ──

    // 开始一个新事务，返回事务ID
    std::wstring BeginTransaction(const std::wstring& description);

    // 向当前事务添加回滚步骤
    void AddStep(const std::wstring& transactionId,
                 Models::RollbackStepType type,
                 const std::wstring& description,
                 const std::wstring& originalData);

    // 标记步骤为已执行
    void MarkStepExecuted(const std::wstring& transactionId, int stepIndex);

    // 提交事务（不可回滚）
    void CommitTransaction(const std::wstring& transactionId);

    // 回滚事务中的所有已执行步骤（逆序）
    bool RollbackTransaction(const std::wstring& transactionId);

    // 回滚最近的N个事务
    int RollbackRecentTransactions(int count);

    // ── 便捷注册方法 ──

    // 注册文件删除回滚（需要先备份或移入回收站）
    void RegisterFileDelete(const std::wstring& transactionId,
                            const std::wstring& filePath,
                            const std::wstring& backupPath = L"");

    // 注册注册表删除回滚
    void RegisterRegistryDelete(const std::wstring& transactionId,
                                HKEY rootKey,
                                const std::wstring& subKey);

    // 注册注册表值修改回滚
    void RegisterRegistryModify(const std::wstring& transactionId,
                                HKEY rootKey,
                                const std::wstring& subKey,
                                const std::wstring& valueName,
                                DWORD type,
                                const std::vector<BYTE>& originalData);

    // 注册服务禁用回滚
    void RegisterServiceDisable(const std::wstring& transactionId,
                                const std::wstring& serviceName,
                                DWORD originalStartType);

    // 注册启动项禁用回滚
    void RegisterStartupDisable(const std::wstring& transactionId,
                                const std::wstring& itemName,
                                const std::wstring& originalValue,
                                const std::wstring& registryPath);

    // 注册计划任务禁用回滚
    void RegisterTaskDisable(const std::wstring& transactionId,
                             const std::wstring& taskName,
                             const std::wstring& taskPath);

    // 注册自定义回滚操作
    void RegisterCustomRollback(const std::wstring& transactionId,
                                const std::wstring& description,
                                std::function<bool()> rollbackFunc);

    // ── 查询方法 ──

    // 获取事务信息
    std::vector<Models::RollbackTransaction> GetRecentTransactions(int count = 20) const;

    // 获取特定事务
    std::shared_ptr<Models::RollbackTransaction> GetTransaction(const std::wstring& transactionId) const;

    // 获取可回滚事务数量
    int GetRollableTransactionCount() const;

    // 获取事务总数
    int GetTotalTransactionCount() const;

    // ── 持久化 ──

    // 保存回滚日志到磁盘
    void SaveRollbackLog() const;

    // 加载回滚日志
    void LoadRollbackLog();

    // 清理已提交/已回滚的旧事务
    int CleanupOldTransactions(int keepCount = 50);

private:
    RollbackManager() = default;
    ~RollbackManager() = default;

    RollbackManager(const RollbackManager&) = delete;
    RollbackManager& operator=(const RollbackManager&) = delete;

    // 执行单个回滚步骤
    bool ExecuteRollbackStep(const Models::RollbackStep& step) const;

    // 执行文件恢复
    bool RestoreFile(const std::wstring& originalData) const;

    // 执行注册表恢复
    bool RestoreRegistry(const std::wstring& originalData) const;

    // 执行服务恢复
    bool RestoreService(const std::wstring& originalData) const;

    // 执行启动项恢复
    bool RestoreStartup(const std::wstring& originalData) const;

    // 执行计划任务恢复
    bool RestoreTask(const std::wstring& originalData) const;

    // 生成事务ID
    static std::wstring GenerateTransactionId();

    // 获取日志文件路径
    static std::wstring GetLogFilePath();

    // 自定义回滚函数存储
    struct CustomRollback {
        std::wstring transactionId;
        int stepIndex;
        std::function<bool()> func;
    };
    std::vector<CustomRollback> m_customRollbacks;

    // 事务存储
    std::vector<std::shared_ptr<Models::RollbackTransaction>> m_transactions;
    mutable std::mutex m_mutex;

    static constexpr const wchar_t* kLogFileName = L"rollback_log.json";
};

} // namespace IceClean::Core::Safety
