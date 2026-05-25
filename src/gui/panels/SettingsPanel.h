#pragma once
#include <wx/wx.h>
#include <vector>
#include <wx/listctrl.h>

namespace IceClean::Gui {

// 设置面板
class SettingsPanel : public wxPanel {
public:
    SettingsPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 公开接口
    bool IsAutoRestoreEnabled() const;
    bool IsMinimizeToTrayEnabled() const;
    std::vector<int> GetEnabledCleanCategories() const;  // 返回选中的清理类别索引

    // 持久化
    void LoadSettings();
    void SaveSettings();

    // 日志
    void RefreshLog();

private:
    // 常规设置控件
    wxCheckBox* m_autoRestoreCheck = nullptr;
    wxCheckBox* m_minimizeToTrayCheck = nullptr;

    // 清理规则控件
    std::vector<wxCheckBox*> m_cleanRuleChecks;

    // 白名单控件
    wxListCtrl* m_whitelistCtrl = nullptr;
    wxButton* m_addWhitelistBtn = nullptr;
    wxButton* m_removeWhitelistBtn = nullptr;

    // 操作日志控件
    wxListCtrl* m_logCtrl = nullptr;
    wxButton* m_clearLogBtn = nullptr;

    void CreateControls();
    void CreateGeneralSection(wxWindow* parent, wxSizer* sizer);
    void CreateCleanRulesSection(wxWindow* parent, wxSizer* sizer);
    void CreateWhitelistSection(wxWindow* parent, wxSizer* sizer);
    void CreateLogSection(wxWindow* parent, wxSizer* sizer);
    void CreateAboutSection(wxWindow* parent, wxSizer* sizer);

    // 事件处理
    void OnAddWhitelist(wxCommandEvent& event);
    void OnRemoveWhitelist(wxCommandEvent& event);
    void OnClearLog(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
