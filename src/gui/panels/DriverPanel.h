#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/srchctrl.h>
#include <vector>
#include "models/DriverInfo.h"

namespace IceClean::Gui {

// 驱动管理面板
class DriverPanel : public wxPanel {
public:
    DriverPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 刷新驱动列表
    void RefreshDriverList();

private:
    std::vector<IceClean::Models::DriverInfo> m_driverList;
    std::vector<IceClean::Models::DriverInfo> m_filteredList;
    int m_sortColumn = -1;
    bool m_sortAsc = true;

    // 控件
    wxSearchCtrl* m_searchCtrl = nullptr;
    wxListCtrl* m_driverListCtrl = nullptr;
    wxButton* m_backupAllButton = nullptr;
    wxButton* m_backupSelectedButton = nullptr;
    wxButton* m_cleanupButton = nullptr;
    wxButton* m_refreshButton = nullptr;
    wxStaticText* m_statusLabel = nullptr;
    wxStaticText* m_totalSizeLabel = nullptr;
    wxChoice* m_filterChoice = nullptr;

    void CreateControls();

    // 事件处理
    void OnSearch(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnBackupAll(wxCommandEvent& event);
    void OnBackupSelected(wxCommandEvent& event);
    void OnCleanup(wxCommandEvent& event);
    void OnItemSelected(wxListEvent& event);
    void OnItemDeselected(wxListEvent& event);
    void OnColumnClick(wxListEvent& event);
    void OnFilterChange(wxCommandEvent& event);

    // 辅助方法
    void PopulateList(const std::vector<IceClean::Models::DriverInfo>& items);
    void UpdateStatus();
    IceClean::Models::DriverInfo GetSelectedDriver() const;
    wxString FormatSize(uint64_t bytes) const;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
