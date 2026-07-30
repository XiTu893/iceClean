// Win32Util 测试

#include <gtest/gtest.h>
#include "utils/Win32Util.h"

using namespace IceClean::Utils;

TEST(Win32UtilTest, GetDiskSpace) {
    uint64_t totalBytes = 0;
    uint64_t freeBytes = 0;
    bool result = Win32Util::GetDiskSpace(L"C:\\", totalBytes, freeBytes);

    // C盘应该存在
    EXPECT_TRUE(result);
    EXPECT_GT(totalBytes, 0u);
    EXPECT_GT(freeBytes, 0u);
    EXPECT_LE(freeBytes, totalBytes);
}

TEST(Win32UtilTest, GetDiskSpaceInvalidDrive) {
    uint64_t totalBytes = 0;
    uint64_t freeBytes = 0;
    bool result = Win32Util::GetDiskSpace(L"Z:\\", totalBytes, freeBytes);

    // Z盘通常不存在
    EXPECT_FALSE(result);
}

TEST(Win32UtilTest, GetDiskSpaceEmptyPath) {
    uint64_t totalBytes = 0;
    uint64_t freeBytes = 0;
    // 空路径可能使用当前驱动器
    bool result = Win32Util::GetDiskSpace(L"", totalBytes, freeBytes);
    // 行为不确定，但不应崩溃
    SUCCEED();
}
