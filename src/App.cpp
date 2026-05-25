#include "App.h"
#include "gui/MainWindow.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sink.h>

namespace IceClean {

bool App::OnInit()
{
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
