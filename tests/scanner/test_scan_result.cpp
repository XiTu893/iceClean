// ScanResult 模型测试

#include <gtest/gtest.h>
#include "models/ScanResult.h"

using namespace IceClean::Models;

TEST(ScanResultTest, DefaultEmptyResult) {
    ScanResult result;
    EXPECT_TRUE(result.categories.empty());
    EXPECT_EQ(result.totalSize, 0);
    EXPECT_EQ(result.totalItems, 0);
}

TEST(ScanResultTest, AddCategory) {
    ScanResult result;
    ScanCategory cat;
    cat.type = ScanCategoryType::TempFiles;
    cat.name = L"临时文件";
    cat.totalSize = 1024 * 1024; // 1MB
    cat.itemCount = 10;

    result.categories.push_back(cat);
    result.totalSize += cat.totalSize;
    result.totalItems += cat.itemCount;

    EXPECT_EQ(result.categories.size(), 1u);
    EXPECT_EQ(result.totalSize, 1024 * 1024);
    EXPECT_EQ(result.totalItems, 10);
}

TEST(ScanResultTest, MultipleCategories) {
    ScanResult result;

    // 添加多个类别
    for (int i = 0; i < 5; ++i) {
        ScanCategory cat;
        cat.type = static_cast<ScanCategoryType>(i);
        cat.totalSize = 100 * (i + 1);
        cat.itemCount = i + 1;
        result.categories.push_back(cat);
        result.totalSize += cat.totalSize;
        result.totalItems += cat.itemCount;
    }

    EXPECT_EQ(result.categories.size(), 5u);
    EXPECT_GT(result.totalSize, 0u);
    EXPECT_GT(result.totalItems, 0u);
}

TEST(ScanItemTest, DefaultValues) {
    ScanItem item;
    EXPECT_TRUE(item.path.empty());
    EXPECT_EQ(item.size, 0);
}

TEST(SafetyLevelTest, EnumValues) {
    // 确保安全级别枚举值正确
    SafetyLevel safe = SafetyLevel::Safe;
    SafetyLevel caution = SafetyLevel::Caution;
    SafetyLevel dangerous = SafetyLevel::Dangerous;

    EXPECT_NE(safe, caution);
    EXPECT_NE(caution, dangerous);
    EXPECT_NE(safe, dangerous);
}
