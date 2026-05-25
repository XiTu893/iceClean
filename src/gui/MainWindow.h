#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>

namespace IceClean::Gui {

// Forward declarations for panels
class NavSidebar;
class DashboardPanel;
class ScanResultPanel;
class DeepCleanPanel;
class MigrationPanel;
class StartupPanel;
class DiskAnalyzerPanel;
class SettingsPanel;

class MainWindow : public wxFrame {
public:
    MainWindow();
    ~MainWindow() override;

    // Switch to a specific panel by index
    void SwitchPanel(int index);

    // Get the scan result panel
    ScanResultPanel* GetScanResultPanel() { return m_scanResultPanel; }

private:
    void CreateControls();
    void LayoutControls();

    // Event handlers
    void OnClose(wxCloseEvent& event);
    void OnSize(wxSizeEvent& event);

    NavSidebar* m_sidebar = nullptr;
    wxSimplebook* m_contentBook = nullptr;

    // Panels
    DashboardPanel* m_dashboardPanel = nullptr;
    ScanResultPanel* m_scanResultPanel = nullptr;
    DeepCleanPanel* m_deepCleanPanel = nullptr;
    MigrationPanel* m_migrationPanel = nullptr;
    StartupPanel* m_startupPanel = nullptr;
    DiskAnalyzerPanel* m_diskAnalyzerPanel = nullptr;
    SettingsPanel* m_settingsPanel = nullptr;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
