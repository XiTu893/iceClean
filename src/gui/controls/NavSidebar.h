#pragma once
#include <wx/wx.h>
#include <vector>

namespace IceClean::Gui {

struct NavItem {
    std::wstring label;
    int iconId;  // Icon identifier
};

class NavSidebar : public wxPanel {
public:
    NavSidebar(wxWindow* parent);

    int GetSelection() const { return m_selection; }
    void SetSelection(int index);

    // Custom event
    static wxDECLARE_EVENT(wxEVT_NAV_SELECTION_CHANGED);

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);

    int m_selection = 0;
    int m_hoverItem = -1;
    std::vector<NavItem> m_items;

    // Colors
    wxColour m_bgColor;
    wxColour m_selectedColor;
    wxColour m_hoverColor;
    wxColour m_textColor;
    wxColour m_selectedTextColor;

    static const int ITEM_HEIGHT = 48;
    static const int ICON_SIZE = 24;
    static const int PADDING = 16;
};

} // namespace IceClean::Gui
