#pragma once
#include <wx/wx.h>
#include <wx/dataview.h>
#include <wx/srchctrl.h>
#include <wx/notebook.h>
#include <wx/tglbtn.h>
#include <wx/listctrl.h>
#include <vector>
#include <set>
#include "core/driver/DeviceDriverInfo.h"
#include "core/driver/DriverBackupManager.h"

namespace IceClean::Gui {

// 设备&驱动视图页：基于 wxDataViewCtrl，支持勾选 + 列排序 + 按类别分组
class DeviceDriverPage : public wxPanel {
public:
    DeviceDriverPage(wxWindow* parent, wxWindowID id = wxID_ANY);

private:
    void CreateControls();
    void RefreshList();
    void Populate(const std::vector<Core::Driver::DeviceDriverInfo>& items);
    void ApplyFilterAndSearch();
    void OnSearch(wxCommandEvent& event);
    void OnFilter(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnToggleGroup(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);
    void OnSelectNone(wxCommandEvent& event);
    void OnSelectThirdParty(wxCommandEvent& event);
    void OnBackupSelected(wxCommandEvent& event);
    void OnValueChanged(wxDataViewEvent& event);
    void OnHeaderSort(wxDataViewEvent& event);

    // 收集当前勾选的所有 oemXX.inf（去重）
    std::vector<std::wstring> CollectSelectedOemInfs() const;

    std::vector<Core::Driver::DeviceDriverInfo> m_all;
    // 视图行号到设备数据的映射（包含组行占位，driverDesc=="__GROUP__"）
    std::vector<Core::Driver::DeviceDriverInfo> m_rowToDevice;
    int m_sortColumn = 0;     // -1: 默认按设备名
    bool m_sortAsc = true;
    bool m_groupByClass = true;

    wxSearchCtrl* m_search = nullptr;
    wxChoice* m_filter = nullptr;
    wxToggleButton* m_toggleGroup = nullptr;
    wxButton* m_refresh = nullptr;
    wxButton* m_selectAll = nullptr;
    wxButton* m_selectNone = nullptr;
    wxButton* m_selectThirdParty = nullptr;
    wxButton* m_backup = nullptr;
    wxDataViewListCtrl* m_view = nullptr;
    wxStaticText* m_status = nullptr;
};

// 驱动包视图页（pnputil /enum-drivers，备份/还原单元）
class PackageDriverPage : public wxPanel {
public:
    PackageDriverPage(wxWindow* parent, wxWindowID id = wxID_ANY);

private:
    void CreateControls();
    void RefreshList();
    void Populate(const std::vector<Core::Driver::DriverPackageInfo>& items);
    void ApplyFilterAndSearch();
    void OnSearch(wxCommandEvent& event);
    void OnFilter(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnBackupAll(wxCommandEvent& event);
    void OnBackupSelected(wxCommandEvent& event);
    void OnRestore(wxCommandEvent& event);
    void OnCleanup(wxCommandEvent& event);

    std::vector<Core::Driver::DriverPackageInfo> m_all;
    std::vector<Core::Driver::DriverPackageInfo> m_filtered;
    int m_sortColumn = -1;
    bool m_sortAsc = true;

    wxSearchCtrl* m_search = nullptr;
    wxChoice* m_filter = nullptr;
    wxButton* m_refresh = nullptr;
    wxButton* m_backupAll = nullptr;
    wxButton* m_backupSelected = nullptr;
    wxButton* m_restore = nullptr;
    wxButton* m_cleanup = nullptr;
    wxListCtrl* m_list = nullptr;
    wxStaticText* m_status = nullptr;
    wxStaticText* m_ticker = nullptr;

    Core::Driver::DriverPackageInfo GetSelected() const;
};

// 备份历史视图页：按时间倒序展开每次备份的全部包清单
class BackupHistoryPage : public wxPanel {
public:
    BackupHistoryPage(wxWindow* parent, wxWindowID id = wxID_ANY);

private:
    void CreateControls();
    void Refresh();
    void Populate(const std::vector<Core::Driver::DriverBackupManager::BackupEntry>& entries);
    void OnRefresh(wxCommandEvent& event);
    void OnRestore(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnOpenFolder(wxCommandEvent& event);
    void OnItemActivated(wxDataViewEvent& event);

    wxDataViewListCtrl* m_view = nullptr;
    wxButton* m_refresh = nullptr;
    wxButton* m_restore = nullptr;
    wxButton* m_delete = nullptr;
    wxButton* m_openFolder = nullptr;
    wxStaticText* m_status = nullptr;
    std::vector<Core::Driver::DriverBackupManager::BackupEntry> m_entries;

    int SelectedIndex() const;
};

// 驱动管理主面板：三 Tab 容器
class DriverPanel : public wxPanel {
public:
    DriverPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

private:
    void CreateControls();
    wxNotebook* m_notebook = nullptr;
    DeviceDriverPage* m_devicePage = nullptr;
    PackageDriverPage* m_packagePage = nullptr;
    BackupHistoryPage* m_historyPage = nullptr;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
