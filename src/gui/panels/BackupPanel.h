#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include "models/BackupInfo.h"

namespace IceClean::Gui {

// 系统备份面板
class BackupPanel : public wxPanel {
public:
    BackupPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 刷新备份列表
    void RefreshBackupList();

private:
    std::vector<IceClean::Models::BackupRecord> m_backupList;

    // 控件
    wxListCtrl* m_backupListCtrl = nullptr;
    wxButton* m_backupRegistryButton = nullptr;
    wxButton* m_backupStartupButton = nullptr;
    wxButton* m_backupFullButton = nullptr;
    wxButton* m_restoreButton = nullptr;
    wxButton* m_deleteButton = nullptr;
    wxButton* m_cleanupButton = nullptr;
    wxButton* m_refreshButton = nullptr;
    wxStaticText* m_statusLabel = nullptr;
    wxStaticText* m_totalSizeLabel = nullptr;

    void CreateControls();

    // 事件处理
    void OnBackupRegistry(wxCommandEvent& event);
    void OnBackupStartup(wxCommandEvent& event);
    void OnBackupFull(wxCommandEvent& event);
    void OnRestore(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnCleanup(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnItemSelected(wxListEvent& event);
    void OnItemDeselected(wxListEvent& event);

    // 辅助方法
    void PopulateList();
    void UpdateStatus();
    wxString GetBackupTypeName(IceClean::Models::BackupType type) const;
    wxString FormatSize(uint64_t bytes) const;
    wxString FormatTime(std::chrono::system_clock::time_point time) const;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
