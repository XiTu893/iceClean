#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/srchctrl.h>
#include <wx/notebook.h>
#include <vector>
#include "core/driver/DeviceDriverInfo.h"

namespace IceClean::Gui {

// 设备&驱动视图页（SetupDi，只读检测，默认在前）
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

    std::vector<Core::Driver::DeviceDriverInfo> m_all;
    std::vector<Core::Driver::DeviceDriverInfo> m_filtered;

    wxSearchCtrl* m_search = nullptr;
    wxChoice* m_filter = nullptr;
    wxButton* m_refresh = nullptr;
    wxListCtrl* m_list = nullptr;
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

// 驱动管理主面板：双 Tab 容器
class DriverPanel : public wxPanel {
public:
    DriverPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

private:
    void CreateControls();
    wxNotebook* m_notebook = nullptr;
    DeviceDriverPage* m_devicePage = nullptr;
    PackageDriverPage* m_packagePage = nullptr;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
