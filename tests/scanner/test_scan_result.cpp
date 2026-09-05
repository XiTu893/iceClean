// ScanResult 模型测试

#include <gtest/gtest.h>
#include "models/ScanResult.h"

using namespace IceClean::Models;

TEST(ScanResultTest, DefaultEmptyResult) {
    ScanResult result;
    EXPECT_TRUE(result.categories.empty());
    EXPECT_EQ(result.totalSize, 0u);
    EXPECT_EQ(result.totalFileCount, 0);
}

TEST(ScanResultTest, AddCategoryWithItems) {
    ScanResult result;

    ScanFileItem item1;
    item1.path = L"C:\\temp\\test1.tmp";
    item1.size = 1024;
    item1.selected = true;

    ScanFileItem item2;
    item2.path = L"C:\\temp\\test2.tmp";
    item2.size = 2048;
    item2.selected = true;

    ScanCategory cat;
    cat.name = L"临时文件";
    cat.safety = SafetyRating::Safe;
    cat.totalSize = 3072;
    cat.items.push_back(item1);
    cat.items.push_back(item2);

    result.categories.push_back(cat);
    result.totalSize += cat.totalSize;
    result.totalFileCount += static_cast<int>(cat.items.size());

    EXPECT_EQ(result.categories.size(), 1u);
    EXPECT_EQ(result.totalSize, 3072u);
    EXPECT_EQ(result.totalFileCount, 2);
}

TEST(ScanResultTest, MultipleCategories) {
    ScanResult result;

    for (int i = 0; i < 5; ++i) {
        ScanCategory cat;
        cat.name = std::wstring(16, L'类' + static_cast<wchar_t>(i % 26));
        cat.safety = (i % 3 == 0) ? SafetyRating::Safe
                    : (i % 3 == 1) ? SafetyRating::Caution
                                   : SafetyRating::Dangerous;
        cat.totalSize = 100 * (i + 1);
        result.categories.push_back(cat);
        result.totalSize += cat.totalSize;
    }

    result.totalFileCount = static_cast<int>(result.categories.size());
    EXPECT_EQ(result.categories.size(), 5u);
    EXPECT_GT(result.totalSize, 0u);
    EXPECT_GT(result.totalFileCount, 0);
}

TEST(ScanItemTest, DefaultValues) {
    ScanFileItem item;
    EXPECT_TRUE(item.path.empty());
    EXPECT_EQ(item.size, 0u);
    EXPECT_FALSE(item.lastWriteTime.dwLowDateTime);
    EXPECT_TRUE(item.selected);
}

TEST(SafetyRatingTest, EnumValues) {
    EXPECT_NE(static_cast<int>(SafetyRating::Safe), static_cast<int>(SafetyRating::Caution));
    EXPECT_NE(static_cast<int>(SafetyRating::Caution), static_cast<int>(SafetyRating::Dangerous));
    EXPECT_NE(static_cast<int>(SafetyRating::Safe), static_cast<int>(SafetyRating::Dangerous));
}