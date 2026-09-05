// 临时 shim：完全禁用 spdlog（避免 vsprintf_s Buffer too small 崩溃）
// 在所有 #include <spdlog/spdlog.h> 之前包含此头
#pragma once
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#define SPDLOG_LOGGER_CATCH(loc, name)         ((void)0)
#define SPDLOG_DEBUG(...)                      ((void)0)
#define SPDLOG_INFO(...)                       ((void)0)
#define SPDLOG_WARN(...)                       ((void)0)
#define SPDLOG_ERROR(...)                      ((void)0)
#define SPDLOG_CRITICAL(...)                   ((void)0)
#define SPDLOG_TRACE(...)                      ((void)0)
#define spdlog(...)                            ((void)0)
