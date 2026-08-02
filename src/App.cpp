#include "App.h"
#include "gui/MainWindow.h"
#include "gui/controls/ThemeManager.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <fstream>
#include <crtdbg.h>

// 获取断言日志文件路径（放在 exe 同目录）
static std::wstring GetAssertLogPath() {
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path(exePath);
    auto pos = path.find_last_of(L'\\');
    if (pos != std::wstring::npos) {
        path = path.substr(0, pos + 1) + L"assert_failures.log";
    } else {
        path = L"assert_failures.log";
    }
    return path;
}

// 自定义 wxWidgets 断言处理器 - 将断言信息写入日志文件而非弹窗
static void CustomAssertHandler(const wxString& file,
                                 int line,
                                 const wxString& func,
                                 const wxString& cond,
                                 const wxString& msg)
{
    // 写入日志文件（使用窄字符串避免缓冲区问题）
    std::ofstream assertLog(GetAssertLogPath(), std::ios::app);
    if (assertLog.is_open()) {
        assertLog << "[wxASSERT] " << file.ToUTF8().data()
                  << "(" << line << "): assert \"" << cond.ToUTF8().data()
                  << "\" failed in " << func.ToUTF8().data();
        if (!msg.IsEmpty()) {
            assertLog << ": " << msg.ToUTF8().data();
        }
        assertLog << std::endl;
    }
}

// 自定义 CRT 断言处理器 - 将断言信息写入日志文件而非弹窗
static int __cdecl CustomCrtReportHook(int reportType, char* message, int* returnValue) {
    const char* typeStr = "UNKNOWN";
    switch (reportType) {
        case _CRT_ASSERT:  typeStr = "ASSERT"; break;
        case _CRT_ERROR:   typeStr = "ERROR"; break;
        case _CRT_WARN:    typeStr = "WARNING"; break;
    }

    // 写入日志文件
    std::ofstream assertLog(GetAssertLogPath(), std::ios::app);
    if (assertLog.is_open()) {
        assertLog << "[CRT_" << typeStr << "] " << (message ? message : "(null)") << std::endl;
    }

    // 对于 ASSERT 和 ERROR，返回 1 表示不显示弹窗（不中断）
    if (reportType == _CRT_ASSERT || reportType == _CRT_ERROR) {
        if (returnValue) *returnValue = 0;
        return 1;  // 不显示弹窗
    }
    return 0;  // 继续默认处理 WARNING
}

// 自定义 CRT 无效参数处理器 - 防止 "Buffer too small" 等断言导致进程终止
// CRT 内部的 sprintf/snprintf 缓冲区溢出会触发 _invalid_parameter_handler，
// 默认处理器会调用 _invoke_watson 导致进程崩溃，自定义处理器仅记录日志
static void __cdecl CustomInvalidParameterHandler(
    const wchar_t* expression,
    const wchar_t* function,
    const wchar_t* file,
    unsigned int line,
    uintptr_t reserved)
{
    // 写入日志文件
    std::ofstream assertLog(GetAssertLogPath(), std::ios::app);
    if (assertLog.is_open()) {
        assertLog << "[CRT_INVALID_PARAM] ";
        if (expression) assertLog << "expr=" << std::string(expression, expression + wcslen(expression)) << " ";
        if (function) assertLog << "func=" << std::string(function, function + wcslen(function)) << " ";
        if (file) assertLog << "file=" << std::string(file, file + wcslen(file)) << ":" << line;
        assertLog << std::endl;
    }
    // 不调用默认处理器，防止进程终止
}

// 自定义未处理异常过滤器 - 防止 TerminateProcess 等操作触发的级联崩溃
static LONG WINAPI CustomUnhandledExceptionFilter(EXCEPTION_POINTERS* ep) {
    // 记录异常信息
    std::ofstream assertLog(GetAssertLogPath(), std::ios::app);
    if (assertLog.is_open()) {
        assertLog << "[UNHANDLED_EXCEPTION] Code=0x"
                  << std::hex << (ep ? ep->ExceptionRecord->ExceptionCode : 0)
                  << " Addr=0x"
                  << (ep ? reinterpret_cast<uintptr_t>(ep->ExceptionRecord->ExceptionAddress) : 0)
                  << std::dec << std::endl;
    }
    // 返回 EXCEPTION_EXECUTE_HANDLER 让进程正常终止而非弹出 WER 对话框
    return EXCEPTION_EXECUTE_HANDLER;
}

namespace IceClean {

bool App::OnInit()
{
    // 设置自定义 CRT 无效参数处理器（在所有初始化之前，防止 Buffer too small 崩溃）
    _set_invalid_parameter_handler(CustomInvalidParameterHandler);

    // 设置自定义未处理异常过滤器（防止级联崩溃导致进程意外终止）
    SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);

    // 设置自定义 CRT 断言处理器（在 wxWidgets 初始化之前）
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, CustomCrtReportHook);
    // 同时禁用 CRT 断言弹窗模式
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

    // 设置自定义 wxWidgets 断言处理器（将断言写入日志文件而非弹窗）
    wxSetAssertHandler(CustomAssertHandler);

    // 初始化所有图片处理器（JPEG/PNG/BMP/GIF等）
    wxInitAllImageHandlers();

    // Set application name
    SetAppName("IceClean");
    SetVendorName("IceClean");

    // Initialize spdlog logger
    try {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("IceClean.log", true);

        std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };
        auto logger = std::make_shared<spdlog::logger>("default", sinks.begin(), sinks.end());
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        logger->set_level(spdlog::level::debug);
        spdlog::set_default_logger(logger);

        spdlog::info("IceClean starting...");
    }
    catch (const spdlog::spdlog_ex& ex) {
        // If logger init fails, continue without logging
    }

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
    spdlog::info("IceClean exiting...");
    spdlog::shutdown();
    return wxApp::OnExit();
}

} // namespace IceClean
