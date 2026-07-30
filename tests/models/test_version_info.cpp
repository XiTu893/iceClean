// VersionInfo 模型测试

#include <gtest/gtest.h>
#include "models/UpdateInfo.h"

using namespace IceClean::Models;

TEST(VersionInfoTest, CompareEqualVersions) {
    VersionInfo v1{1, 0, 0, 0};
    VersionInfo v2{1, 0, 0, 0};
    EXPECT_EQ(v1.Compare(v2), 0);
    EXPECT_FALSE(v1.IsNewerThan(v2));
}

TEST(VersionInfoTest, CompareMajorDifference) {
    VersionInfo v1{2, 0, 0, 0};
    VersionInfo v2{1, 9, 9, 9};
    EXPECT_GT(v1.Compare(v2), 0);
    EXPECT_TRUE(v1.IsNewerThan(v2));
    EXPECT_FALSE(v2.IsNewerThan(v1));
}

TEST(VersionInfoTest, CompareMinorDifference) {
    VersionInfo v1{1, 2, 0, 0};
    VersionInfo v2{1, 1, 9, 9};
    EXPECT_GT(v1.Compare(v2), 0);
    EXPECT_TRUE(v1.IsNewerThan(v2));
}

TEST(VersionInfoTest, ComparePatchDifference) {
    VersionInfo v1{1, 0, 3, 0};
    VersionInfo v2{1, 0, 2, 0};
    EXPECT_GT(v1.Compare(v2), 0);
}

TEST(VersionInfoTest, CompareBuildDifference) {
    VersionInfo v1{1, 0, 0, 100};
    VersionInfo v2{1, 0, 0, 99};
    EXPECT_GT(v1.Compare(v2), 0);
}

TEST(VersionInfoTest, CompareOlderVersion) {
    VersionInfo v1{1, 0, 0, 0};
    VersionInfo v2{2, 0, 0, 0};
    EXPECT_LT(v1.Compare(v2), 0);
    EXPECT_TRUE(v2.IsNewerThan(v1));
}

TEST(AutoUpdateSettingsTest, DefaultValues) {
    AutoUpdateSettings settings;
    EXPECT_TRUE(settings.autoCheckEnabled);
    EXPECT_EQ(settings.checkIntervalHours, 24);
    EXPECT_FALSE(settings.autoDownloadEnabled);
    EXPECT_TRUE(settings.notifyOnUpdate);
}

TEST(UpdateCheckResultTest, DefaultNoUpdate) {
    UpdateCheckResult result;
    EXPECT_FALSE(result.hasUpdate);
    EXPECT_FALSE(result.networkError);
    EXPECT_TRUE(result.errorMessage.empty());
}
