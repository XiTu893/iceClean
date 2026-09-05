// StartupOptimizer 单元测试
// 覆盖：StartupApproved 标志读写、非破坏式禁用/启用往返、幂等性、旧机制兼容
// 全部操作限定在 HKCU\Software\IceClean\Tests 沙箱键内，不触碰真实启动配置

#include <gtest/gtest.h>
#include <windows.h>
#include "core/optimizer/StartupOptimizer.h"
#include "utils/RegistryUtil.h"

using namespace IceClean::Core::Optimizer;
using IceClean::Utils::RegistryUtil;

namespace {

constexpr wchar_t kSandboxRoot[] = L"Software\\IceClean\\Tests";
const std::wstring kRunKey = std::wstring(kSandboxRoot) + L"\\Run";
const std::wstring kApprovedKey = std::wstring(kSandboxRoot) + L"\\Approved";

// 模拟 Electron 应用（如 LM Studio）写入 HKCU Run 的典型条目
constexpr wchar_t kElectronName[] = L"electron.app.LM Studio";
constexpr wchar_t kElectronCmd[] =
    L"\"C:\\Users\\tester\\AppData\\Local\\Programs\\LM Studio\\LM Studio.exe\"";

void WriteRunValue(const std::wstring& name, const std::wstring& cmd) {
    ASSERT_TRUE(RegistryUtil::WriteStringValue(HKEY_CURRENT_USER, kRunKey, name, cmd));
}

} // namespace

class StartupApprovedTest : public ::testing::Test {
protected:
    void SetUp() override {
        RegDeleteTreeW(HKEY_CURRENT_USER, kSandboxRoot);
    }
    void TearDown() override {
        RegDeleteTreeW(HKEY_CURRENT_USER, kSandboxRoot);
        // 清理可能触达的真实旧版备份键中的测试值
        RegistryUtil::DeleteValue(HKEY_CURRENT_USER,
                                  L"Software\\IceClean\\DisabledStartup", kElectronName);
    }
};

// ── StartupApproved 标志语义 ──

TEST_F(StartupApprovedTest, AbsentFlagMeansEnabled) {
    EXPECT_FALSE(StartupOptimizer::IsStartupApprovedDisabled(
        HKEY_CURRENT_USER, kApprovedKey, L"Not.Exist"));
}

TEST_F(StartupApprovedTest, DisableThenEnableRoundTrip) {
    const std::wstring name = kElectronName;

    EXPECT_FALSE(StartupOptimizer::IsStartupApprovedDisabled(
        HKEY_CURRENT_USER, kApprovedKey, name));

    EXPECT_TRUE(StartupOptimizer::SetStartupApproved(
        HKEY_CURRENT_USER, kApprovedKey, name, false));
    EXPECT_TRUE(StartupOptimizer::IsStartupApprovedDisabled(
        HKEY_CURRENT_USER, kApprovedKey, name));

    EXPECT_TRUE(StartupOptimizer::SetStartupApproved(
        HKEY_CURRENT_USER, kApprovedKey, name, true));
    EXPECT_FALSE(StartupOptimizer::IsStartupApprovedDisabled(
        HKEY_CURRENT_USER, kApprovedKey, name));
}

TEST_F(StartupApprovedTest, EnableWhenAlreadyEnabledSucceeds) {
    // 标志本就不存在，恢复启用应为幂等成功而非失败
    EXPECT_TRUE(StartupOptimizer::SetStartupApproved(
        HKEY_CURRENT_USER, kApprovedKey, kElectronName, true));
}

// ── 非破坏式禁用（核心回归用例：LM Studio 场景）──

TEST_F(StartupApprovedTest, DisableIsNonDestructiveAndAppProof) {
    const std::wstring name = kElectronName;
    const std::wstring cmd = kElectronCmd;
    WriteRunValue(name, cmd);

    StartupOptimizer opt;
    ASSERT_TRUE(opt.DisableRegistryItemAt(HKEY_CURRENT_USER, kRunKey,
                                          kApprovedKey, name));

    // 关键断言：Run 值必须原样保留（应用重写也无效，Explorer 依据标志跳过）
    EXPECT_EQ(RegistryUtil::ReadStringValue(HKEY_CURRENT_USER, kRunKey, name), cmd);
    EXPECT_TRUE(StartupOptimizer::IsStartupApprovedDisabled(
        HKEY_CURRENT_USER, kApprovedKey, name));

    // 幂等：重复禁用返回成功
    EXPECT_TRUE(opt.DisableRegistryItemAt(HKEY_CURRENT_USER, kRunKey,
                                          kApprovedKey, name));

    // 恢复：清除标志后值仍在、状态为启用
    ASSERT_TRUE(opt.EnableRegistryItemAt(HKEY_CURRENT_USER, kRunKey,
                                         kApprovedKey, name));
    EXPECT_FALSE(StartupOptimizer::IsStartupApprovedDisabled(
        HKEY_CURRENT_USER, kApprovedKey, name));
    EXPECT_EQ(RegistryUtil::ReadStringValue(HKEY_CURRENT_USER, kRunKey, name), cmd);
}

TEST_F(StartupApprovedTest, DisableMissingValueFails) {
    StartupOptimizer opt;
    // Run 值不存在且无旧版备份 → 明确失败，不得谎报成功
    EXPECT_FALSE(opt.DisableRegistryItemAt(HKEY_CURRENT_USER, kRunKey,
                                           kApprovedKey, L"Ghost.App"));
}

TEST_F(StartupApprovedTest, LegacyBackupRecognizedAsDisabled) {
    // 兼容旧机制：值已移入旧备份键时，再次禁用应视为已生效
    ASSERT_TRUE(RegistryUtil::WriteStringValue(HKEY_CURRENT_USER,
                                               L"Software\\IceClean\\DisabledStartup",
                                               kElectronName, kElectronCmd));

    StartupOptimizer opt;
    EXPECT_TRUE(opt.DisableRegistryItemAt(HKEY_CURRENT_USER, kRunKey,
                                          kApprovedKey, kElectronName));
}
