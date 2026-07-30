// IceClean 单元测试 - 主入口
// 不需要定义 main()，由 GTest::gtest_main 提供

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

// 全局测试环境设置
class IceCleanTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // 初始化 spdlog 为测试模式
        spdlog::set_level(spdlog::level::debug);
        spdlog::info("IceClean 测试套件启动");
    }

    void TearDown() override {
        spdlog::info("IceClean 测试套件结束");
        spdlog::shutdown();
    }
};

// 注册全局环境
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new IceCleanTestEnvironment);
    return RUN_ALL_TESTS();
}
