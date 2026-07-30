// OperationLogger 测试

#include <gtest/gtest.h>
#include "core/safety/OperationLogger.h"
#include "models/OperationRecord.h"

using namespace IceClean::Core::Safety;
using namespace IceClean::Models;

TEST(OperationLoggerTest, LogOperation) {
    OperationRecord record;
    record.type = OperationType::Clean;
    record.description = L"测试操作";
    record.size = 1024;
    record.timestamp = std::chrono::system_clock::now();
    record.success = true;

    // 记录操作不应抛异常
    EXPECT_NO_THROW(OperationLogger::LogOperation(record));
}

TEST(OperationLoggerTest, GetRecentOperations) {
    // 先记录一条操作
    OperationRecord record;
    record.type = OperationType::Clean;
    record.description = L"获取操作测试";
    record.size = 2048;
    record.timestamp = std::chrono::system_clock::now();
    record.success = true;
    OperationLogger::LogOperation(record);

    // 获取最近操作
    auto operations = OperationLogger::GetRecentOperations(10);
    EXPECT_GE(operations.size(), 1u);
}

TEST(OperationRecordTest, DefaultValues) {
    OperationRecord record;
    EXPECT_EQ(record.size, 0);
    EXPECT_FALSE(record.success);
    EXPECT_TRUE(record.description.empty());
    EXPECT_TRUE(record.details.empty());
}

TEST(OperationTypeTest, EnumValues) {
    EXPECT_NE(OperationType::Clean, OperationType::Migrate);
    EXPECT_NE(OperationType::Optimize, OperationType::Restore);
}
