# ── 收集源文件 (仅 .cpp 和 .rc，不包含 .h 头文件) ──
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.rc
)

# ── 禁用 spdlog（避免 GUI 环境下 CRT vsprintf_s "Buffer too small" 崩溃）──
# SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF 让 spdlog 内部所有日志宏变为 no-op
# (spdlog 头文件会根据这个宏在编译期消除日志代码)
add_compile_definitions(
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF
    SPDLOG_NO_EXCEPTIONS
)

# ── WebP 库 (wxWidgets 静态库中的 WebP 图像处理器需要) ──
# vcpkg 的 libwebp 提供 libwebp.lib / libwebpdemux.lib / libwebpmux.lib
if(VCPKG_INSTALLED_DIR AND VCPKG_TARGET_TRIPLET)
    set(WEBP_LIB_DIR "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib")
    find_library(WEBP_LIB_MAIN NAMES libwebp PATHS "${WEBP_LIB_DIR}" NO_DEFAULT_PATH)
    find_library(WEBP_LIB_DEMUX NAMES libwebpdemux PATHS "${WEBP_LIB_DIR}" NO_DEFAULT_PATH)
    find_library(WEBP_LIB_MUX NAMES libwebpmux PATHS "${WEBP_LIB_DIR}" NO_DEFAULT_PATH)
    find_library(WEBP_LIB_SHARPYUV NAMES sharpyuv PATHS "${WEBP_LIB_DIR}" NO_DEFAULT_PATH)
else()
    find_library(WEBP_LIB_MAIN NAMES libwebp)
    find_library(WEBP_LIB_DEMUX NAMES libwebpdemux)
    find_library(WEBP_LIB_MUX NAMES libwebpmux)
    find_library(WEBP_LIB_SHARPYUV NAMES sharpyuv)
endif()
if(NOT WEBP_LIB_MAIN)
    message(FATAL_ERROR "libwebp not found")
endif()

# ── 创建可执行文件 (WIN32 子系统) ──
add_executable(IceClean WIN32 ${SOURCES})

# ── CLI 烟雾测试 (CONSOLE 子系统，链接核心迁移逻辑，无 GUI) ──
set(MIGRATOR_SOURCES
    core/migrator/LargeFolderDetector.cpp
    core/migrator/WeChatMigrator.cpp
    core/migrator/QQMigrator.cpp
    core/migrator/SteamMigrator.cpp
    core/migrator/UserFolderMigrator.cpp
    core/migrator/DevCacheMigrator.cpp
    core/migrator/ProgramMigrator.cpp
    core/migrator/FolderMigrator.cpp
    core/migrator/MigratorBase.cpp
    utils/Win32Util.cpp
    utils/FileUtil.cpp
    utils/RegistryUtil.cpp
    utils/JunctionPoint.cpp
    utils/FormatUtil.cpp
)
add_executable(IceCleanCliTest CliTestMain.cpp ${MIGRATOR_SOURCES})
target_compile_definitions(IceCleanCliTest PRIVATE
    _CONSOLE
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
)
target_include_directories(IceCleanCliTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(IceCleanCliTest PRIVATE
    nlohmann_json::nlohmann_json
    unofficial::sqlite3::sqlite3
    spdlog::spdlog
    ${SYSTEM_LIBS}
)

# ── Windows DLL 导出符号 ──
set_target_properties(IceClean PROPERTIES
    CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON
)

# ── 包含目录 ──
target_include_directories(IceClean PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/resources
)

# ── 链接第三方库 ──
target_link_libraries(IceClean PRIVATE
    ${wxWidgets_LIBRARIES}
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    unofficial::sqlite3::sqlite3
    ${WEBP_LIB_SHARPYUV}
    ${WEBP_LIB_MAIN}
    ${WEBP_LIB_DEMUX}
    ${WEBP_LIB_MUX}
)

# ── 链接 Windows 系统库 ──
target_link_libraries(IceClean PRIVATE ${SYSTEM_LIBS})

# ── wxWidgets 包含目录 ──
target_include_directories(IceClean PRIVATE ${wxWidgets_INCLUDE_DIRS})

# ── wxWidgets 定义 ──
target_compile_definitions(IceClean PRIVATE ${wxWidgets_DEFINITIONS})
target_compile_definitions(IceClean PRIVATE $<$<CONFIG:Debug>:${wxWidgets_DEFINITIONS_DEBUG}>)

# ── 嵌入 manifest（UAC 管理员权限 / DPI 感知 / UTF-8 / Common Controls v6）──
# manifest 经 app.rc 的 RT_MANIFEST 资源嵌入可执行文件。这里关闭链接器自动生成
# manifest（其默认 asInvoker 会与 requireAdministrator 冲突，mt 合并报 c1010001）。
# 本工程使用静态 CRT，无需链接器注入运行库依赖。若不嵌入/不起作用，程序将以
# 非管理员身份运行，导致 HKLM 注册表清理等需要提权的操作失败。
if(MSVC)
    target_link_options(IceClean PRIVATE "/MANIFEST:NO")
endif()

# ── 复制运行时资源到输出目录 ──
# resources/ 下包含：app.ico、tray.ico、app.manifest、XiTu-logo.jpg、QrReward.jpg
# 以及托盘/应用图标的 PNG 预览文件
add_custom_command(TARGET IceClean POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/gui/resources
        $<TARGET_FILE_DIR:IceClean>/resources
    COMMENT "Copying resources/ to build output"
)
