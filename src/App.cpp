// 禁用所有 spdlog 调用（CRT vsprintf_s 在 GUI 应用中崩溃）
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_OFF
#define SPDLOG_TRACE(...)    ((void)0)
#define SPDLOG_DEBUG(...)    ((void)0)
#define SPDLOG_INFO(...)     ((void)0)
#define SPDLOG_WARN(...)     ((void)0)
#define SPDLOG_ERROR(...)    ((void)0)
#define SPDLOG_CRITICAL(...) ((void)0)
#define spdlog(...)          ((void)0)

#include "App.h"
#include "gui/MainWindow.h"
#include "gui/controls/ThemeManager.h"

#include <fstream>
#include <mutex>
#include <cstdarg>
#include <crtdbg.h>

// 自定义 CRT 断言处理器 - 完全静默（避免 CRT debug assert 在 GUI 环境下持续触发）
static int __cdecl CustomCrtReportHook(int reportType, char* message, int* returnValue) {
    (void)message;
    if (reportType == _CRT_ASSERT || reportType == _CRT_ERROR) {
        if (returnValue) *returnValue = 0;
        return 1;
    }
    return 0;
}

// 自定义 CRT 无效参数处理器 - 完全静默，防止 "Buffer too small" 导致进程终止
static void __cdecl CustomInvalidParameterHandler(
    const wchar_t* expression,
    const wchar_t* function,
    const wchar_t* file,
    unsigned int line,
    uintptr_t reserved)
{
    (void)expression; (void)function; (void)file; (void)line; (void)reserved;
    // 完全静默，不记录，让调用继续执行
}

// 自定义未处理异常过滤器 - 防止 TerminateProcess 等操作触发的级联崩溃
static LONG WINAPI CustomUnhandledExceptionFilter(EXCEPTION_POINTERS* ep) {
    (void)ep;
    return EXCEPTION_EXECUTE_HANDLER;
}

namespace IceClean {

bool App::OnInit()
{
    // spdlog 初始化已禁用 - 在 GUI 应用中与 CRT 格式化冲突导致崩溃
    // 改用 ofstream + DebugLog 函数
    DebugLog("App", "OnInit called");
    DebugLog("App", "IceClean starting");
    DebugLog("App", "spdlog disabled, using ofstream DebugLog");

    // 禁用 CRT debug assert 对话框（改为静默记录，防止 GUI 应用崩溃）
    // _CRTDBG_MODE_DEBUG = 1：输出到调试器（无窗口）
    // _CRTDBG_MODE_FILE = 2：输出到文件
    // _CRTDBG_MODE_WNDW = 4：弹出窗口（要禁用）
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    // 安装 CRT 断言处理器 - 记录到日志文件
    _CrtSetReportHook2(1, CustomCrtReportHook);
    _set_invalid_parameter_handler(CustomInvalidParameterHandler);
    SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);

    // 初始化所有图片处理器（JPEG/PNG/BMP/GIF等）
    wxInitAllImageHandlers();
    DebugLog("App", "wxInitAllImageHandlers done");

    // Set application name
    SetAppName("IceClean");
    SetVendorName("IceClean");

    // Create main window
    IceClean::Gui::ThemeManager::Instance().Initialize();

    // Create main window
    IceClean::Gui::ThemeManager::Instance().Initialize();
    auto* mainWindow = new Gui::MainWindow();
    mainWindow->SetSize(1100, 700);
    mainWindow->Center();
    mainWindow->SetMinSize(wxSize(900, 600));
    mainWindow->Show();

    return true;
}

int App::OnExit()
{
    return wxApp::OnExit();
}

// 临时调试日志函数
void DebugLog(const char* tag, const char* fmt, ...) {
    static std::mutex logMutex;
    std::lock_guard<std::mutex> lock(logMutex);
    std::ofstream log("IceClean.log", std::ios::app);
    if (!log.is_open()) return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    char timeBuf[64];
    sprintf_s(timeBuf, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] ",
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, tag);
    log << timeBuf;
    char msgBuf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msgBuf, sizeof(msgBuf), fmt, args);
    va_end(args);
    log << msgBuf << "\n";
}

} // namespace IceClean
