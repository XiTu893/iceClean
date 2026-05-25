cmake_minimum_required(VERSION 3.25)

project(IceClean VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── vcpkg 提供的 find_package 支持 ──
find_package(wxWidgets REQUIRED COMPONENTS core base)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# ── Windows 系统库 ──
set(SYSTEM_LIBS
    SrClient
    Psapi
    Shlwapi
    Ole32
    OleAut32
    Taskschd
    Version
)

# ── MSVC 专用设置 ──
if(MSVC)
    # UTF-8 源码与执行字符集
    add_compile_options(/utf-8)

    # 静态运行时 (/MT Release, /MTd Debug)
    set_property(DIRECTORY PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    # 禁用 MSVC 安全警告
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()

# ── 子目录 ──
add_subdirectory(src)
