// spdlog 全局禁用 shim
// 在所有包含 <spdlog/spdlog.h> 之前定义此文件，阻止 spdlog 实际执行
// 解决 CRT vsprintf_s "Buffer too small" 在 GUI 环境下崩溃
#pragma once
#define SPDLOG_DISABLE_DEFAULT_LOGGER
#define SPDLOG_NO_EXCEPTIONS
#define SPDLOG_NO_THREAD_ID
#define SPDLOG_NO_WORKER_THREADS
#define SPDLOG_FUNCNAME
#define SPDLOG_TRACE(...)      ((void)0)
#define SPDLOG_DEBUG(...)     ((void)0)
#define SPDLOG_INFO(...)      ((void)0)
#define SPDLOG_WARN(...)      ((void)0)
#define SPDLOG_ERROR(...)     ((void)0)
#define SPDLOG_CRITICAL(...)  ((void)0)
#define SPDLOG_LOGGER_TRACE(...)  ((void)0)
#define SPDLOG_LOGGER_DEBUG(...)  ((void)0)
#define SPDLOG_LOGGER_INFO(...)   ((void)0)
#define SPDLOG_LOGGER_WARN(...)   ((void)0)
#define SPDLOG_LOGGER_ERROR(...)  ((void)0)
#define SPDLOG_LOGGER_CRITICAL(...) ((void)0)
