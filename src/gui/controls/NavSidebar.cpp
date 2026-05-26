#include "NavSidebar.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace IceClean::Gui {

// ── Custom event definition ──

wxDEFINE_EVENT(wxEVT_NAV_SELECTION_CHANGED, wxCommandEvent);

// ── Constructor ──

NavSidebar::NavSidebar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(200, -1))
    , m_bgColor(30, 30, 46)           // #1E1E2E - dark background
    , m_selectedColor(0, 120, 212)     // #0078D4 - accent blue
    , m_hoverColor(45, 45, 65)         // slightly lighter than bg
    , m_textColor(200, 200, 210)       // light gray text
    , m_selectedTextColor(255, 255, 255) // white text for selected
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(m_bgColor);
    SetMinSize(wxSize(200, -1));

    // Define navigation items with colored circle indicators
    // iconId: 0=home, 1=clean, 2=migrate, 3=speedup, 4=analyze, 5=settings
    m_items = {
        { L"首页",   0 },
        { L"清理",   1 },
        { L"迁移",   2 },
        { L"加速",   3 },
        { L"分析",   4 },
        { L"设置",   5 }
    };

    // Bind events
    Bind(wxEVT_PAINT, &NavSidebar::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &NavSidebar::OnLeftDown, this);
    Bind(wxEVT_MOTION, &NavSidebar::OnMouseMove, this);
    Bind(wxEVT_LEAVE_WINDOW, &NavSidebar::OnMouseLeave, this);
}

// ── Selection ──

void NavSidebar::SetSelection(int index)
{
    if (index >= 0 && index < static_cast<int>(m_items.size()) && index != m_selection) {
        m_selection = index;
        Refresh();

        // Fire selection changed event
        wxCommandEvent event(wxEVT_NAV_SELECTION_CHANGED, GetId());
        event.SetInt(m_selection);
        event.SetEventObject(this);
        ProcessWindowEvent(event);
    }
}

// ── Paint ──

void NavSidebar::OnPaint(wxPaintEvent& /*event*/)
{
    wxBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);

    if (!gc) {
        // Fallback without graphics context
        dc.SetBackground(m_bgColor);
        dc.Clear();
        return;
    }

    gc->SetBrush(gc->CreateBrush(wxBrush(m_bgColor)));
    gc->DrawRectangle(wxRect2DDouble(0, 0, GetSize().x, GetSize().y));

    // Draw app title at the top
    gc->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), wxColour(255, 255, 255));
    gc->DrawText("IceClean", PADDING, 16);

    // Draw a subtle separator line below the title
    int titleAreaHeight = 56;
    gc->SetPen(gc->CreatePen(wxPen(wxColour(60, 60, 80), 1)));
    gc->StrokeLine(PADDING, titleAreaHeight, GetSize().x - PADDING, titleAreaHeight);

    // Item colors for the circle indicators
    const wxColour itemColors[] = {
        wxColour(0, 120, 212),    // Home - blue
        wxColour(16, 185, 129),   // Clean - green
        wxColour(249, 115, 22),   // Migrate - orange
        wxColour(234, 179, 8),    // Speedup - yellow
        wxColour(139, 92, 246),   // Analyze - purple
        wxColour(107, 114, 128),  // Settings - gray
    };

    // Draw navigation items
    int y = titleAreaHeight + 8;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        int itemY = y + i * ITEM_HEIGHT;
        wxRect itemRect(PADDING / 2, itemY, GetSize().x - PADDING, ITEM_HEIGHT);

        // Draw hover/selection background
        if (i == m_selection) {
            gc->SetBrush(gc->CreateBrush(wxBrush(m_selectedColor)));
            gc->DrawRoundedRectangle(itemRect.x, itemRect.y, itemRect.width, itemRect.height, 6);
        } else if (i == m_hoverItem) {
            gc->SetBrush(gc->CreateBrush(wxBrush(m_hoverColor)));
            gc->DrawRoundedRectangle(itemRect.x, itemRect.y, itemRect.width, itemRect.height, 6);
        }

        // Draw colored circle indicator
        int circleX = itemRect.x + PADDING;
        int circleY = itemRect.y + (ITEM_HEIGHT - ICON_SIZE) / 2;
        int circleRadius = ICON_SIZE / 2;

        wxColour indicatorColor = itemColors[i % 6];
        if (i == m_selection) {
            indicatorColor = wxColour(255, 255, 255);
        }

        gc->SetBrush(gc->CreateBrush(wxBrush(indicatorColor)));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawEllipse(circleX, circleY, ICON_SIZE, ICON_SIZE);

        // Draw short text label inside the circle
        gc->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD),
                    i == m_selection ? m_selectedColor : wxColour(255, 255, 255));
        const wchar_t* circleLabels[] = { L"H", L"C", L"M", L"S", L"A", L"G" };
        gc->DrawText(circleLabels[i], circleX + 6, circleY + 3);

        // Draw item label text
        wxColour textColor = (i == m_selection) ? m_selectedTextColor : m_textColor;
        gc->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL), textColor);
        gc->DrawText(m_items[i].label, circleX + ICON_SIZE + 10, itemY + (ITEM_HEIGHT - 20) / 2);
    }

    delete gc;
}

// ── Mouse events ──

void NavSidebar::OnLeftDown(wxMouseEvent& event)
{
    int titleAreaHeight = 64;
    int y = titleAreaHeight + 8;
    int clickedIndex = -1;

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        int itemY = y + i * ITEM_HEIGHT;
        wxRect itemRect(0, itemY, GetSize().x, ITEM_HEIGHT);
        if (itemRect.Contains(event.GetPosition())) {
            clickedIndex = i;
            break;
        }
    }

    if (clickedIndex >= 0) {
        SetSelection(clickedIndex);
    }
}

void NavSidebar::OnMouseMove(wxMouseEvent& event)
{
    int titleAreaHeight = 64;
    int y = titleAreaHeight + 8;
    int newHover = -1;

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        int itemY = y + i * ITEM_HEIGHT;
        wxRect itemRect(0, itemY, GetSize().x, ITEM_HEIGHT);
        if (itemRect.Contains(event.GetPosition())) {
            newHover = i;
            break;
        }
    }

    if (newHover != m_hoverItem) {
        m_hoverItem = newHover;
        Refresh();
    }
}

void NavSidebar::OnMouseLeave(wxMouseEvent& /*event*/)
{
    if (m_hoverItem != -1) {
        m_hoverItem = -1;
        Refresh();
    }
}

} // namespace IceClean::Gui
