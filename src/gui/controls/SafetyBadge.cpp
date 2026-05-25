#include "SafetyBadge.h"

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(SafetyBadge, wxPanel)
    EVT_PAINT(SafetyBadge::OnPaint)
wxEND_EVENT_TABLE()

SafetyBadge::SafetyBadge(wxWindow* parent, wxWindowID id,
                         const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(60, 22));
}

void SafetyBadge::SetSafetyRating(IceClean::Models::SafetyRating rating) {
    m_rating = rating;
    Refresh();
}

wxString SafetyBadge::GetRatingText(IceClean::Models::SafetyRating rating) {
    switch (rating) {
        case IceClean::Models::SafetyRating::Safe:      return L"安全";
        case IceClean::Models::SafetyRating::Caution:   return L"谨慎";
        case IceClean::Models::SafetyRating::Dangerous: return L"危险";
        default: return L"";
    }
}

wxColour SafetyBadge::GetRatingColor(IceClean::Models::SafetyRating rating) {
    switch (rating) {
        case IceClean::Models::SafetyRating::Safe:      return wxColour(0x10, 0x7C, 0x10); // 绿色
        case IceClean::Models::SafetyRating::Caution:   return wxColour(0xFF, 0x8C, 0x00); // 橙色
        case IceClean::Models::SafetyRating::Dangerous: return wxColour(0xE8, 0x11, 0x23); // 红色
        default: return *wxBLACK;
    }
}

void SafetyBadge::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();

    const int w = GetSize().GetWidth();
    const int h = GetSize().GetHeight();
    const int circleRadius = 5;
    const int circleY = h / 2;

    wxColour color = GetRatingColor(m_rating);
    wxString text = GetRatingText(m_rating);

    // 绘制彩色圆点
    dc.SetBrush(wxBrush(color));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawCircle(circleRadius + 2, circleY, circleRadius);

    // 绘制文字
    wxFont font(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑");
    dc.SetFont(font);
    dc.SetTextForeground(color);
    int textX = (circleRadius + 2) * 2 + 4;
    dc.DrawText(text, textX, (h - dc.GetCharHeight()) / 2);
}

} // namespace IceClean::Gui
