#pragma once
#include <wx/wx.h>
#include <vector>
#include "models/StartupItem.h"

namespace IceClean::Gui {

// 启动优化面板
class StartupPanel : public wxPanel {
public:
    StartupPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 设置启动项列表
    void SetStartupItems(const std::vector<IceClean::Models::StartupItem>& items);

    // 设置服务列表
    void SetServices(const std::vector<IceClean::Models::StartupItem>& services);

    // 获取已修改的启动项
    std::vector<IceClean::Models::StartupItem> GetModifiedStartupItems() const;

    // 获取已修改的服务
    std::vector<IceClean::Models::StartupItem> GetModifiedServices() const;

private:
    std::vector<IceClean::Models::StartupItem> m_startupItems;
    std::vector<IceClean::Models::StartupItem> m_services;

    // 控件
    wxScrolledWindow* m_startupScroller = nullptr;
    wxBoxSizer* m_startupSizer = nullptr;
    wxScrolledWindow* m_serviceScroller = nullptr;
    wxBoxSizer* m_serviceSizer = nullptr;
    wxStaticText* m_bootTimeLabel = nullptr;
    wxButton* m_optimizeButton = nullptr;

    // Toggle开关控件数据
    struct ToggleItem {
        wxPanel* rowPanel = nullptr;
        wxPanel* togglePanel = nullptr;
        wxStaticText* nameLabel = nullptr;
        wxStaticText* publisherLabel = nullptr;
        wxStaticText* typeLabel = nullptr;
        bool isOn = true;
        bool isSystemCritical = false;
        int itemIndex = -1; // 在m_startupItems或m_services中的索引
    };
    std::vector<ToggleItem> m_startupToggles;
    std::vector<ToggleItem> m_serviceToggles;

    void CreateControls();
    void BuildStartupList();
    void BuildServiceList();
    void UpdateBootTimeEstimate();

    // 自绘Toggle开关
    wxPanel* CreateToggleSwitch(wxWindow* parent, bool isOn, bool canToggle);
    void DrawToggle(wxPanel* panel, bool isOn, bool canToggle);
    void OnToggleClick(wxMouseEvent& event);

    void OnOptimizeButton(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
