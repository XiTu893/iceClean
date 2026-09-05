#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <vector>
#include <wx/thread.h>
#include "models/MigrationItem.h"
#include "gui/controls/ScanInfoPanel.h"
#include "gui/Events.h"

namespace IceClean::Gui {

struct MigrationScanProgressInfo {
    wxString phase;
    wxString path;
    int foundCount = 0;
};

struct MigrationScanRequestInfo {
    int scanType = 1;        // 1 = 应用迁移扫描, 2 = 大文件夹扫描
    int thresholdMB = 100;
};

class MigrationPanel : public wxPanel {
public:
    MigrationPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    void SetMigrationItems(const std::vector<IceClean::Models::MigrationItem>& items);
    void SetProgramItems(const std::vector<IceClean::Models::MigrationItem>& items);
    std::vector<IceClean::Models::MigrationItem> GetSelectedItems() const;
    wxString GetTargetDrive() const;
    void UpdateScanProgress(const wxString& phase, const wxString& currentPath, int foundCount);
    int GetCurrentScanType() const { return m_currentScanType; }
    int GetCurrentThresholdMB() const { return m_currentThresholdMB; }

private:
    int m_currentScanType = 1;  // 1 = Application Migration, 2 = Large Folder
    std::vector<IceClean::Models::MigrationItem> m_items;

    wxButton* m_scanButton = nullptr;
    wxButton* m_stopButton = nullptr;
    wxCheckBox* m_headerCheckbox = nullptr;
    wxListCtrl* m_fileList = nullptr;
    wxChoice* m_targetDriveChoice = nullptr;
    wxButton* m_migrateButton = nullptr;
    wxButton* m_deleteButton = nullptr;
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

    wxNotebook* m_notebook = nullptr;
    wxPanel* m_programPage = nullptr;
    wxPanel* m_folderPage = nullptr;

    void CreateControls();
    void PopulateDriveList();
    void PopulateThresholdList();
    void ShowExpandPanel(int itemIndex);
    void HideExpandPanel();
    std::wstring ScanLargeSubDirs(const std::wstring& parentPath, int thresholdBytes);
    void UpdateHeaderCheckboxState();
    void SetupListColumnsForType(int scanType);
    void RefreshItemList();
    void CreateProgramTab();
    void CreateFolderTab();

    void OnScanButton(wxCommandEvent& event);
    void OnStopButton(wxCommandEvent& event);
    void OnMigrateButton(wxCommandEvent& event);
    void OnDeleteButton(wxCommandEvent& event);
    void OnHeaderCheckbox(wxCommandEvent& event);
    void OnListItemChecked(wxListEvent& event);
    void OnItemActivated(wxListEvent& event);
    void OnMigrationScanProgress(wxThreadEvent& event);
    void OnThresholdChanged(wxCommandEvent& event);
    void OnNotebookPageChanged(wxNotebookEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
