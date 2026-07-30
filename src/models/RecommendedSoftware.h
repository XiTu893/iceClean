#pragma once
#include <string>
#include <vector>
#include <chrono>

namespace IceClean::Models {

// 推荐软件分类
struct RecommendCategory {
    std::wstring id;            // 分类唯一标识
    std::wstring name;          // 分类名称
    std::wstring icon;          // 图标标识
    int sortOrder = 0;          // 排序权重
};

// 推荐软件项
struct RecommendedSoftware {
    std::wstring id;            // 软件唯一标识
    std::wstring name;          // 软件名称
    std::wstring description;   // 软件描述
    std::wstring version;       // 版本号
    std::wstring categoryId;    // 所属分类ID
    std::wstring downloadUrl;   // 下载地址
    std::wstring officialUrl;   // 官方网站
    std::wstring iconUrl;       // 图标URL
    int sizeMb = 0;             // 安装包大小(MB)
    std::wstring platform;      // 平台
    std::vector<std::wstring> tags; // 标签
    bool isRecommended = false; // 是否精选推荐
    int sortOrder = 0;          // 排序权重
};

// 推荐软件数据（从JSON解析后的完整结构）
struct RecommendData {
    int version = 0;                            // 数据版本号
    std::wstring updatedAt;                     // 更新时间
    std::vector<RecommendCategory> categories;  // 分类列表
    std::vector<RecommendedSoftware> software;  // 软件列表
};

} // namespace IceClean::Models
