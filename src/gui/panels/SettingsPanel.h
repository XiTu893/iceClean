#pragma once
#include <wx/wx.h>
#include <vector>
#include <wx/listctrl.h>
#include "core/optimizer/ScheduledCleanManager.h"
#include "core/optimizer/ContextMenuManager.h"
#include "core/safety/UpdateChecker.h"

namespace IceClean::Gui {

// 设置面板
class SettingsPanel : public wxPanel {
public:
    SettingsPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 公开接口
    bool IsAutoRestoreEnabled() const;
    bool IsMinimizeToTrayEnabled() const;
    std::vector<int> GetEnabledCleanCategories() const;

    // 持久化
    void LoadSettings();
    void SaveSettings();

    // 日志
    void RefreshLog();

    // 更新检查
    void CheckForUpdate();

private:
    // 常规设置控件
    wxCheckBox* m_autoRestoreCheck = nullptr;
    wxCheckBox* m_minimizeToTrayCheck = nullptr;

    // 清理规则控件
    std::vector<wxCheckBox*> m_cleanRuleChecks;

    // 定时清理控件
    wxListCtrl* m_scheduleListCtrl = nullptr;
    wxButton* m_addScheduleBtn = nullptr;
    wxButton* m_editScheduleBtn = nullptr;
    wxButton* m_deleteScheduleBtn = nullptr;
    wxButton* m_runNowScheduleBtn = nullptr;
    std::vector<IceClean::Core::Optimizer::ScheduledCleanTask> m_scheduledTasks;

    // 右键菜单管理控件
    wxListCtrl* m_contextMenuListCtrl = nullptr;
    wxButton* m_scanCtxMenuBtn = nullptr;
    wxButton* m_disableCtxMenuBtn = nullptr;
    wxButton* m_enableCtxMenuBtn = nullptr;
    wxButton* m_deleteCtxMenuBtn = nullptr;
    std::vector<IceClean::Core::Optimizer::ContextMenuItem> m_contextMenuItems;

    // 白名单控件
    wxListCtrl* m_whitelistCtrl = nullptr;
    wxButton* m_addWhitelistBtn = nullptr;
    wxButton* m_removeWhitelistBtn = nullptr;

    // 操作日志控件
    wxListCtrl* m_logCtrl = nullptr;
    wxButton* m_clearLogBtn = nullptr;

    // 更新检查控件
    wxButton* m_checkUpdateBtn = nullptr;
    wxStaticText* m_updateStatusText = nullptr;

    void CreateControls();
    void CreateGeneralSection(wxWindow* parent, wxSizer* sizer);
    void CreateCleanRulesSection(wxWindow* parent, wxSizer* sizer);
    void CreateScheduledCleanSection(wxWindow* parent, wxSizer* sizer);
    void CreateContextMenuSection(wxWindow* parent, wxSizer* sizer);
    void CreateWhitelistSection(wxWindow* parent, wxSizer* sizer);
    void CreateLogSection(wxWindow* parent, wxSizer* sizer);
    void CreateAboutSection(wxWindow* parent, wxSizer* sizer);

    // 事件处理
    void OnAddWhitelist(wxCommandEvent& event);
    void OnRemoveWhitelist(wxCommandEvent& event);
    void OnClearLog(wxCommandEvent& event);
    void OnAddSchedule(wxCommandEvent& event);
    void OnEditSchedule(wxCommandEvent& event);
    void OnDeleteSchedule(wxCommandEvent& event);
    void OnRunNowSchedule(wxCommandEvent& event);
    void OnScanContextMenu(wxCommandEvent& event);
    void OnDisableContextMenu(wxCommandEvent& event);
    void OnEnableContextMenu(wxCommandEvent& event);
    void OnDeleteContextMenu(wxCommandEvent& event);
    void OnCheckUpdate(wxCommandEvent& event);

    // 辅助方法
    void RefreshScheduledTasks();
    wxString GetScheduleTypeString(IceClean::Core::Optimizer::ScheduledCleanTask::ScheduleType type) const;
    wxString GetScheduleTimeString(const IceClean::Core::Optimizer::ScheduledCleanTask& task) const;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
