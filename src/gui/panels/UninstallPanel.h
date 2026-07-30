#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/srchctrl.h>
#include <wx/notebook.h>
#include <vector>
#include "models/InstalledSoftware.h"
#include "core/analyzer/SoftwareUpdateChecker.h"

namespace IceClean::Gui {

// 软件卸载面板
class UninstallPanel : public wxPanel {
public:
    UninstallPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 刷新软件列表
    void RefreshSoftwareList();

private:
    // 软件列表数据
    std::vector<IceClean::Models::InstalledSoftware> m_softwareList;
    std::vector<IceClean::Models::InstalledSoftware> m_filteredList;  // 过滤后的列表
    int m_sortColumn = -1;
    bool m_sortAsc = true;

    // 可升级软件数据
    std::vector<IceClean::Core::Analyzer::UpdatableSoftware> m_updatableSoftware;

    // 控件
    wxNotebook* m_notebook = nullptr;
    wxSearchCtrl* m_searchCtrl = nullptr;
    wxListCtrl* m_softwareListCtrl = nullptr;
    wxListCtrl* m_updateListCtrl = nullptr;
    wxButton* m_uninstallButton = nullptr;
    wxButton* m_silentUninstallButton = nullptr;
    wxButton* m_refreshButton = nullptr;
    wxButton* m_cleanResidualButton = nullptr;
    wxButton* m_forceUninstallButton = nullptr;
    wxStaticText* m_statusLabel = nullptr;
    wxStaticText* m_totalSizeLabel = nullptr;
    wxButton* m_checkUpdateButton = nullptr;
    wxButton* m_openDownloadButton = nullptr;

    void CreateControls();

    // 事件处理
    void OnSearch(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnUninstall(wxCommandEvent& event);
    void OnSilentUninstall(wxCommandEvent& event);
    void OnCleanResidual(wxCommandEvent& event);
    void OnForceUninstall(wxCommandEvent& event);
    void OnCheckUpdate(wxCommandEvent& event);
    void OnOpenDownload(wxCommandEvent& event);
    void OnItemSelected(wxListEvent& event);
    void OnItemDeselected(wxListEvent& event);
    void OnColumnClick(wxListEvent& event);
    void OnItemActivated(wxListEvent& event);
    void OnUpdateItemSelected(wxListEvent& event);
    void OnUpdateItemDeselected(wxListEvent& event);

    // 辅助方法
    void PopulateList(const std::vector<IceClean::Models::InstalledSoftware>& items);
    void PopulateUpdateList(const std::vector<IceClean::Core::Analyzer::UpdatableSoftware>& items);
    void UpdateStatus();
    IceClean::Models::InstalledSoftware GetSelectedSoftware() const;
    wxString FormatSize(uint64_t bytes) const;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
