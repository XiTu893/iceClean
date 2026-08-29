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

// 智能迁移面板
class MigrationPanel : public wxPanel {
public:
    MigrationPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 设置扫描到的大文件列表
    void SetMigrationItems(const std::vector<IceClean::Models::MigrationItem>& items);

    // 获取选中的迁移项
    std::vector<IceClean::Models::MigrationItem> GetSelectedItems() const;

    // 获取目标驱动器
    wxString GetTargetDrive() const;

    // 扫描过程实况（由 MainWindow 从工作线程经事件转发，UI 线程调用）
    void UpdateScanProgress(const wxString& phase, const wxString& currentPath, int foundCount);

private:
    std::vector<IceClean::Models::MigrationItem> m_items;

    // 控件
    wxButton* m_scanButton = nullptr;
    wxButton* m_stopButton = nullptr;
    wxListCtrl* m_fileList = nullptr;
    wxChoice* m_targetDriveChoice = nullptr;
    wxButton* m_migrateButton = nullptr;
    wxStaticText* m_statusLabel = nullptr;
    IceClean::Gui::ScanInfoPanel* m_scanInfoPanel = nullptr;  // 扫描实况条
    wxStaticText* m_currentPathLabel = nullptr;             // 当前扫描目录
    wxString m_currentPath;                                  // 完整路径，仅在控件中显示省略版

    void CreateControls();
    void PopulateDriveList();

    // 事件处理
    void OnScanButton(wxCommandEvent& event);
    void OnStopButton(wxCommandEvent& event);
    void OnMigrateButton(wxCommandEvent& event);
    void OnItemChecked(wxListEvent& event);
    void OnMigrationScanProgress(wxThreadEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
