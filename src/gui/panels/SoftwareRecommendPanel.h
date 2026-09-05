#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <vector>
#include "models/RecommendedSoftware.h"

namespace IceClean::Gui {

// 推荐软件面板
// 分类以 Tab 页呈现；每个分类内软件以单列列表展示
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

    // 填充单个 Tab 页的单列软件列表
    void FillTab(wxScrolledWindow* scroll,
                 const std::vector<IceClean::Models::RecommendedSoftware>& software);

    // 创建单列行式软件卡片
    wxPanel* CreateSoftwareCard(wxWindow* parent,
                                const IceClean::Models::RecommendedSoftware& software);

    // 事件处理
    void OnDownloadClick(wxCommandEvent& event);
    void OnVisitClick(wxCommandEvent& event);

    // 数据
    IceClean::Models::RecommendData m_data;

    // 控件
    wxNotebook* m_categoryNotebook = nullptr;   // 分类 Tab：全部 + 各分类
    wxStaticText* m_statusLabel = nullptr;
    wxButton* m_refreshButton = nullptr;
    wxStaticText* m_updateTimeLabel = nullptr;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
