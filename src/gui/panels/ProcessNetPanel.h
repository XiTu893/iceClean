#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <memory>
#include <vector>
#include <algorithm>
#include "core/analyzer/ProcessNetworkMonitor.h"

namespace IceClean::Gui {

class ProcessNetPanel : public wxPanel {
public:
    ProcessNetPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    ~ProcessNetPanel();

    void StartMonitoring();
    void StopMonitoring();

private:
    void OnSnapshot(const std::vector<IceClean::Core::Analyzer::ProcessNetworkStats>& stats);
    void OnColumnClick(wxListEvent& event);
    void SortAndPopulate();

    enum SortColumn { ColProcess = 0, ColPid, ColDownSpeed, ColUpSpeed, ColTotalDown, ColTotalUp };

    static wxString FormatBytes(uint64_t bytes);
    static wxString FormatSpeed(uint64_t bytesPerSec);

    wxListCtrl* m_listCtrl = nullptr;
    std::unique_ptr<IceClean::Core::Analyzer::ProcessNetworkMonitor> m_monitor;
    std::vector<IceClean::Core::Analyzer::ProcessNetworkStats> m_data;
    SortColumn m_sortCol = ColDownSpeed;
    bool m_sortAsc = false;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
