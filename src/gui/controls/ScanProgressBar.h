#pragma once
#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/timer.h>

namespace IceClean::Gui {

class ScanProgressBar : public wxPanel {
public:
    ScanProgressBar(wxWindow* parent, wxWindowID id = wxID_ANY,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize);
    ~ScanProgressBar() override;

    void SetValue(double value); // 0-100（内部平滑趋近，视觉连续）
    double GetValue() const { return m_value; }

    void SetStatusText(const wxString& text);
    wxString GetStatusText() const { return m_statusText; }

    void SetColor(const wxColour& color);
    wxColour GetColor() const { return m_progressColor; }

protected:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnAnimTimer(wxTimerEvent& event);

private:
    void EnsureAnimRunning();
    void EnsureAnimStopped();

    double m_value = 0.0;        // 目标值
    double m_displayValue = 0.0; // 动画显示值
    wxColour m_progressColor;
    wxColour m_backColor;
    wxColour m_trackColor;
    wxString m_statusText;
    int m_height = 12;
    int m_cornerRadius = 6;
    wxTimer* m_animTimer = nullptr;

private:
    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
