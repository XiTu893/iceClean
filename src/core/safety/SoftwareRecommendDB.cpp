#include "SoftwareRecommendDB.h"
#include "SoftwareRecommendFetcher.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <shlobj.h>
#include <filesystem>

namespace IceClean::Core::Safety {

using json = nlohmann::json;

// ── 单例 ──

SoftwareRecommendDB& SoftwareRecommendDB::Instance() {
    static SoftwareRecommendDB instance;
    return instance;
}

SoftwareRecommendDB::~SoftwareRecommendDB() {
    Shutdown();
}

// ── 初始化 ──

bool SoftwareRecommendDB::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return true;

    const auto dbPath = GetDBFilePath();

    // 确保目录存在
    std::filesystem::path dir = std::filesystem::path(dbPath).parent_path();
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    // 打开数据库
    int rc = sqlite3_open16(dbPath.c_str(), &m_db);
    if (rc != SQLITE_OK) {
        spdlog::error("无法打开推荐软件数据库: {}", sqlite3_errmsg(m_db));
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    // 设置密码（PRAGMA key）
    std::string pragmaSQL = std::string("PRAGMA key = '") + kDBPassword + "';";
    rc = sqlite3_exec(m_db, pragmaSQL.c_str(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        spdlog::warn("设置数据库密码失败（可能不支持SQLCipher）: {}", sqlite3_errmsg(m_db));
        // 继续执行，因为标准 SQLite 不支持 PRAGMA key
        // 但我们仍然保留密码字段以备将来迁移到 SQLCipher
    }

    // 创建表结构
    if (!CreateTables()) {
        spdlog::error("创建推荐软件数据库表失败");
        sqlite3_close(m_db);
        m_db = nullptr;
        return false;
    }

    m_initialized = true;
    spdlog::info("推荐软件数据库初始化成功");
    return true;
}

void SoftwareRecommendDB::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
    m_initialized = false;
}

// ── 数据操作 ──

bool SoftwareRecommendDB::SaveRecommendData(const Models::RecommendData& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_db) return false;

    // 清空旧数据
    if (!ClearAllTables()) return false;

    // 插入分类数据
    for (const auto& cat : data.categories) {
        std::string sql = "INSERT INTO recommend_categories (id, name, icon, sort_order) VALUES (?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("准备插入分类语句失败: {}", sqlite3_errmsg(m_db));
            continue;
        }

        // 转换 wstring 到 UTF-8 string
        std::string idUtf8(cat.id.begin(), cat.id.end());
        std::string nameUtf8(cat.name.begin(), cat.name.end());
        std::string iconUtf8(cat.icon.begin(), cat.icon.end());

        sqlite3_bind_text(stmt, 1, idUtf8.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, nameUtf8.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, iconUtf8.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, cat.sortOrder);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            spdlog::error("插入分类数据失败: {}", sqlite3_errmsg(m_db));
        }
        sqlite3_finalize(stmt);
    }

    // 插入软件数据
    for (const auto& sw : data.software) {
        std::string sql = "INSERT INTO recommend_software "
            "(id, name, description, version, category_id, download_url, official_url, "
            "icon_url, size_mb, platform, tags, is_recommended, sort_order) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            spdlog::error("准备插入软件语句失败: {}", sqlite3_errmsg(m_db));
            continue;
        }

        auto toUtf8 = [](const std::wstring& ws) -> std::string {
            return std::string(ws.begin(), ws.end());
        };

        // 拼接 tags 为逗号分隔字符串
        std::string tagsStr;
        for (size_t i = 0; i < sw.tags.size(); ++i) {
            if (i > 0) tagsStr += ",";
            tagsStr += toUtf8(sw.tags[i]);
        }

        sqlite3_bind_text(stmt, 1, toUtf8(sw.id).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, toUtf8(sw.name).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, toUtf8(sw.description).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, toUtf8(sw.version).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, toUtf8(sw.categoryId).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, toUtf8(sw.downloadUrl).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7, toUtf8(sw.officialUrl).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 8, toUtf8(sw.iconUrl).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 9, sw.sizeMb);
        sqlite3_bind_text(stmt, 10, toUtf8(sw.platform).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 11, tagsStr.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 12, sw.isRecommended ? 1 : 0);
        sqlite3_bind_int(stmt, 13, sw.sortOrder);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            spdlog::error("插入软件数据失败: {}", sqlite3_errmsg(m_db));
        }
        sqlite3_finalize(stmt);
    }

    // 记录更新时间
    RecordUpdateTime();

    spdlog::info("推荐软件数据保存成功: {} 个分类, {} 个软件",
                 data.categories.size(), data.software.size());
    return true;
}

Models::RecommendData SoftwareRecommendDB::LoadRecommendData() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    Models::RecommendData data;

    if (!m_initialized || !m_db) return data;

    data.categories = GetCategories();
    data.software = GetAllSoftware();

    // 读取更新时间
    auto lastUpdate = GetLastUpdateTime();
    if (lastUpdate != std::chrono::system_clock::time_point{}) {
        auto timeT = std::chrono::system_clock::to_time_t(lastUpdate);
        struct tm tmBuf {};
        localtime_s(&tmBuf, &timeT);
        wchar_t timeStr[32] = {};
        wcsftime(timeStr, 32, L"%Y-%m-%d", &tmBuf);
        data.updatedAt = timeStr;
    }

    return data;
}

std::vector<Models::RecommendCategory> SoftwareRecommendDB::GetCategories() const {
    std::vector<Models::RecommendCategory> categories;
    if (!m_db) return categories;

    const char* sql = "SELECT id, name, icon, sort_order FROM recommend_categories ORDER BY sort_order;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return categories;

    auto toWstr = [](const char* s) -> std::wstring {
        if (!s) return L"";
        return std::wstring(s, s + strlen(s));
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::RecommendCategory cat;
        cat.id = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        cat.name = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        cat.icon = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        cat.sortOrder = sqlite3_column_int(stmt, 3);
        categories.push_back(std::move(cat));
    }
    sqlite3_finalize(stmt);
    return categories;
}

std::vector<Models::RecommendedSoftware> SoftwareRecommendDB::GetSoftwareByCategory(const std::wstring& categoryId) const {
    auto all = GetAllSoftware();
    std::vector<Models::RecommendedSoftware> result;
    for (const auto& sw : all) {
        if (sw.categoryId == categoryId) {
            result.push_back(sw);
        }
    }
    return result;
}

std::vector<Models::RecommendedSoftware> SoftwareRecommendDB::GetAllSoftware() const {
    std::vector<Models::RecommendedSoftware> software;
    if (!m_db) return software;

    const char* sql = "SELECT id, name, description, version, category_id, download_url, "
                      "official_url, icon_url, size_mb, platform, tags, is_recommended, sort_order "
                      "FROM recommend_software ORDER BY sort_order;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return software;

    auto toWstr = [](const char* s) -> std::wstring {
        if (!s) return L"";
        return std::wstring(s, s + strlen(s));
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::RecommendedSoftware sw;
        sw.id = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        sw.name = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        sw.description = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        sw.version = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        sw.categoryId = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        sw.downloadUrl = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        sw.officialUrl = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        sw.iconUrl = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        sw.sizeMb = sqlite3_column_int(stmt, 8);
        sw.platform = toWstr(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9)));

        // 解析 tags
        std::string tagsStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        if (!tagsStr.empty()) {
            size_t start = 0, end = 0;
            while ((end = tagsStr.find(',', start)) != std::string::npos) {
                std::string tag = tagsStr.substr(start, end - start);
                sw.tags.push_back(std::wstring(tag.begin(), tag.end()));
                start = end + 1;
            }
            if (start < tagsStr.size()) {
                std::string tag = tagsStr.substr(start);
                sw.tags.push_back(std::wstring(tag.begin(), tag.end()));
            }
        }

        sw.isRecommended = sqlite3_column_int(stmt, 11) != 0;
        sw.sortOrder = sqlite3_column_int(stmt, 12);
        software.push_back(std::move(sw));
    }
    sqlite3_finalize(stmt);
    return software;
}

// ── 更新时间管理 ──

std::chrono::system_clock::time_point SoftwareRecommendDB::GetLastUpdateTime() const {
    if (!m_db) return {};

    const char* sql = "SELECT update_time FROM recommend_meta WHERE id = 1;";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return {};

    std::chrono::system_clock::time_point result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int64_t timestamp = sqlite3_column_int64(stmt, 0);
        result = std::chrono::system_clock::time_point{
            std::chrono::seconds{timestamp}};
    }
    sqlite3_finalize(stmt);
    return result;
}

bool SoftwareRecommendDB::NeedsUpdate() const {
    auto lastUpdate = GetLastUpdateTime();
    if (lastUpdate == std::chrono::system_clock::time_point{}) return true;

    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::hours>(now - lastUpdate);
    return duration.count() >= 24;
}

void SoftwareRecommendDB::RecordUpdateTime() {
    if (!m_db) return;

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();

    std::string sql = "INSERT OR REPLACE INTO recommend_meta (id, update_time) VALUES (1, " +
                      std::to_string(timestamp) + ");";
    ExecuteSQL(sql);
}

// ── 内部方法 ──

std::wstring SoftwareRecommendDB::GetDBFilePath() {
    wchar_t appDataPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath))) {
        return std::wstring(appDataPath) + L"\\IceClean\\recommend.db";
    }
    return L"recommend.db";
}

bool SoftwareRecommendDB::ExecuteSQL(const std::string& sql) const {
    if (!m_db) return false;
    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        spdlog::error("SQL执行失败: {} - {}", sql, errMsg ? errMsg : "unknown");
        if (errMsg) sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool SoftwareRecommendDB::CreateTables() {
    // 分类表
    const char* catSQL =
        "CREATE TABLE IF NOT EXISTS recommend_categories ("
        "  id TEXT PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  icon TEXT DEFAULT '',"
        "  sort_order INTEGER DEFAULT 0"
        ");";
    if (!ExecuteSQL(catSQL)) return false;

    // 软件表
    const char* swSQL =
        "CREATE TABLE IF NOT EXISTS recommend_software ("
        "  id TEXT PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  description TEXT DEFAULT '',"
        "  version TEXT DEFAULT '',"
        "  category_id TEXT NOT NULL,"
        "  download_url TEXT DEFAULT '',"
        "  official_url TEXT DEFAULT '',"
        "  icon_url TEXT DEFAULT '',"
        "  size_mb INTEGER DEFAULT 0,"
        "  platform TEXT DEFAULT 'win64',"
        "  tags TEXT DEFAULT '',"
        "  is_recommended INTEGER DEFAULT 0,"
        "  sort_order INTEGER DEFAULT 0"
        ");";
    if (!ExecuteSQL(swSQL)) return false;

    // 元数据表（更新时间等）
    const char* metaSQL =
        "CREATE TABLE IF NOT EXISTS recommend_meta ("
        "  id INTEGER PRIMARY KEY,"
        "  update_time INTEGER DEFAULT 0"
        ");";
    if (!ExecuteSQL(metaSQL)) return false;

    return true;
}

bool SoftwareRecommendDB::ClearAllTables() {
    if (!ExecuteSQL("DELETE FROM recommend_software;")) return false;
    if (!ExecuteSQL("DELETE FROM recommend_categories;")) return false;
    return true;
}

} // namespace IceClean::Core::Safety
