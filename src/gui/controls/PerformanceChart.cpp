#include "PerformanceChart.h"
#include "ThemeManager.h"
#include <wx/dcbuffer.h>
#include <algorithm>

namespace IceClean::Gui {

PerformanceChart::PerformanceChart(wxWindow* parent, wxWindowID id,
                                   const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(200, 80));
    Bind(wxEVT_PAINT, &PerformanceChart::OnPaint, this);
    Bind(wxEVT_SIZE, &PerformanceChart::OnSize, this);

    ThemeManager::Instance().RegisterChangeCallback([this](const ThemeColors& c) {
        Refresh();
    });
}

void PerformanceChart::SetData(const std::deque<double>& data) {
    m_data.clear();
    for (double v : data) {
        m_data.push_back(v);
    }
    while (m_data.size() > kMaxPoints) {
        m_data.pop_front();
    }
    Refresh();
}

void PerformanceChart::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    const auto& colors = ThemeManager::Instance().GetColors();
    const wxSize size = GetClientSize();

    dc.SetBackground(wxBrush(colors.surface));
    dc.Clear();

    if (size.GetWidth() < 10 || size.GetHeight() < 10) {
        return;
    }

    const int padding = 8;
    const int chartW = size.GetWidth() - padding * 2;
    const int chartH = size.GetHeight() - padding * 2;

    if (m_data.empty()) {
        dc.SetTextForeground(colors.textSecondary);
        dc.SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                          false, L"微软雅黑"));
        wxString text = L"等待数据...";
        wxSize textSize = dc.GetTextExtent(text);
        dc.DrawText(text, (size.GetWidth() - textSize.GetWidth()) / 2,
                    (size.GetHeight() - textSize.GetHeight()) / 2);
        return;
    }

    const double effectiveMax = m_maxValue > 0 ? m_maxValue : 100.0;

    std::vector<wxPoint> points;
    points.reserve(m_data.size());
    const int n = static_cast<int>(m_data.size());
    const double xStep = (n > 1) ? static_cast<double>(chartW) / (kMaxPoints - 1) : 0.0;
    const double startX = padding + chartW - (n - 1) * xStep;
    for (int i = 0; i < n; ++i) {
        double value = std::clamp(m_data[i], 0.0, effectiveMax);
        double ratio = value / effectiveMax;
        int x = static_cast<int>(startX + i * xStep);
        int y = padding + chartH - static_cast<int>(ratio * chartH);
        points.emplace_back(x, y);
    }

    if (points.size() >= 2) {
        dc.SetPen(wxPen(m_fillColor, 1));
        dc.SetBrush(wxBrush(m_fillColor));
        wxPointList fillPts;
        for (auto& p : points) fillPts.Append(new wxPoint(p));
        fillPts.Append(new wxPoint(padding + chartW, padding + chartH));
        fillPts.Append(new wxPoint(points.front().x, padding + chartH));
        dc.DrawPolygon(&fillPts);
        fillPts.clear();
    }

    if (points.size() >= 2) {
        dc.SetPen(wxPen(m_lineColor, 2));
        for (size_t i = 1; i < points.size(); ++i) {
            dc.DrawLine(points[i - 1], points[i]);
        }
    }

    double current = m_data.back();
    wxString valueText = wxString::Format(L"%.1f%s", current, m_unit);
    dc.SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                      false, L"微软雅黑"));
    dc.SetTextForeground(colors.textPrimary);
    wxSize textSize = dc.GetTextExtent(valueText);
    dc.DrawText(valueText, padding, padding);
}

void PerformanceChart::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

} // namespace IceClean::Gui
