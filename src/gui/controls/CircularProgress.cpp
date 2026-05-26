#include "CircularProgress.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <wx/dcbuffer.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(CircularProgress, wxPanel)
    EVT_PAINT(CircularProgress::OnPaint)
    EVT_SIZE(CircularProgress::OnSize)
wxEND_EVENT_TABLE()

CircularProgress::CircularProgress(wxWindow* parent, wxWindowID id,
                                   const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(120, 120));
}

void CircularProgress::SetValue(int value) {
    m_value = std::clamp(value, 0, 100);
    Refresh();
}

void CircularProgress::SetProgressColor(const wxColour& color) {
    m_progressColor = color;
    Refresh();
}

void CircularProgress::SetLabel(const wxString& label) {
    m_label = label;
    Refresh();
}

void CircularProgress::SetSubLabel(const wxString& subLabel) {
    m_subLabel = subLabel;
    Refresh();
}

void CircularProgress::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    auto gc = std::unique_ptr<wxGraphicsContext>(wxGraphicsContext::Create(dc));
    if (!gc) return;

    const int w = GetSize().GetWidth();
    const int h = GetSize().GetHeight();
    const int side = wxMin(w, h);
    const int lineWidth = wxMax(side / 10, 6);
    const int margin = lineWidth / 2 + 2;
    const int diameter = side - 2 * margin;
    const double radius = static_cast<double>(diameter) / 2.0;
    const double cx = w / 2.0;
    const double cy = h / 2.0;

    // 绘制背景轨道
    gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(m_trackColor, lineWidth, wxPENSTYLE_SOLID)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath trackPath = gc->CreatePath();
    trackPath.AddCircle(cx, cy, radius);
    gc->StrokePath(trackPath);

    // 绘制进度弧
    if (m_value > 0) {
        double startAngle = -M_PI / 2.0; // 从顶部12点钟方向开始
        double sweepAngle = (m_value / 100.0) * 2.0 * M_PI;

        gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(m_progressColor, lineWidth, wxPENSTYLE_SOLID)
                                     .Cap(wxCAP_ROUND)));
        wxGraphicsPath progressPath = gc->CreatePath();
        progressPath.AddArc(cx, cy, radius, startAngle, startAngle + sweepAngle, false);
        gc->StrokePath(progressPath);
    }

    // 绘制中心文字
    if (!m_label.IsEmpty()) {
        wxFont labelFont(wxMax(side / 5, 12), wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                         false, L"微软雅黑");
        gc->SetFont(labelFont, *wxBLACK);
        double textW, textH;
        gc->GetTextExtent(m_label, &textW, &textH);

        double labelY = cy - textH / 2.0;
        if (!m_subLabel.IsEmpty()) {
            labelY -= textH * 0.15;
        }
        gc->DrawText(m_label, (w - textW) / 2.0, labelY);
    }

    if (!m_subLabel.IsEmpty()) {
        wxFont subFont(wxMax(side / 10, 8), wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                       false, L"微软雅黑");
        gc->SetFont(subFont, wxColour(0x66, 0x66, 0x66));
        double subW, subH;
        gc->GetTextExtent(m_subLabel, &subW, &subH);

        double mainH = 0;
        if (!m_label.IsEmpty()) {
            gc->GetTextExtent(m_label, nullptr, &mainH);
        }
        double subY = cy - mainH / 2.0 + mainH * 0.85;
        gc->DrawText(m_subLabel, (w - subW) / 2.0, subY);
    }
}

void CircularProgress::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

} // namespace IceClean::Gui
