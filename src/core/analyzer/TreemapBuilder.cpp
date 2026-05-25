#include "TreemapBuilder.h"
#include <cmath>
#include <numeric>
#include <algorithm>

namespace IceClean::Core::Analyzer {

void TreemapBuilder::Build(Models::DiskNode& rootNode, const Rect& rect, int minSize) {
    // 收集子节点并按大小降序排序
    if (rootNode.children.empty()) return;

    uint64_t totalSize = rootNode.GetTotalSize();
    if (totalSize == 0) return;

    // 计算总面积(像素)
    double totalArea = static_cast<double>(rect.width) * rect.height;

    // 收集非零大小的子节点指针
    std::vector<Models::DiskNode*> items;
    for (auto& child : rootNode.children) {
        if (child->GetTotalSize() > 0) {
            items.push_back(child.get());
        }
    }

    // 按大小降序排序
    std::sort(items.begin(), items.end(),
              [](const Models::DiskNode* a, const Models::DiskNode* b) {
                  return a->GetTotalSize() > b->GetTotalSize();
              });

    // 执行Squarified布局
    Squarify(items, rect, totalSize, minSize);

    // 递归处理每个子节点的子节点
    for (auto& child : rootNode.children) {
        if (child->isDirectory && !child->children.empty() &&
            child->width > minSize * 2 && child->height > minSize * 2) {
            // 子节点的矩形区域留出1像素边距
            Rect childRect;
            childRect.x = child->x + 1;
            childRect.y = child->y + 1;
            childRect.width = child->width - 2;
            childRect.height = child->height - 2;

            if (childRect.width > 0 && childRect.height > 0) {
                Build(*child, childRect, minSize);
            }
        }
    }
}

double TreemapBuilder::GetAreaRatio(uint64_t itemSize, uint64_t totalSize, double totalArea) {
    if (totalSize == 0) return 0.0;
    return static_cast<double>(itemSize) / static_cast<double>(totalSize) * totalArea;
}

double TreemapBuilder::WorstAspectRatio(const std::vector<double>& areas, double sideLength) {
    if (areas.empty() || sideLength <= 0) return 0.0;

    double totalArea = 0;
    for (double a : areas) totalArea += a;

    if (totalArea <= 0) return 0.0;

    double worst = 1.0;
    for (double area : areas) {
        if (area <= 0) continue;
        // 每个矩形的宽度 = totalArea / sideLength
        // 每个矩形的高度 = area / width = area * sideLength / totalArea
        double width = totalArea / sideLength;
        double height = area / width;
        if (height <= 0 || width <= 0) continue;

        double ratio = (width > height) ? width / height : height / width;
        worst = std::max(worst, ratio);
    }

    return worst;
}

void TreemapBuilder::LayoutRow(std::vector<Models::DiskNode*>& row, const Rect& rect,
                                 bool isHorizontal, uint64_t rowArea, uint64_t totalSize) {
    double totalArea = static_cast<double>(rect.width) * rect.height;
    double rowTotalArea = GetAreaRatio(rowArea, totalSize, totalArea);

    if (isHorizontal) {
        // 水平布局：行在左侧，宽度由行面积决定
        double rowWidth = (rect.height > 0) ? rowTotalArea / rect.height : 0;
        int rowWidthInt = std::max(1, static_cast<int>(std::round(rowWidth)));

        double yOffset = 0;
        for (auto* node : row) {
            double nodeArea = GetAreaRatio(node->GetTotalSize(), totalSize, totalArea);
            double nodeHeight = (rowWidth > 0) ? nodeArea / rowWidth : 0;
            int nodeHeightInt = std::max(1, static_cast<int>(std::round(nodeHeight)));

            node->x = rect.x;
            node->y = rect.y + static_cast<int>(std::round(yOffset));
            node->width = std::min(rowWidthInt, rect.width);
            node->height = std::min(nodeHeightInt,
                rect.height - static_cast<int>(std::round(yOffset)));

            yOffset += nodeHeight;
        }
    } else {
        // 垂直布局：行在顶部，高度由行面积决定
        double rowHeight = (rect.width > 0) ? rowTotalArea / rect.width : 0;
        int rowHeightInt = std::max(1, static_cast<int>(std::round(rowHeight)));

        double xOffset = 0;
        for (auto* node : row) {
            double nodeArea = GetAreaRatio(node->GetTotalSize(), totalSize, totalArea);
            double nodeWidth = (rowHeight > 0) ? nodeArea / rowHeight : 0;
            int nodeWidthInt = std::max(1, static_cast<int>(std::round(nodeWidth)));

            node->x = rect.x + static_cast<int>(std::round(xOffset));
            node->y = rect.y;
            node->width = std::min(nodeWidthInt,
                rect.width - static_cast<int>(std::round(xOffset)));
            node->height = std::min(rowHeightInt, rect.height);

            xOffset += nodeWidth;
        }
    }
}

void TreemapBuilder::Squarify(std::vector<Models::DiskNode*>& items, const Rect& rect,
                                uint64_t totalSize, int minSize) {
    if (items.empty()) return;
    if (rect.width <= 0 || rect.height <= 0) return;

    // 过滤掉太小的项目
    double totalArea = static_cast<double>(rect.width) * rect.height;

    size_t startIdx = 0;
    while (startIdx < items.size()) {
        // 确定短边方向
        bool isHorizontal = rect.width >= rect.height;
        double shortSide = isHorizontal ? rect.height : rect.width;

        if (shortSide <= 0) break;

        // Squarified算法：逐步添加节点到当前行，直到宽高比变差
        std::vector<Models::DiskNode*> currentRow;
        uint64_t rowArea = 0;
        double bestWorst = std::numeric_limits<double>::max();

        size_t i = startIdx;
        for (; i < items.size(); ++i) {
            std::vector<Models::DiskNode*> testRow = currentRow;
            testRow.push_back(items[i]);
            uint64_t testRowArea = rowArea + items[i]->GetTotalSize();

            // 计算测试行的面积列表
            std::vector<double> testAreas;
            for (auto* node : testRow) {
                testAreas.push_back(GetAreaRatio(node->GetTotalSize(), totalSize, totalArea));
            }

            double worst = WorstAspectRatio(testAreas, shortSide);

            if (worst <= bestWorst || currentRow.empty()) {
                // 添加此节点使宽高比更好或不变
                currentRow.push_back(items[i]);
                rowArea = testRowArea;
                bestWorst = worst;
            } else {
                // 添加此节点使宽高比变差，停止
                break;
            }
        }

        if (currentRow.empty()) {
            // 至少放一个节点
            currentRow.push_back(items[startIdx]);
            rowArea = items[startIdx]->GetTotalSize();
            i = startIdx + 1;
        }

        // 计算行占用后剩余的矩形区域
        double rowTotalArea = GetAreaRatio(rowArea, totalSize, totalArea);

        Rect remainingRect = rect;
        if (isHorizontal) {
            // 行在左侧
            double rowWidth = (rect.height > 0) ? rowTotalArea / rect.height : 0;
            int rowWidthInt = std::max(1, static_cast<int>(std::round(rowWidth)));

            LayoutRow(currentRow, rect, true, rowArea, totalSize);

            remainingRect.x = rect.x + rowWidthInt;
            remainingRect.width = std::max(0, rect.width - rowWidthInt);
        } else {
            // 行在顶部
            double rowHeight = (rect.width > 0) ? rowTotalArea / rect.width : 0;
            int rowHeightInt = std::max(1, static_cast<int>(std::round(rowHeight)));

            LayoutRow(currentRow, rect, false, rowArea, totalSize);

            remainingRect.y = rect.y + rowHeightInt;
            remainingRect.height = std::max(0, rect.height - rowHeightInt);
        }

        // 继续处理剩余节点
        startIdx = i;

        if (remainingRect.width > 0 && remainingRect.height > 0) {
            // 构建剩余项目列表
            std::vector<Models::DiskNode*> remainingItems(
                items.begin() + startIdx, items.end());

            if (!remainingItems.empty()) {
                Squarify(remainingItems, remainingRect, totalSize, minSize);
            }
            break;
        }
    }

    // 处理因空间不足而未布局的节点
    for (size_t i = startIdx; i < items.size(); ++i) {
        items[i]->x = 0;
        items[i]->y = 0;
        items[i]->width = 0;
        items[i]->height = 0;
    }
}

} // namespace IceClean::Core::Analyzer
