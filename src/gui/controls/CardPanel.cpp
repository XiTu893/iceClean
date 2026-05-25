#include "CardPanel.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace IceClean::Gui {

CardPanel::CardPanel(wxWindow* parent, wxWindowID id, const wxString& title)
    : wxPanel(parent, id)
    , m_title(title)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(GetParent()->GetBackgroundColour());

    // Outer sizer with padding for the shadow effect
    auto* outerSizer = new wxBoxSizer(wxVERTICAL);

    // Add top padding for shadow offset
    outerSizer->AddSpacer(2);

    // Inner panel area with padding
    auto* innerSizer = new wxBoxSizer(wxVERTICAL);

    // Add title if provided
    if (!m_title.IsEmpty()) {
        auto* titleLabel = new wxStaticText(this, wxID_ANY, m_title);
        titleLabel->SetFont(wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        titleLabel->SetForegroundColour(wxColour(30, 30, 30));
        innerSizer->Add(titleLabel, 0, wxBOTTOM, 8);
    }

    // Card content sizer for external use
    m_cardSizer = new wxBoxSizer(wxVERTICAL);
    innerSizer->Add(m_cardSizer, 1, wxEXPAND);

    outerSizer->Add(innerSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, PADDING);
    SetSizer(outerSizer);

    Bind(wxEVT_PAINT, &CardPanel::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &CardPanel::OnLeftDown, this);
}

void CardPanel::SetClickable(bool clickable) {
    m_clickable = clickable;
    SetCursor(clickable ? wxCURSOR_HAND : wxCURSOR_ARROW);
}

void CardPanel::OnLeftDown(wxMouseEvent& event) {
    if (m_clickable) {
        // 发送 wxEVT_BUTTON 事件，让父窗口可以 Bind 处理
        wxCommandEvent btnEvent(wxEVT_BUTTON, GetId());
        btnEvent.SetEventObject(this);
        ProcessWindowEvent(btnEvent);
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

    // Clear background with parent color
    gc->SetBrush(gc->CreateBrush(wxBrush(GetParent()->GetBackgroundColour())));
    gc->DrawRectangle(wxRect(wxPoint(0, 0), GetSize()));

    auto rect = GetClientRect();

    // Draw shadow (offset, semi-transparent)
    wxRect shadowRect = rect;
    shadowRect.Offset(2, 2);
    shadowRect.Deflate(2);
    gc->SetBrush(gc->CreateBrush(wxBrush(wxColour(0, 0, 0, 25))));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRoundedRectangle(shadowRect.x, shadowRect.y, shadowRect.width, shadowRect.height, CORNER_RADIUS);

    // Draw card body (white rounded rectangle)
    wxRect cardRect = rect;
    cardRect.Deflate(2);
    gc->SetBrush(gc->CreateBrush(wxBrush(*wxWHITE)));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRoundedRectangle(cardRect.x, cardRect.y, cardRect.width, cardRect.height, CORNER_RADIUS);

    // Draw subtle border
    gc->SetPen(gc->CreatePen(wxPen(wxColour(230, 230, 230), 1)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRoundedRectangle(cardRect.x, cardRect.y, cardRect.width, cardRect.height, CORNER_RADIUS);

    delete gc;
}

} // namespace IceClean::Gui
