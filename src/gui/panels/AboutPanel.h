#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>

namespace IceClean::Gui {

// 关于面板
class AboutPanel : public wxPanel {
public:
    AboutPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

private:
    wxScrolledWindow* m_scroller = nullptr;

    void CreateControls();

    // 辅助方法：从RCDATA资源加载图片
    wxImage LoadResourceImage(int resourceId);

    // 创建卡片式区块
    wxPanel* CreateCardSection(const wxString& title, const wxString& icon,
                                const std::vector<std::pair<wxString, wxString>>& items);

    // 创建带图标的特性行
    wxPanel* CreateFeatureRow(const wxString& icon, const wxString& title,
                               const wxString& desc);
};

} // namespace IceClean::Gui
