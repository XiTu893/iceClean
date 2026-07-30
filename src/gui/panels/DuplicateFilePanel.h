#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/dirctrl.h>
#include <vector>
#include "models/DuplicateFileGroup.h"

namespace IceClean::Gui {

// 重复文件查找面板
class DuplicateFilePanel : public wxPanel {
public:
    DuplicateFilePanel(wxWindow* parent, wxWindowID id = wxID_ANY);

private:
    std::vector<IceClean::Models::DuplicateFileGroup> m_duplicateGroups;
    bool m_isScanning = false;

    // 控件
    wxTextCtrl* m_pathText = nullptr;
    wxButton* m_browseButton = nullptr;
    wxButton* m_scanButton = nullptr;
    wxButton* m_stopButton = nullptr;
    wxSpinCtrl* m_minSizeSpin = nullptr;
    wxListCtrl* m_resultListCtrl = nullptr;
    wxListCtrl* m_detailListCtrl = nullptr;
    wxButton* m_deleteSelectedButton = nullptr;
    wxButton* m_moveSelectedButton = nullptr;
    wxStaticText* m_statusLabel = nullptr;
    wxStaticText* m_wastedLabel = nullptr;
    wxGauge* m_progressGauge = nullptr;

    void CreateControls();

    // 事件处理
    void OnBrowse(wxCommandEvent& event);
    void OnScan(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnGroupSelected(wxListEvent& event);
    void OnGroupDeselected(wxListEvent& event);
    void OnDeleteSelected(wxCommandEvent& event);
    void OnMoveSelected(wxCommandEvent& event);
    void OnDetailItemChecked(wxListEvent& event);

    // 辅助方法
    void UpdateResultList();
    void UpdateWastedSpace();
    wxString FormatSize(uint64_t bytes) const;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
