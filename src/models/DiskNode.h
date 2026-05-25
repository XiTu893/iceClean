#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace IceClean::Models {

// 磁盘节点(用于矩形树图)
struct DiskNode {
    std::wstring name;             // 文件/文件夹名
    std::wstring fullPath;         // 完整路径
    uint64_t size;                 // 大小(字节)
    bool isDirectory;              // 是否为目录
    std::vector<std::shared_ptr<DiskNode>> children;  // 子节点

    // 矩形树图布局数据(运行时计算)
    int x = 0, y = 0;             // 矩形位置
    int width = 0, height = 0;    // 矩形尺寸

    uint64_t GetTotalSize() const {
        if (children.empty()) return size;
        uint64_t total = 0;
        for (const auto& child : children) {
            total += child->GetTotalSize();
        }
        return total;
    }
};

} // namespace IceClean::Models
