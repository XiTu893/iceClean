#pragma once
#include <wx/wx.h>

namespace IceClean::Gui {

// A panel that renders as a card with rounded corners and shadow
class CardPanel : public wxPanel {
public:
    CardPanel(wxWindow* parent, const wxString& title = wxEmptyString);

    wxSizer* GetCardSizer() { return m_cardSizer; }

private:
    void OnPaint(wxPaintEvent& event);

    wxString m_title;
    wxSizer* m_cardSizer = nullptr;

    static const int CORNER_RADIUS = 8;
    static const int PADDING = 16;
};

} // namespace IceClean::Gui
