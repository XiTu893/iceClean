#pragma once
#include <wx/wx.h>

namespace IceClean::Gui {

// A panel that renders as a card with rounded corners and shadow
class CardPanel : public wxPanel {
public:
    CardPanel(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString);

    wxSizer* GetCardSizer() { return m_cardSizer; }
    wxSizer* GetContentSizer() { return m_cardSizer; }

    // 设置是否可点击 (点击时发送 wxEVT_BUTTON 事件)
    void SetClickable(bool clickable);
    bool IsClickable() const { return m_clickable; }

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);

    wxString m_title;
    wxSizer* m_cardSizer = nullptr;
    bool m_clickable = false;

    static const int CORNER_RADIUS = 8;
    static const int PADDING = 16;
};

} // namespace IceClean::Gui
