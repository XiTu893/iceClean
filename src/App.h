#pragma once
#include <wx/wx.h>
#include <cstdint>

namespace IceClean {

class App : public wxApp {
public:
    bool OnInit() override;
    int OnExit() override;
};

// 临时调试日志函数（用 ofstream 替代 spdlog，避免 GUI 应用中崩溃）
void DebugLog(const char* tag, const char* fmt, ...);

} // namespace IceClean
