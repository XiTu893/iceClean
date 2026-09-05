#include "PauseOverlay.h"
#include "gui/controls/ThemeManager.h"
#include <wx/dcbuffer.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(PauseOverlay, wxPanel)
    EVT_PAINT(PauseOverlay::OnPaint)
    EVT_ERASE_BACKGROUND(PauseOverlay::OnEraseBackground)
wxEND_EVENT_TABLE()

PauseOverlay::PauseOverlay(wxWindow* parent, wxWindowID id,
                           const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size, wxBORDER_NONE)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    m_overlayColor = wxColour(0, 0, 0, 180); // 半透明黑
    m_textColor = *wxWHITE;
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Hide();
}

void PauseOverlay::ShowOverlay(bool show) {
    m_visible = show;
    if (show) {
        Raise();
        Show();
    } else {
        Hide();
    }
    Refresh();
}

void PauseOverlay::SetStatusText(const wxString& text) {
    m_statusText = text;
    Refresh();
}

void PauseOverlay::OnPaint(wxPaintEvent& event) {
    if (!m_visible) return;
    
    wxAutoBufferedPaintDC dc(this);
    wxSize sz = GetClientSize();
    int w = sz.GetWidth();
    int h = sz.GetHeight();

    // 绘制半透明遮罩
    wxBrush overlayBrush(m_overlayColor);
    dc.SetBrush(overlayBrush);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, w, h);

    // 绘制中心内容
    wxString displayText = m_statusText.empty() ? wxString(L"暂停中...") : m_statusText;
    dc.SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    dc.SetTextForeground(m_textColor);
    wxSize textSize = dc.GetTextExtent(displayText);
    int x = (w - textSize.GetWidth()) / 2;
    int y = (h - textSize.GetHeight()) / 2 - 30;
    dc.DrawText(displayText, x, y);

    // 绘制旋转的加载动画圆圈（简单的静态圆圈）
    int cx = w / 2;
    int cy = (h - textSize.GetHeight()) / 2 + 10;
    int radius = 20;
    wxPen pen(m_textColor, 3);
    dc.SetPen(pen);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawCircle(cx, cy, radius);
}

void PauseOverlay::OnEraseBackground(wxEraseEvent& event) {
    // 防止闪烁
}

} // namespace IceClean::Gui