cmake_minimum_required(VERSION 3.25)

project(IceCleanTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── GoogleTest via FetchContent ──
include(FetchContent)
set(GTEST_VERSION v1.17.0)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/${GTEST_VERSION}.zip
    # Prevent overriding the parent project's compiler/linker settings on Windows
    SET(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
)
FetchContent_MakeAvailable(googletest)

# ── MSVC settings ──
if(MSVC)
    add_compile_options(/utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()

# ── Test executable ──
# 单元测试：测试 core 层纯逻辑（不依赖 wxWidgets）
set(TEST_SOURCES
    # 测试主入口
    tests/main_test.cpp

    # ── Models 测试 ──
    tests/models/test_version_info.cpp

    # ── Core/Scanner 测试 ──
    tests/scanner/test_scan_result.cpp

    # ── Core/Cleaner 测试 ──
    tests/cleaner/test_privacy_cleaner.cpp
    tests/cleaner/test_registry_cleaner.cpp

    # ── Core/Safety 测试 ──
    tests/safety/test_rollback_manager.cpp
    tests/safety/test_operation_logger.cpp

    # ── Core/Optimizer 测试 ──
    tests/optimizer/test_startup_optimizer.cpp

    # ── Utils 测试 ──
    tests/utils/test_format_util.cpp
    tests/utils/test_win32_util.cpp
)

add_executable(IceCleanTests ${TEST_SOURCES})

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
