#pragma once
#include <wx/wx.h>
#include <vector>

namespace IceClean::Gui {

// 导航栏选择变更事件
wxDECLARE_EVENT(wxEVT_NAV_SELECTION_CHANGED, wxCommandEvent);

struct NavItem {
    std::wstring label;
    int iconId = -1;       // -1 = separator
};

class NavSidebar : public wxPanel {
public:
    NavSidebar(wxWindow* parent);

    int GetSelection() const { return m_selection; }
    void SetSelection(int index);

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);

    // 图标绘制方法
    void DrawHomeIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawCleanIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawMigrateIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawSpeedupIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawSettingsIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawAboutIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawUninstallIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawNetworkIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawPopupBlockerIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawRecommendIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawDiskIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawFileTypeIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);
    void DrawDownloadIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color);

    int m_selection = 0;
    int m_hoverItem = -1;
    std::vector<NavItem> m_items;

    // Colors
    wxColour m_bgColor;
    wxColour m_selectedColor;
    wxColour m_hoverColor;
    wxColour m_textColor;
    wxColour m_selectedTextColor;

    static const int ITEM_HEIGHT = 44;
    static const int ICON_SIZE = 22;
    static const int PADDING = 16;
    static const int SEPARATOR_HEIGHT = 12;
};

} // namespace IceClean::Gui
