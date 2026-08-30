#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include <wx/thread.h>
#include "models/MigrationItem.h"
#include "gui/controls/ScanInfoPanel.h"
#include "gui/Events.h"

namespace IceClean::Gui {

// 迁移扫描进度载荷（worker 线程 → UI 线程）
struct MigrationScanProgressInfo {
    wxString phase;
    wxString path;
    int foundCount = 0;
};

// 迁移扫描请求载荷（UI 线程 → MainWindow）
struct MigrationScanRequestInfo {
    int scanType = 1;        // 1 = 迁移扫描
    int thresholdMB = 100;   // 扫描阈值
};

// 智能迁移面板
class MigrationPanel : public wxPanel {
public:
    MigrationPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    void SetMigrationItems(const std::vector<IceClean::Models::MigrationItem>& items);
    std::vector<IceClean::Models::MigrationItem> GetSelectedItems() const;
    wxString GetTargetDrive() const;
    void UpdateScanProgress(const wxString& phase, const wxString& currentPath, int foundCount);

private:
    std::vector<IceClean::Models::MigrationItem> m_items;

    wxButton* m_scanButton = nullptr;
    wxButton* m_stopButton = nullptr;
    wxCheckBox* m_headerCheckbox = nullptr;  // 列表上方全选 checkbox
    wxListCtrl* m_fileList = nullptr;
    wxChoice* m_targetDriveChoice = nullptr;
    wxButton* m_migrateButton = nullptr;
    wxButton* m_deleteButton = nullptr;     // 删除按钮
    wxStaticText* m_statusLabel = nullptr;
    IceClean::Gui::ScanInfoPanel* m_scanInfoPanel = nullptr;
    wxStaticText* m_currentPathLabel = nullptr;
    wxString m_currentPath;
    wxChoice* m_thresholdChoice = nullptr;
    int m_currentThresholdMB = 100;

    wxPanel* m_expandPanel = nullptr;
    wxStaticText* m_expandTitle = nullptr;
    wxStaticText* m_expandContent = nullptr;
    int m_expandedIndex = -1;

    void CreateControls();
    void PopulateDriveList();
    void PopulateThresholdList();
    void ShowExpandPanel(int itemIndex);
    void HideExpandPanel();
    std::wstring ScanLargeSubDirs(const std::wstring& parentPath, int thresholdBytes);
    void UpdateHeaderCheckboxState();

    void OnScanButton(wxCommandEvent& event);
    void OnStopButton(wxCommandEvent& event);
    void OnMigrateButton(wxCommandEvent& event);
    void OnDeleteButton(wxCommandEvent& event);
    void OnHeaderCheckbox(wxCommandEvent& event);
    void OnListItemChecked(wxListEvent& event);
    void OnItemActivated(wxListEvent& event);
    void OnMigrationScanProgress(wxThreadEvent& event);
    void OnThresholdChanged(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
