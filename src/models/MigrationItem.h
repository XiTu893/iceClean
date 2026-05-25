#pragma once
#include <string>
#include <cstdint>

namespace IceClean::Models {

// 迁移类型
enum class MigrationType {
    SteamGame,         // Steam游戏
    UserFolder,        // 用户文件夹(桌面/文档等)
    WeChatCache,       // 微信缓存
    QQCache,           // QQ缓存
    CustomFolder,      // 自定义文件夹
    LargeSoftware      // 大型软件
};

// 迁移建议
enum class MigrationAdvice {
    Recommended,       // 推荐迁移
    Possible,          // 可以迁移
    NotRecommended     // 不建议迁移
};

// 迁移项
struct MigrationItem {
    std::wstring name;             // 名称
    std::wstring sourcePath;       // 源路径(C盘)
    std::wstring targetPath;       // 目标路径(其他盘)
    uint64_t size;                 // 大小(字节)
    MigrationType type;            // 迁移类型
    MigrationAdvice advice;        // 迁移建议
    bool selected = false;         // 是否选中迁移
    bool migrated = false;         // 是否已迁移
};

// 迁移进度
struct MigrationProgress {
    int currentItem = 0;
    int totalItems = 0;
    uint64_t migratedSize = 0;
    uint64_t totalSize = 0;
    uint64_t speed = 0;            // 当前速度(字节/秒)
    std::wstring currentFile;      // 当前正在迁移的文件
    bool isRunning = false;
    bool isCancelled = false;
};

// 迁移结果
struct MigrationResult {
    bool success = false;
    uint64_t totalMigratedSize = 0;
    int migratedCount = 0;
    int failedCount = 0;
    std::wstring errorMessage;
};

} // namespace IceClean::Models
