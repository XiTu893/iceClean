#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace IceClean::Models {

// 操作类型
enum class OperationType {
    Clean,           // 清理
    Migrate,         // 迁移
    Optimize,        // 优化
    Restore          // 还原
};

// 操作记录
struct OperationRecord {
    OperationType type;
    std::wstring description;       // 操作描述
    uint64_t size;                  // 涉及大小
    std::chrono::system_clock::time_point timestamp;  // 操作时间
    bool success;                   // 是否成功
    std::wstring details;           // 详细信息(JSON)
};

} // namespace IceClean::Models
