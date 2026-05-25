#pragma once
#include "models/DiskNode.h"
#include <vector>
#include <memory>
#include <algorithm>
#include <cstdint>

namespace IceClean::Core::Analyzer {

// 矩形区域
struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

class TreemapBuilder {
public:
    // 构建矩形树图
    // rootNode: 磁盘空间树的根节点
    // rect: 可用矩形区域
    // minSize: 最小节点像素大小(小于此值的不显示)
    static void Build(Models::DiskNode& rootNode, const Rect& rect, int minSize = 2);

private:
    // Squarified Treemap算法实现
    static void Squarify(std::vector<Models::DiskNode*>& items, const Rect& rect,
                          uint64_t totalArea, int minSize);

    // 对一组节点进行布局(尝试使宽高比接近1:1)
    static void LayoutRow(std::vector<Models::DiskNode*>& row, const Rect& rect,
                           bool isHorizontal, uint64_t rowArea, uint64_t totalArea);

    // 计算一组节点的最差宽高比
    static double WorstAspectRatio(const std::vector<double>& areas, double sideLength);

    // 计算单个节点的面积比例
    static double GetAreaRatio(uint64_t itemSize, uint64_t totalSize, double totalArea);
};

} // namespace IceClean::Core::Analyzer
