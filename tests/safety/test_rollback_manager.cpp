// RollbackManager 测试

#include <gtest/gtest.h>
#include "core/safety/RollbackManager.h"

using namespace IceClean::Core::Safety;
using namespace IceClean::Models;

TEST(RollbackManagerTest, BeginTransaction) {
    auto& mgr = RollbackManager::Instance();
    auto txId = mgr.BeginTransaction(L"测试事务");
    EXPECT_FALSE(txId.empty());
    EXPECT_GT(mgr.GetTotalTransactionCount(), 0);
}

TEST(RollbackManagerTest, AddStepToTransaction) {
    auto& mgr = RollbackManager::Instance();
    auto txId = mgr.BeginTransaction(L"添加步骤测试");

    mgr.AddStep(txId, RollbackStepType::Custom, L"测试步骤", L"{}");

    auto tx = mgr.GetTransaction(txId);
    ASSERT_TRUE(tx != nullptr);
    EXPECT_EQ(tx->steps.size(), 1u);
    EXPECT_EQ(tx->steps[0].type, RollbackStepType::Custom);
    EXPECT_FALSE(tx->steps[0].executed);
}

TEST(RollbackManagerTest, MarkStepExecuted) {
    auto& mgr = RollbackManager::Instance();
    auto txId = mgr.BeginTransaction(L"标记步骤测试");

    mgr.AddStep(txId, RollbackStepType::Custom, L"测试步骤", L"{}");
    mgr.MarkStepExecuted(txId, 0);

    auto tx = mgr.GetTransaction(txId);
    ASSERT_TRUE(tx != nullptr);
    EXPECT_TRUE(tx->steps[0].executed);
}

TEST(RollbackManagerTest, CommitTransaction) {
    auto& mgr = RollbackManager::Instance();
    auto txId = mgr.BeginTransaction(L"提交测试");

    mgr.AddStep(txId, RollbackStepType::Custom, L"测试步骤", L"{}");
    mgr.CommitTransaction(txId);

    auto tx = mgr.GetTransaction(txId);
    ASSERT_TRUE(tx != nullptr);
    EXPECT_TRUE(tx->committed);
}

TEST(RollbackManagerTest, GetRecentTransactions) {
    auto& mgr = RollbackManager::Instance();

    // 创建几个事务
    mgr.BeginTransaction(L"事务1");
    mgr.BeginTransaction(L"事务2");

    auto recent = mgr.GetRecentTransactions(2);
    EXPECT_GE(recent.size(), 2u);
}

TEST(RollbackManagerTest, CustomRollback) {
    auto& mgr = RollbackManager::Instance();
    auto txId = mgr.BeginTransaction(L"自定义回滚测试");

    bool rollbackCalled = false;
    mgr.RegisterCustomRollback(txId, L"自定义操作", [&rollbackCalled]() {
        rollbackCalled = true;
        return true;
    });

    mgr.MarkStepExecuted(txId, 0);

    // 注意：自定义回滚在实际回滚时调用
    // 此处验证注册成功
    auto tx = mgr.GetTransaction(txId);
    ASSERT_TRUE(tx != nullptr);
    EXPECT_EQ(tx->steps.size(), 1u);
    EXPECT_EQ(tx->steps[0].type, RollbackStepType::Custom);
}

TEST(RollbackManagerTest, RollableTransactionCount) {
    auto& mgr = RollbackManager::Instance();
    int count = mgr.GetRollableTransactionCount();
    EXPECT_GE(count, 0);
}

TEST(RollbackManagerTest, TransactionIdFormat) {
    auto& mgr = RollbackManager::Instance();
    auto txId = mgr.BeginTransaction(L"ID格式测试");
    EXPECT_TRUE(txId.find(L"TX_") == 0);
}
