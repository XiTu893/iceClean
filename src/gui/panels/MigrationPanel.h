#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <vector>
#include "models/MigrationItem.h"

namespace IceClean::Gui {

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

private:
    std::vector<IceClean::Models::MigrationItem> m_items;

    // 控件
    wxButton* m_scanButton = nullptr;
    wxListCtrl* m_fileList = nullptr;
    wxChoice* m_targetDriveChoice = nullptr;
    wxButton* m_migrateButton = nullptr;

    void CreateControls();
    void PopulateDriveList();

    // 事件处理
    void OnScanButton(wxCommandEvent& event);
    void OnMigrateButton(wxCommandEvent& event);
    void OnItemChecked(wxListEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
