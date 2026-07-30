// StartupOptimizer 测试

#include <gtest/gtest.h>
#include "core/optimizer/StartupOptimizer.h"

using namespace IceClean::Core::Optimizer;

TEST(StartupOptimizerTest, DefaultConstruction) {
    StartupOptimizer optimizer;
    SUCCEED();
}

TEST(StartupOptimizerTest, GetStartupItems) {
    StartupOptimizer optimizer;
    // 获取启动项列表（可能为空，但不应崩溃）
    auto items = optimizer.GetStartupItems();
    // 启动项数量 >= 0
    EXPECT_GE(items.size(), 0u);
}
