#include "CardPanel.h"
#include "ThemeManager.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace IceClean::Gui {

CardPanel::CardPanel(wxWindow* parent, wxWindowID id, const wxString& title)
    : wxPanel(parent, id)
    , m_title(title)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(GetParent()->GetBackgroundColour());
    SetName("card");

    auto* outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->AddSpacer(4);

    auto* innerSizer = new wxBoxSizer(wxVERTICAL);

    if (!m_title.IsEmpty()) {
        const auto& colors = ThemeManager::Instance().GetColors();
        auto* titleLabel = new wxStaticText(this, wxID_ANY, m_title);
        titleLabel->SetFont(ThemeManager::GetSubtitleFont());
        titleLabel->SetForegroundColour(colors.textPrimary);
        innerSizer->Add(titleLabel, 0, wxBOTTOM, 8);
    }

    m_cardSizer = new wxBoxSizer(wxVERTICAL);
    innerSizer->Add(m_cardSizer, 1, wxEXPAND);

    outerSizer->Add(innerSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, PADDING);
    SetSizer(outerSizer);

    Bind(wxEVT_PAINT, &CardPanel::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &CardPanel::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &CardPanel::OnLeftUp, this);
    Bind(wxEVT_ENTER_WINDOW, &CardPanel::OnMouseEnter, this);
    Bind(wxEVT_LEAVE_WINDOW, &CardPanel::OnMouseLeave, this);
}

void CardPanel::SetClickable(bool clickable) {
    m_clickable = clickable;
    SetCursor(clickable ? wxCURSOR_HAND : wxCURSOR_ARROW);
}

void CardPanel::OnLeftDown(wxMouseEvent& event) {
    if (m_clickable) {
        m_isPressed = true;
        Refresh();
    }
    event.Skip();
}

void CardPanel::OnLeftUp(wxMouseEvent& event) {
    if (m_clickable && m_isPressed) {
        m_isPressed = false;
        Refresh();
        wxCommandEvent btnEvent(wxEVT_BUTTON, GetId());
        btnEvent.SetEventObject(this);
        ProcessWindowEvent(btnEvent);
    }
    event.Skip();
}

void CardPanel::OnMouseEnter(wxMouseEvent& event) {
    if (m_clickable) {
        m_isHovered = true;
        Refresh();
    }
    event.Skip();
}

void CardPanel::OnMouseLeave(wxMouseEvent& event) {
    if (m_clickable && (m_isHovered || m_isPressed)) {
        m_isHovered = false;
        m_isPressed = false;
        Refresh();
    }
    event.Skip();
}

void CardPanel::OnPaint(wxPaintEvent& /*event*/)
{
    wxBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);

    if (!gc) {
        dc.SetBackground(GetParent()->GetBackgroundColour());
        dc.Clear();
        return;
    }

    const auto& colors = ThemeManager::Instance().GetColors();

    // Clear background with parent color
    gc->SetBrush(gc->CreateBrush(wxBrush(GetParent()->GetBackgroundColour())));
    gc->DrawRectangle(wxRect2DDouble(0, 0, GetSize().GetWidth(), GetSize().GetHeight()));

    auto rect = GetClientRect();

    // Shadow parameters based on state
    int shadowOffsetX = 2;
    int shadowOffsetY = 3;

    if (m_clickable && m_isPressed) {
        shadowOffsetX = 1;
        shadowOffsetY = 1;
    } else if (m_clickable && m_isHovered) {
        shadowOffsetX = 3;
        shadowOffsetY = 5;
    }

    // Draw shadow (双层阴影，更自然)
    wxRect shadowRect1 = rect;
    shadowRect1.Offset(shadowOffsetX, shadowOffsetY);
    shadowRect1.Deflate(2);

    // Outer shadow (ambient)
    gc->SetBrush(gc->CreateBrush(wxBrush(wxColour(0, 0, 0, 12))));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRoundedRectangle(shadowRect1.x, shadowRect1.y + 2, shadowRect1.width, shadowRect1.height, CORNER_RADIUS);

    // Inner shadow (contact)
    gc->SetBrush(gc->CreateBrush(wxBrush(wxColour(0, 0, 0, 8))));
    gc->DrawRoundedRectangle(shadowRect1.x + 1, shadowRect1.y + 1, shadowRect1.width, shadowRect1.height, CORNER_RADIUS);

    // Draw card body
    wxRect cardRect = rect;
    cardRect.Deflate(2);

    // Card background gradient - 悬停时使用 surfaceHover 作为终止色
    wxColour gradientEnd = colors.cardGradientEnd;
    if (m_clickable && m_isHovered && !m_isPressed) {
        gradientEnd = colors.surfaceHover;
    } else if (m_clickable && m_isPressed) {
        gradientEnd = colors.surfaceHover;
    }

    wxGraphicsBrush cardBrush = gc->CreateLinearGradientBrush(
        cardRect.x, cardRect.y, cardRect.x, cardRect.y + cardRect.height,
        colors.cardGradientStart, gradientEnd);
    gc->SetBrush(cardBrush);
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRoundedRectangle(cardRect.x, cardRect.y, cardRect.width, cardRect.height, CORNER_RADIUS);

    // Draw border - 使用 ThemeManager 边框颜色
    wxColour borderColor = colors.border;
    if (m_clickable && m_isHovered) {
        borderColor = colors.accent;  // 品牌蓝边框悬停
    }
    gc->SetPen(gc->CreatePen(wxPen(borderColor, 1)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRoundedRectangle(cardRect.x, cardRect.y, cardRect.width, cardRect.height, CORNER_RADIUS);

    delete gc;
}

} // namespace IceClean::Gui
