#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <vector>
#include "models/RecommendedSoftware.h"

namespace IceClean::Gui {

// 推荐软件面板
// 展示从数据库读取的推荐软件，按分类显示
class SoftwareRecommendPanel : public wxPanel {
public:
    SoftwareRecommendPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 刷新推荐软件列表
    void RefreshList();

    // 从数据库加载数据并显示
    void LoadFromDB();

    // 手动触发更新（从网络获取最新数据）
    void OnRefreshFromNetwork(wxCommandEvent& event);

private:
    void CreateControls();

    // 构建分类标签页
    void BuildCategoryTabs(const std::vector<IceClean::Models::RecommendCategory>& categories);

    // 构建指定分类的软件卡片列表
    void BuildSoftwareCards(wxScrolledWindow* container,
                            const std::vector<IceClean::Models::RecommendedSoftware>& software);

    // 创建单个软件卡片
    wxPanel* CreateSoftwareCard(wxWindow* parent,
                                const IceClean::Models::RecommendedSoftware& software);

    // 事件处理
    void OnCategorySelected(wxCommandEvent& event);
    void OnDownloadClick(wxCommandEvent& event);
    void OnVisitClick(wxCommandEvent& event);

    // 数据
    IceClean::Models::RecommendData m_data;
    int m_selectedCategoryIndex = 0;

    // 控件
    wxPanel* m_categoryBar = nullptr;
    wxScrolledWindow* m_contentScroller = nullptr;
    wxStaticText* m_statusLabel = nullptr;
    wxButton* m_refreshButton = nullptr;
    wxStaticText* m_updateTimeLabel = nullptr;

    // 分类按钮
    std::vector<wxButton*> m_categoryButtons;
    wxBoxSizer* m_categorySizer = nullptr;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
