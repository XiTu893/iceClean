#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace IceClean::Models {

// 文件变更类型
enum class FileChangeType {
    Added,       // 新增文件
    Removed,     // 删除文件
    Modified,    // 修改文件
    RenamedOld,  // 重命名（旧名）
    RenamedNew   // 重命名（新名）
};

// 文件变更记录
struct FileChangeRecord {
    std::wstring filePath;        // 变更文件路径
    std::wstring fileName;         // 文件名
    FileChangeType changeType = FileChangeType::Modified;
    FILETIME timestamp;            // 变更时间
    uint64_t fileSize = 0;         // 文件大小
};

// 监控目录配置
struct FileWatchConfig {
    std::wstring watchPath;        // 监控路径
    bool watchSubtree = true;      // 是否监控子目录
    bool watchAdded = true;        // 监控新增
    bool watchRemoved = true;      // 监控删除
    bool watchModified = true;     // 监控修改
    bool watchRenamed = true;      // 监控重命名
    bool isEnabled = true;         // 是否启用
};

// 文件监控统计
struct FileWatchStats {
    int totalChanges = 0;          // 总变更数
    int todayChanges = 0;          // 今日变更数
    int watchedPaths = 0;          // 监控路径数
    bool isRunning = false;        // 是否正在监控
};

} // namespace IceClean::Models
