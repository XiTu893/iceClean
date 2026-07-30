// FormatUtil 测试

#include <gtest/gtest.h>
#include "utils/FormatUtil.h"

using namespace IceClean::Utils;

TEST(FormatUtilTest, FormatFileSizeZero) {
    EXPECT_EQ(FormatUtil::FormatFileSize(0), L"0 B");
}

TEST(FormatUtilTest, FormatFileSizeBytes) {
    EXPECT_EQ(FormatUtil::FormatFileSize(512), L"512 B");
}

TEST(FormatUtilTest, FormatFileSizeKB) {
    auto result = FormatUtil::FormatFileSize(1024);
    EXPECT_TRUE(result.find(L"KB") != std::wstring::npos || result.find(L"1.0") != std::wstring::npos);
}

TEST(FormatUtilTest, FormatFileSizeMB) {
    auto result = FormatUtil::FormatFileSize(1024 * 1024);
    EXPECT_TRUE(result.find(L"MB") != std::wstring::npos);
}

TEST(FormatUtilTest, FormatFileSizeGB) {
    auto result = FormatUtil::FormatFileSize(1024ULL * 1024 * 1024);
    EXPECT_TRUE(result.find(L"GB") != std::wstring::npos);
}

TEST(FormatUtilTest, FormatFileSizeTB) {
    auto result = FormatUtil::FormatFileSize(1024ULL * 1024 * 1024 * 1024);
    EXPECT_TRUE(result.find(L"TB") != std::wstring::npos || result.find(L"GB") != std::wstring::npos);
}

TEST(FormatUtilTest, FormatFileSizeLargeValue) {
    // 10 GB
    auto result = FormatUtil::FormatFileSize(10ULL * 1024 * 1024 * 1024);
    EXPECT_FALSE(result.empty());
}
