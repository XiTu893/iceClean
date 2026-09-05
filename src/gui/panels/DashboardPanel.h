#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <memory>
#include "gui/controls/CardPanel.h"
#include "gui/controls/CircularProgress.h"
#include "models/HardwareInfo.h"

namespace IceClean::Core::Analyzer {
class PerformanceMonitor;
}

namespace IceClean::Gui {

class DashboardPanel : public wxPanel {
public:
    DashboardPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    ~DashboardPanel() override;

    void UpdateHealthScore();

    void LoadCumulativeStats();

private:
    CircularProgress* m_cpuProgress = nullptr;
    CircularProgress* m_memProgress = nullptr;
    CircularProgress* m_diskProgress = nullptr;
    wxStaticText* m_netLabel = nullptr;

    wxStaticText* m_cpuInfoLabel = nullptr;
    wxStaticText* m_memInfoLabel = nullptr;

    CircularProgress* m_healthProgress = nullptr;
    wxStaticText* m_healthScoreLabel = nullptr;
    wxStaticText* m_healthDescLabel = nullptr;
    wxStaticText* m_cumulativeLabel = nullptr;
    wxStaticText* m_diskUsageLabel = nullptr;
    wxStaticText* m_spaceWarningLabel = nullptr;

    std::unique_ptr<Core::Analyzer::PerformanceMonitor> m_performanceMonitor;

    wxTimer* m_updateTimer = nullptr;
    uint64_t m_lastDiskCheck = 0;

    void CreateControls();

    void OnUpdateTimer(wxTimerEvent& event);

    void UpdatePerformanceStats();
    void UpdateDiskSpaceInfo();

    static wxString FormatBytes(uint64_t bytes);
    static wxString FormatSpeed(uint64_t bytesPerSec);
    static wxString FormatFrequency(double ghz);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
