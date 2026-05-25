#pragma once
#include <string>
#include <vector>
#include <cstdint>

// FILETIME 定义在 <windows.h> 中
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace IceClean::Models {

// 安全等级
enum class SafetyRating {
    Safe,       // 安全，可放心清理
    Caution,    // 谨慎，需用户确认
    Dangerous   // 危险，不推荐清理
};

// 单个扫描到的文件项
struct ScanFileItem {
    std::wstring path;          // 文件路径
    uint64_t size;              // 文件大小(字节)
    FILETIME lastWriteTime;     // 最后修改时间
    bool selected = true;       // 是否选中清理
};

// 扫描结果分类
struct ScanCategory {
    std::wstring name;          // 分类名称(如"系统临时文件")
    std::wstring description;   // 分类描述
    SafetyRating safety;        // 安全等级
    std::wstring icon;          // 图标标识
    std::vector<ScanFileItem> items;  // 该分类下的文件列表
    uint64_t totalSize = 0;     // 总大小
    bool expanded = false;      // UI: 是否展开
    bool selected = true;       // UI: 是否选中(安全项默认选中，谨慎项默认不选)
};

// 完整扫描结果
struct ScanResult {
    std::vector<ScanCategory> categories;
    uint64_t totalSize = 0;             // 总可清理大小
    uint64_t selectedSize = 0;          // 选中项总大小
    int totalFileCount = 0;             // 总文件数
    int selectedFileCount = 0;          // 选中文件数
    double scanDurationMs = 0;          // 扫描耗时(毫秒)
};

} // namespace IceClean::Models
