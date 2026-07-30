#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include "models/NetworkInfo.h"

namespace IceClean::Gui {

// 网络优化面板
class NetworkPanel : public wxPanel {
public:
    NetworkPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

private:
    std::vector<IceClean::Models::NetworkOptimizeItem> m_optimizeItems;
    std::vector<IceClean::Models::NetworkAdapterInfo> m_adapters;
    std::vector<IceClean::Models::DnsConfig> m_dnsConfigs;

    // 控件
    wxListCtrl* m_adapterListCtrl = nullptr;
    wxListCtrl* m_optimizeListCtrl = nullptr;
    wxChoice* m_dnsChoice = nullptr;
    wxButton* m_applyOptButton = nullptr;
    wxButton* m_applyDnsButton = nullptr;
    wxButton* m_resetDnsButton = nullptr;
    wxButton* m_flushDnsButton = nullptr;
    wxButton* m_refreshButton = nullptr;
    wxStaticText* m_statusLabel = nullptr;
    wxStaticText* m_pingResultLabel = nullptr;

    void CreateControls();

    // 事件处理
    void OnRefresh(wxCommandEvent& event);
    void OnApplyOptimize(wxCommandEvent& event);
    void OnApplyDns(wxCommandEvent& event);
    void OnResetDns(wxCommandEvent& event);
    void OnFlushDns(wxCommandEvent& event);
    void OnPingTest(wxCommandEvent& event);
    void OnOptimizeChecked(wxListEvent& event);

    // 辅助方法
    void LoadAdapters();
    void LoadOptimizeItems();
    void LoadDnsConfigs();
    void UpdateOptimizeStatus();

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
