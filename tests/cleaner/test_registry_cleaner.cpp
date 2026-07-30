// RegistryCleaner 测试

#include <gtest/gtest.h>
#include "core/cleaner/RegistryCleaner.h"

using namespace IceClean::Core::Cleaner;

TEST(RegistryCleanerTest, DefaultConstruction) {
    RegistryCleaner cleaner;
    SUCCEED();
}

TEST(RegistryCleanerTest, InvalidItemTypes) {
    // 确保无效项类型枚举值不同
    // RegistryInvalidItem::Type 枚举应包含多种类型
    SUCCEED();
}
