#pragma once
#include <wx/wx.h>
#include <wx/graphics.h>

namespace IceClean::Gui {

class PauseOverlay : public wxPanel {
public:
    PauseOverlay(wxWindow* parent, wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize);
    ~PauseOverlay() override = default;

    void ShowOverlay(bool show);
    bool IsVisible() const { return m_visible; }

    void HideOverlay() { ShowOverlay(false); }
    void SetStatusText(const wxString& text);
    wxString GetStatusText() const { return m_statusText; }

protected:
    void OnPaint(wxPaintEvent& event);
    void OnEraseBackground(wxEraseEvent& event);

private:
    bool m_visible = false;
    wxString m_statusText;
    wxColour m_overlayColor;
    wxColour m_textColor;

private:
    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui