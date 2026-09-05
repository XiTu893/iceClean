#pragma once
#include <wx/wx.h>
#include <vector>
#include <deque>

namespace IceClean::Gui {

class PerformanceChart : public wxPanel {
public:
    PerformanceChart(wxWindow* parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize);

    void SetData(const std::deque<double>& data);
    void SetLineColor(const wxColour& color) { m_lineColor = color; }
    void SetFillColor(const wxColour& color) { m_fillColor = color; }
    void SetMaxValue(double maxValue) { m_maxValue = maxValue; }
    void SetUnit(const wxString& unit) { m_unit = unit; }

private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);

    std::deque<double> m_data;
    static constexpr size_t kMaxPoints = 60;

    wxColour m_lineColor = wxColour(60, 130, 240);
    wxColour m_fillColor = wxColour(60, 130, 240, 80);
    double m_maxValue = 100.0;
    wxString m_unit = L"%";
};

} // namespace IceClean::Gui
