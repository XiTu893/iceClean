#include "MainWindow.h"
#include "controls/NavSidebar.h"
#include "panels/DashboardPanel.h"
#include "panels/ScanResultPanel.h"
#include "panels/DeepCleanPanel.h"
#include "panels/MigrationPanel.h"
#include "panels/StartupPanel.h"
#include "panels/DiskAnalyzerPanel.h"
#include "panels/SettingsPanel.h"

#include <wx/dcbuffer.h>

namespace IceClean::Gui {

// ── Event table ──

wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
    EVT_CLOSE(MainWindow::OnClose)
    EVT_SIZE(MainWindow::OnSize)
wxEND_EVENT_TABLE()

// ── Constructor / Destructor ──

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY, L"IceClean - 智能C盘清理工具",
              wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE)
{
    SetBackgroundColour(*wxWHITE);
    CreateControls();
    LayoutControls();
}

MainWindow::~MainWindow() = default;

// ── Control creation ──

void MainWindow::CreateControls()
{
    // Create sidebar
    m_sidebar = new NavSidebar(this);

    // Create content area (simplebook for panel switching)
    m_contentBook = new wxSimplebook(this, wxID_ANY);
    m_contentBook->SetBackgroundColour(*wxWHITE);

    // Create all panels and add to simplebook
    m_dashboardPanel = new DashboardPanel(m_contentBook);
    m_scanResultPanel = new ScanResultPanel(m_contentBook);
    m_deepCleanPanel = new DeepCleanPanel(m_contentBook);
    m_migrationPanel = new MigrationPanel(m_contentBook);
    m_startupPanel = new StartupPanel(m_contentBook);
    m_diskAnalyzerPanel = new DiskAnalyzerPanel(m_contentBook);
    m_settingsPanel = new SettingsPanel(m_contentBook);

    m_contentBook->AddPage(m_dashboardPanel, L"首页");
    m_contentBook->AddPage(m_scanResultPanel, L"清理");
    m_contentBook->AddPage(m_deepCleanPanel, L"深度清理");
    m_contentBook->AddPage(m_migrationPanel, L"迁移");
    m_contentBook->AddPage(m_startupPanel, L"加速");
    m_contentBook->AddPage(m_diskAnalyzerPanel, L"分析");
    m_contentBook->AddPage(m_settingsPanel, L"设置");

    // Bind sidebar selection change event
    m_sidebar->Bind(wxEVT_NAV_SELECTION_CHANGED, [this](wxCommandEvent& event) {
        SwitchPanel(event.GetInt());
    });
}

void MainWindow::LayoutControls()
{
    auto* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Sidebar: fixed 200px width, full height
    mainSizer->Add(m_sidebar, 0, wxEXPAND);

    // Separator line between sidebar and content
    auto* separator = new wxPanel(this, wxID_ANY);
    separator->SetBackgroundColour(wxColour(230, 230, 230));
    separator->SetSize(1, -1);
    separator->SetMinSize(wxSize(1, 1));
    mainSizer->Add(separator, 0, wxEXPAND);

    // Content area: expand to fill remaining space
    mainSizer->Add(m_contentBook, 1, wxEXPAND);

    SetSizer(mainSizer);
}

// ── Panel switching ──

void MainWindow::SwitchPanel(int index)
{
    if (index >= 0 && index < m_contentBook->GetPageCount()) {
        m_contentBook->SetSelection(index);
    }
}

// ── Event handlers ──

void MainWindow::OnClose(wxCloseEvent& event)
{
    Destroy();
}

void MainWindow::OnSize(wxSizeEvent& event)
{
    event.Skip();
}

} // namespace IceClean::Gui
