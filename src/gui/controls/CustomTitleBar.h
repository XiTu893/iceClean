#pragma once

#include <wx/wx.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>

namespace IceClean::Gui {

// 自定义标题栏 - 对标 QQ/微信/钉钉 等商业软件的无边框窗口标题栏
class CustomTitleBar : public wxPanel {
public:
    CustomTitleBar(wxWindow* parent, wxFrame* ownerFrame);

    // 设置标题文字
    void SetTitle(const wxString& title);

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMinimize(wxCommandEvent& event);
    void OnMaximize(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnThemeChanged(const struct ThemeColors& colors);

    wxFrame*   m_ownerFrame = nullptr;
    wxString   m_title = L"IceClean";

    // 标题栏按钮区域
    wxButton*  m_minBtn = nullptr;
    wxButton*  m_maxBtn = nullptr;
    wxButton*  m_closeBtn = nullptr;

    // 拖拽状态
    bool       m_isDragging = false;
    wxPoint    m_dragStartPos;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
