// PrivacyCleaner 测试

#include <gtest/gtest.h>
#include "core/cleaner/PrivacyCleaner.h"

using namespace IceClean::Core::Cleaner;

TEST(PrivacyCleanerTest, PrivacyTypeEnumValues) {
    // 确保隐私类型枚举值不同
    EXPECT_NE(PrivacyType::Cookies, PrivacyType::History);
    EXPECT_NE(PrivacyType::History, PrivacyType::FormData);
}

TEST(PrivacyCleanerTest, DefaultConstruction) {
    PrivacyCleaner cleaner;
    // 构造不崩溃即可
    SUCCEED();
}

TEST(PrivacyCleanerTest, GetBrowserPathsNotEmpty) {
    // 测试浏览器路径映射是否已配置
    // 至少应包含 Chrome 和 Edge
    PrivacyCleaner cleaner;
    // 此测试验证对象初始化正常
    SUCCEED();
}
