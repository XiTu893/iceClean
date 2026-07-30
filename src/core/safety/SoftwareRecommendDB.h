#pragma once
#include "models/RecommendedSoftware.h"
#include <string>
#include <vector>
#include <mutex>
#include <chrono>

struct sqlite3;

namespace IceClean::Core::Safety {

// 推荐软件数据库管理器
// 使用 SQLite 存储推荐软件数据，数据库设置密码保护
class SoftwareRecommendDB {
public:
    // 获取单例
    static SoftwareRecommendDB& Instance();

    // 初始化数据库（创建表结构、打开连接）
    bool Initialize();

    // 关闭数据库连接
    void Shutdown();

    // 保存推荐软件数据（从网络获取后调用）
    bool SaveRecommendData(const Models::RecommendData& data);

    // 读取推荐软件数据（从本地数据库读取）
    Models::RecommendData LoadRecommendData() const;

    // 获取所有分类
    std::vector<Models::RecommendCategory> GetCategories() const;

    // 获取指定分类的软件列表
    std::vector<Models::RecommendedSoftware> GetSoftwareByCategory(const std::wstring& categoryId) const;

    // 获取所有推荐软件
    std::vector<Models::RecommendedSoftware> GetAllSoftware() const;

    // 获取上次更新时间
    std::chrono::system_clock::time_point GetLastUpdateTime() const;

    // 是否需要更新（距上次更新超过24小时）
    bool NeedsUpdate() const;

    // 记录更新时间
    void RecordUpdateTime();

    // 数据库是否已初始化
    bool IsInitialized() const { return m_initialized; }

private:
    SoftwareRecommendDB() = default;
    ~SoftwareRecommendDB();

    SoftwareRecommendDB(const SoftwareRecommendDB&) = delete;
    SoftwareRecommendDB& operator=(const SoftwareRecommendDB&) = delete;

    // 获取数据库文件路径
    static std::wstring GetDBFilePath();

    // 执行SQL语句（无返回值）
    bool ExecuteSQL(const std::string& sql) const;

    // 创建表结构
    bool CreateTables();

    // 清空所有数据表
    bool ClearAllTables();

    sqlite3* m_db = nullptr;
    bool m_initialized = false;
    mutable std::mutex m_mutex;

    // 数据库密码（硬编码）
    static constexpr const char* kDBPassword = "IceClean2026@Rec0mmend!";
};

} // namespace IceClean::Core::Safety
