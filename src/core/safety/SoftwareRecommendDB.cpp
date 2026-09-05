#include "SoftwareRecommendDB.h"
#include "SoftwareRecommendFetcher.h"
#include "SoftwareRecommendSeed.h"
#include "utils/JsonUtil.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <shlobj.h>
#include <filesystem>

namespace IceClean::Core::Safety {

using json = nlohmann::json;

// SQLite 文本为 UTF-8，宽字符为 UTF-16：统一使用 JsonUtil 的系统级转换
using IceClean::Utils::JsonUtil;

namespace {
// meta 表复用：id=1 更新时间戳，id=2 数据集版本
void SetMetaInt(sqlite3* db, int id, int value) {
    if (!db) return;
    std::string sql = "INSERT OR REPLACE INTO recommend_meta (id, update_time) VALUES (" +
                      std::to_string(id) + ", " + std::to_string(value) + ");";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        if (errMsg) sqlite3_free(errMsg);
    }
}
} // namespace

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
        std::string idUtf8 = JsonUtil::WideToUtf8(cat.id);
        std::string nameUtf8 = JsonUtil::WideToUtf8(cat.name);
        std::string iconUtf8 = JsonUtil::WideToUtf8(cat.icon);

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
            return JsonUtil::WideToUtf8(ws);
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

    // 记录更新时间与数据集版本（远程/种子统一走此入口）
    RecordUpdateTime();
    SetMetaInt(m_db, 2, data.version);

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

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::RecommendCategory cat;
        cat.id = JsonUtil::Utf8ToWide(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        cat.name = JsonUtil::Utf8ToWide(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        cat.icon = JsonUtil::Utf8ToWide(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
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

    auto colW = [&stmt](int col) -> std::wstring {
        const auto* text = sqlite3_column_text(stmt, col);
        return text ? JsonUtil::Utf8ToWide(reinterpret_cast<const char*>(text)) : std::wstring{};
    };

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Models::RecommendedSoftware sw;
        sw.id = colW(0);
        sw.name = colW(1);
        sw.description = colW(2);
        sw.version = colW(3);
        sw.categoryId = colW(4);
        sw.downloadUrl = colW(5);
        sw.officialUrl = colW(6);
        sw.iconUrl = colW(7);
        sw.sizeMb = sqlite3_column_int(stmt, 8);
        sw.platform = colW(9);

        // 解析 tags
        const auto* tagsText = sqlite3_column_text(stmt, 10);
        if (tagsText) {
            std::string tagsStr(reinterpret_cast<const char*>(tagsText));
            size_t start = 0, end = 0;
            while ((end = tagsStr.find(',', start)) != std::string::npos) {
                sw.tags.push_back(JsonUtil::Utf8ToWide(tagsStr.substr(start, end - start)));
                start = end + 1;
            }
            if (start < tagsStr.size()) {
                sw.tags.push_back(JsonUtil::Utf8ToWide(tagsStr.substr(start)));
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

bool SoftwareRecommendDB::EnsureSeedLoaded() {
    // 版本化同步：本地版本 >= 种子版本则不覆盖（联网更新写入更高版本后不会被回退）
    const int seedVersion = GetSeedJsonVersion();
    if (GetDataVersion() >= seedVersion) return true;

    Models::RecommendData seed;
    if (!SoftwareRecommendFetcher::Instance().ParseJson(GetSeedJsonUtf8(), seed)) {
        spdlog::warn("解析内置推荐种子数据失败");
        return false;
    }
    const bool ok = SaveRecommendData(seed);
    spdlog::info("已导入/升级内置推荐种子 v{}: {} 分类 / {} 软件",
                 seedVersion, seed.categories.size(), seed.software.size());
    return ok;
}

int SoftwareRecommendDB::GetDataVersion() const {
    if (!m_db) return 0;
    const char* sql = "SELECT update_time FROM recommend_meta WHERE id = 2;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;
    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return version;
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
