#pragma once
#include <wx/wx.h>
#include <wx/graphics.h>

namespace IceClean::Gui {

// 圆形进度条控件
class CircularProgress : public wxPanel {
public:
    CircularProgress(wxWindow* parent,
                     wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize);

    // 设置进度值 (0-100)
    void SetValue(int value);
    int GetValue() const { return m_value; }

    // 设置进度弧颜色
    void SetProgressColor(const wxColour& color);
    const wxColour& GetProgressColor() const { return m_progressColor; }

    // 设置标签文字 (显示在圆心)
    void SetLabel(const wxString& label);
    wxString GetLabel() const override { return m_label; }

    // 设置副标签 (显示在主标签下方)
    void SetSubLabel(const wxString& subLabel);
    const wxString& GetSubLabel() const { return m_subLabel; }

private:
    int m_value = 0;
    wxColour m_progressColor = wxColour(0x00, 0x78, 0xD4); // #0078D4
    wxColour m_trackColor = wxColour(0xE0, 0xE0, 0xE0);    // 浅灰
    wxString m_label;
    wxString m_subLabel;

    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
