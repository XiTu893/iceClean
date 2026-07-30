#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include "core/analyzer/HardwareDetector.h"

namespace IceClean::Gui {

class HardwareInfoPanel : public wxPanel {
public:
    HardwareInfoPanel(wxWindow* parent);

private:
    void LoadHardwareInfo();
    wxStaticText* AddInfoRow(wxSizer* sizer, const wxString& label, wxStaticText*& valueLabel);

    wxStaticText* m_cpuInfo = nullptr;
    wxStaticText* m_gpuInfo = nullptr;
    wxStaticText* m_memoryInfo = nullptr;
    wxStaticText* m_diskInfo = nullptr;
    wxStaticText* m_motherboardInfo = nullptr;
    wxStaticText* m_osInfo = nullptr;
    wxStaticText* m_uptimeInfo = nullptr;

    Core::Analyzer::HardwareDetector m_detector;
};

} // namespace IceClean::Gui
