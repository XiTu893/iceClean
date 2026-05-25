# ── 收集源文件 (仅 .cpp 和 .rc，不包含 .h 头文件) ──
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.rc
)

# ── 创建可执行文件 (WIN32 子系统) ──
add_executable(IceClean WIN32 ${SOURCES})

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
)

# ── 链接 Windows 系统库 ──
target_link_libraries(IceClean PRIVATE ${SYSTEM_LIBS})

# ── wxWidgets 包含目录 ──
target_include_directories(IceClean PRIVATE ${wxWidgets_INCLUDE_DIRS})

# ── wxWidgets 定义 ──
target_compile_definitions(IceClean PRIVATE ${wxWidgets_DEFINITIONS})
target_compile_definitions(IceClean PRIVATE $<$<CONFIG:Debug>:${wxWidgets_DEFINITIONS_DEBUG}>)

# ── 复制 manifest 到输出目录 ──
add_custom_command(TARGET IceClean POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/gui/resources/app.manifest"
        "$<TARGET_FILE_DIR:IceClean>/app.manifest"
    COMMENT "Copying app.manifest to output directory"
)
