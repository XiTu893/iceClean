cmake_minimum_required(VERSION 3.25)

project(IceClean VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Windows DLL 导出符号 ──
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# ── MSVC 运行时库设置 (需在 project 之后) ──
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ── vcpkg 提供的 find_package 支持 ──
find_package(wxWidgets REQUIRED COMPONENTS core base adv aui html xml net richtext)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# ── Windows 系统库 ──
set(SYSTEM_LIBS
    SrClient
    Psapi
    Shlwapi
    Ole32
    OleAut32
    Taskschd
    Version
    Iphlpapi
    Ws2_32
    Crypt32
    Advapi32
    Shell32
    Winhttp
    SetupApi        # 设备/驱动枚举（SetupDi）
    CfgMgr32        # CM_Get_DevNode_Status 等设备状态
)

# ── MSVC 专用设置 ──
if(MSVC)
    # UTF-8 源码与执行字符集
    add_compile_options(/utf-8)

    # 定义 _USE_MATH_DEFINES 以启用 M_PI 等数学常量
    add_definitions(-D_USE_MATH_DEFINES)

    # 禁用 MSVC 安全警告
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()

# ── 子目录 ──
add_subdirectory(src)

# ── 核心功能命令行验证工具（无 GUI 依赖，EXCLUDE_FROM_ALL 不进主构建）──
add_executable(startup_opt_cli EXCLUDE_FROM_ALL
    tools/startup_optimizer_cli.cpp
    src/core/optimizer/StartupOptimizer.cpp
    src/core/optimizer/ScheduledTaskOptimizer.cpp
    src/utils/RegistryUtil.cpp
    src/utils/Win32Util.cpp
    src/utils/FileUtil.cpp
)
target_include_directories(startup_opt_cli PRIVATE ${CMAKE_SOURCE_DIR}/src)
# 与主程序保持一致的字符集环境（Win32Util 等源码按 UNICODE 语义编写）
target_compile_definitions(startup_opt_cli PRIVATE UNICODE _UNICODE)

add_executable(core_check_cli EXCLUDE_FROM_ALL
    tools/core_check_cli.cpp
    src/core/migrator/LargeFolderDetector.cpp
    src/core/migrator/FolderMigrator.cpp
    src/core/migrator/MigratorBase.cpp
    src/core/optimizer/ServiceOptimizer.cpp
    src/core/optimizer/StartupOptimizer.cpp
    src/core/optimizer/ScheduledTaskOptimizer.cpp
    src/core/safety/SoftwareRecommendFetcher.cpp
    src/core/safety/SoftwareRecommendSeed.cpp
    src/core/safety/RestorePointManager.cpp
    src/core/safety/OperationLogger.cpp
    src/core/driver/DeviceDriverScanner.cpp
    src/core/driver/PnpUtilRunner.cpp
    src/core/driver/DriverBackupManager.cpp
    src/utils/JunctionPoint.cpp
    src/utils/RegistryUtil.cpp
    src/utils/Win32Util.cpp
    src/utils/FileUtil.cpp
    src/utils/FormatUtil.cpp
    src/utils/JsonUtil.cpp
)
target_include_directories(core_check_cli PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_definitions(core_check_cli PRIVATE UNICODE _UNICODE)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(core_check_cli PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    SetupApi
    CfgMgr32
)

# ── 测试（可选）──
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
