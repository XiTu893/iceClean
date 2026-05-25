#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "ScanResult.h"

namespace IceClean::Models {

// 清理进度信息
struct CleanProgress {
    int currentItem = 0;        // 当前清理项索引
    int totalItems = 0;         // 总清理项数
    uint64_t cleanedSize = 0;   // 已清理大小
    uint64_t totalSize = 0;     // 总需清理大小
    std::wstring currentFile;   // 当前正在清理的文件
    bool isRunning = false;     // 是否正在清理
    bool isCancelled = false;   // 是否已取消
};

// 清理结果
struct CleanResult {
    bool success = false;
    uint64_t totalCleanedSize = 0;     // 总清理大小
    int cleanedFileCount = 0;          // 清理文件数
    int failedFileCount = 0;           // 失败文件数
    std::vector<std::wstring> failedFiles; // 失败文件列表
    std::wstring errorMessage;
};

} // namespace IceClean::Models
