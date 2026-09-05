#include "ScanProgressBar.h"
#include "gui/controls/ThemeManager.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <cmath>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(ScanProgressBar, wxPanel)
    EVT_PAINT(ScanProgressBar::OnPaint)
    EVT_SIZE(ScanProgressBar::OnSize)
wxEND_EVENT_TABLE()

ScanProgressBar::ScanProgressBar(wxWindow* parent, wxWindowID id,
                                 const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size, wxBORDER_NONE)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    m_progressColor = colors.accent;
    m_backColor = colors.background;
    m_trackColor = colors.border;
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(160, 14));
    SetMaxSize(wxSize(-1, 14));

    // 平滑动画：显示值向目标值插值，消除 33/66/90 跳变的生硬感
    m_animTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &ScanProgressBar::OnAnimTimer, this, m_animTimer->GetId());
}

ScanProgressBar::~ScanProgressBar() {
    if (m_animTimer) {
        m_animTimer->Stop();
        delete m_animTimer;
        m_animTimer = nullptr;
    }
}

void ScanProgressBar::SetValue(double value) {
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    m_value = value;
    EnsureAnimRunning();
}

void ScanProgressBar::EnsureAnimRunning() {
    if (m_animTimer && !m_animTimer->IsRunning()) {
        m_animTimer->Start(16); // ~60fps
    }
}

void ScanProgressBar::EnsureAnimStopped() {
    if (m_animTimer && m_animTimer->IsRunning()) {
        m_animTimer->Stop();
    }
}

void ScanProgressBar::OnAnimTimer(wxTimerEvent& event) {
    const double diff = m_value - m_displayValue;
    if (std::abs(diff) < 0.5) {
        m_displayValue = m_value;
        EnsureAnimStopped();
    } else {
        // 指数趋近 + 最低步长，兼顾顺滑与到达速度
        m_displayValue += diff * 0.18 + (diff > 0 ? 0.4 : -0.4);
    }
    Refresh();
}

void ScanProgressBar::SetStatusText(const wxString& text) {
    m_statusText = text;
    Refresh();
}

void ScanProgressBar::SetColor(const wxColour& color) {
    m_progressColor = color;
    Refresh();
}

void ScanProgressBar::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) return;

    wxSize sz = GetClientSize();
    int w = sz.GetWidth();
    int h = sz.GetHeight();
    int r = std::min(6, h / 2); // corner radius

    // Draw track
    wxGraphicsPath trackPath = gc->CreatePath();
    trackPath.AddRoundedRectangle(0, 0, w, h, r);
    gc->SetBrush(wxBrush(m_trackColor));
    gc->SetPen(wxPen(m_trackColor, 1));
    gc->FillPath(trackPath);

    // Draw progress（用动画显示值，视觉连续）
    if (m_displayValue > 0) {
        int fillWidth = static_cast<int>(w * m_displayValue / 100.0);
        if (fillWidth > 0) {
            wxGraphicsPath fillPath = gc->CreatePath();
            // Create a rounded rectangle for the filled portion
            // We need to handle corners: left side rounded, right side straight unless 100%
            double rightRadius = (m_displayValue >= 100.0) ? r : 0;
            fillPath.MoveToPoint(0, 0);
            fillPath.AddLineToPoint(fillWidth, 0);
            if (rightRadius > 0) {
                fillPath.AddArc(fillWidth - rightRadius, rightRadius, rightRadius, 0, -M_PI_2, true);
                fillPath.AddArc(fillWidth - rightRadius, h - rightRadius, rightRadius, -M_PI_2, -M_PI, true);
            }
            fillPath.AddLineToPoint(0, h);
            if (r > 0) {
                fillPath.AddArc(rightRadius, h - r, r, -M_PI, -M_PI_2, true);
                fillPath.AddArc(rightRadius, r, r, -M_PI_2, 0, true);
            }
            fillPath.CloseSubpath();

            gc->SetBrush(wxBrush(m_progressColor));
            gc->SetPen(wxPen(m_progressColor, 1));
            gc->FillPath(fillPath);
        }
    }

    // Draw status text (right-aligned) — 仅在控件足够高时绘制，矮条模式下文字会被裁切
    if (!m_statusText.empty() && h >= 26) {
        gc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                           false, L"微软雅黑"), m_progressColor);
        double tw, th;
        gc->GetTextExtent(m_statusText, &tw, &th);
        gc->DrawText(m_statusText, w - tw - 4, (h - th) / 2);
    }

    delete gc;

    // 右侧百分比（画在填充末端上方，宽度足够时显示）
    if (w >= 220 && m_displayValue > 0) {
        wxString pct = wxString::Format(L"%.0f%%", m_displayValue);
        dc.SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                          false, L"微软雅黑"));
        int fillEnd = static_cast<int>(w * m_displayValue / 100.0);
        bool inside = fillEnd > w - 34;
        dc.SetTextForeground(inside ? wxColour(255, 255, 255)
                                    : ThemeManager::Instance().GetColors().textSecondary);
        wxSize ts = dc.GetTextExtent(pct);
        dc.DrawText(pct, w - ts.GetWidth() - 6, (h - ts.GetHeight()) / 2);
    }
}

void ScanProgressBar::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

} // namespace IceClean::Gui