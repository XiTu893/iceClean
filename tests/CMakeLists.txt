cmake_minimum_required(VERSION 3.25)

project(IceCleanTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── GoogleTest via vcpkg ──
# 使用 vcpkg 已安装的 gtest（避免 FetchContent 网络下载在离线/代理环境下失败）
find_package(GTest CONFIG REQUIRED)
if(MSVC)
    add_compile_options(/utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()

# ── Test executable ──
# 单元测试：测试 core 层纯逻辑（不依赖 wxWidgets）
set(TEST_SOURCES
    # 测试主入口
    main_test.cpp

    # ── Models 测试 ──
    models/test_version_info.cpp

    # ── Core/Scanner 测试 ──
    scanner/test_scan_result.cpp

    # ── Core/Cleaner 测试 ──
    cleaner/test_privacy_cleaner.cpp
    cleaner/test_registry_cleaner.cpp

    # ── Core/Safety 测试 ──
    safety/test_rollback_manager.cpp
    safety/test_operation_logger.cpp

    # ── Core/Optimizer 测试 ──
    optimizer/test_startup_optimizer.cpp

    # ── Utils 测试 ──
    utils/test_format_util.cpp
utils/test_win32_util.cpp

    # 被测 core 实现（启动优化链路）
    ../src/core/optimizer/StartupOptimizer.cpp
    ../src/core/optimizer/ScheduledTaskOptimizer.cpp
    ../src/utils/RegistryUtil.cpp
    ../src/utils/Win32Util.cpp
    ../src/utils/FileUtil.cpp
../src/core/cleaner/CleanerBase.cpp
    ../src/core/cleaner/PrivacyCleaner.cpp
    ../src/core/cleaner/RegistryCleaner.cpp
    ../src/core/safety/RollbackManager.cpp
    ../src/core/safety/OperationLogger.cpp
    ../src/core/safety/SystemBackupManager.cpp
    ../src/core/safety/WhitelistProvider.cpp
    ../src/core/safety/OperationLogger.cpp
    ../src/core/safety/SystemBackupManager.cpp
    ../src/utils/JsonUtil.cpp
    ../src/utils/FormatUtil.cpp
)

add_executable(IceCleanTests ${TEST_SOURCES})

target_compile_definitions(IceCleanTests PRIVATE UNICODE _UNICODE)

# ── 包含目录（访问 src/ 下的头文件）──
target_include_directories(IceCleanTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/gui/resources
)

# ── 链接库 ──
target_link_libraries(IceCleanTests PRIVATE
    GTest::gtest
    GTest::gmock
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    advapi32
    version
    shell32
    shlwapi
    ${SYSTEM_LIBS}
)

# ── Coverage option ──
option(ENABLE_COVERAGE "Enable coverage flags" OFF)
if(ENABLE_COVERAGE)
    if(MSVC)
        # MSVC coverage via OpenCPPCoverage (external tool)
        message(STATUS "Coverage enabled - use OpenCPPCoverage for MSVC")
    endif()
endif()

# ── Sanitizers ──
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(ENABLE_ASAN AND NOT MSVC)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN AND NOT MSVC)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif()

# ── CTest integration ──
enable_testing()
include(GoogleTest)
gtest_discover_tests(IceCleanTests)
