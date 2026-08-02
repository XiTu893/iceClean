#include "SettingsPanel.h"
#include "core/safety/OperationLogger.h"
#include "core/safety/UpdateChecker.h"
#include "core/optimizer/ScheduledCleanManager.h"
#include "core/optimizer/ContextMenuManager.h"
#include "gui/controls/ThemeManager.h"
#include "utils/JsonUtil.h"
#include "gui/resources/resource.h"
#include <nlohmann/json.hpp>
#include <wx/filename.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <thread>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(SettingsPanel, wxPanel)
wxEND_EVENT_TABLE()

SettingsPanel::SettingsPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();
}

void SettingsPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"设置");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    auto* scrollWindow = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                               wxVSCROLL | wxBORDER_NONE);
    scrollWindow->SetBackgroundColour(colors.background);
    scrollWindow->SetScrollRate(0, 10);

    auto* scrollSizer = new wxBoxSizer(wxVERTICAL);

    CreateGeneralSection(scrollWindow, scrollSizer);
    CreateCleanRulesSection(scrollWindow, scrollSizer);
    CreateScheduledCleanSection(scrollWindow, scrollSizer);
    CreateContextMenuSection(scrollWindow, scrollSizer);
    CreateWhitelistSection(scrollWindow, scrollSizer);
    CreateLogSection(scrollWindow, scrollSizer);
    CreateAboutSection(scrollWindow, scrollSizer);

    scrollWindow->SetSizer(scrollSizer);
    mainSizer->Add(scrollWindow, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);

    SetSizer(mainSizer);
}

void SettingsPanel::CreateGeneralSection(wxWindow* parent, wxSizer* sizer) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"常规设置");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(colors.textPrimary);
    sizer->Add(sectionLabel, 0, wxTOP | wxBOTTOM, 8);

    m_autoRestoreCheck = new wxCheckBox(parent, wxID_ANY, L"清理前自动创建系统还原点");
    m_autoRestoreCheck->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                       false, L"微软雅黑"));
    m_autoRestoreCheck->SetValue(true);
    sizer->Add(m_autoRestoreCheck, 0, wxLEFT | wxBOTTOM, 8);

    m_minimizeToTrayCheck = new wxCheckBox(parent, wxID_ANY, L"关闭时最小化到系统托盘");
    m_minimizeToTrayCheck->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
    m_minimizeToTrayCheck->SetValue(false);
    sizer->Add(m_minimizeToTrayCheck, 0, wxLEFT | wxBOTTOM, 8);

    // 主题选择
    auto* themeSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* themeLabel = new wxStaticText(parent, wxID_ANY, L"主题：");
    themeLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    themeLabel->SetForegroundColour(colors.textPrimary);
    themeSizer->Add(themeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    auto* themeChoice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxSize(160, 28));
    themeChoice->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    themeChoice->Append(L"浅色");
    themeChoice->Append(L"深色");
    themeChoice->Append(L"跟随系统");

    // 设置当前主题
    auto currentTheme = ThemeManager::Instance().GetTheme();
    themeChoice->SetSelection(static_cast<int>(currentTheme));

    themeChoice->Bind(wxEVT_CHOICE, [](wxCommandEvent& evt) {
        auto theme = static_cast<ThemeType>(evt.GetSelection());
        ThemeManager::Instance().SetTheme(theme);
    });

    themeSizer->Add(themeChoice, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(themeSizer, 0, wxLEFT | wxBOTTOM, 16);

    auto* line = new wxStaticLine(parent, wxID_ANY);
    sizer->Add(line, 0, wxEXPAND | wxBOTTOM, 12);
}

void SettingsPanel::CreateCleanRulesSection(wxWindow* parent, wxSizer* sizer) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"清理规则");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(colors.textPrimary);
    sizer->Add(sectionLabel, 0, wxTOP | wxBOTTOM, 8);

    auto* descLabel = new wxStaticText(parent, wxID_ANY, L"选择一键扫描时包含的清理类别:");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    sizer->Add(descLabel, 0, wxLEFT | wxBOTTOM, 8);

    const struct {
        const wchar_t* name;
        bool defaultChecked;
    } categories[] = {
        {L"系统临时文件", true},
        {L"Windows更新缓存", true},
        {L"浏览器缓存", true},
        {L"缩略图缓存", true},
        {L"预取文件", true},
        {L"错误报告/内存转储", true},
        {L"回收站", true},
        {L"传递优化文件", true},
        {L"系统日志", true},
        {L"旧驱动备份", true},
        {L"休眠文件", false},
        {L"旧Windows安装", false},
    };

    for (const auto& cat : categories) {
        auto* check = new wxCheckBox(parent, wxID_ANY, cat.name);
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        check->SetValue(cat.defaultChecked);
        sizer->Add(check, 0, wxLEFT | wxBOTTOM, 4);
        m_cleanRuleChecks.push_back(check);
    }

    sizer->AddSpacer(8);

    auto* line = new wxStaticLine(parent, wxID_ANY);
    sizer->Add(line, 0, wxEXPAND | wxBOTTOM, 12);
}

void SettingsPanel::CreateScheduledCleanSection(wxWindow* parent, wxSizer* sizer) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"定时清理");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(colors.textPrimary);
    sizer->Add(sectionLabel, 0, wxTOP | wxBOTTOM, 8);

    auto* descLabel = new wxStaticText(parent, wxID_ANY,
        L"设置定时清理计划，自动在指定时间清理系统垃圾文件。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    descLabel->Wrap(550);
    sizer->Add(descLabel, 0, wxLEFT | wxBOTTOM, 8);

    // 任务列表
    m_scheduleListCtrl = new wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 120),
                                         wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_scheduleListCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                       false, L"微软雅黑"));
    m_scheduleListCtrl->AppendColumn(L"任务名称", wxLIST_FORMAT_LEFT, 150);
    m_scheduleListCtrl->AppendColumn(L"频率", wxLIST_FORMAT_LEFT, 100);
    m_scheduleListCtrl->AppendColumn(L"执行时间", wxLIST_FORMAT_LEFT, 100);
    m_scheduleListCtrl->AppendColumn(L"状态", wxLIST_FORMAT_LEFT, 60);
    m_scheduleListCtrl->AppendColumn(L"上次运行", wxLIST_FORMAT_LEFT, 140);
    sizer->Add(m_scheduleListCtrl, 0, wxEXPAND | wxBOTTOM, 8);

    // 按钮行
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    m_addScheduleBtn = new wxButton(parent, wxID_ANY, L"新建计划", wxDefaultPosition, wxSize(100, 30));
    m_addScheduleBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
    m_addScheduleBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnAddSchedule, this);
    btnSizer->Add(m_addScheduleBtn, 0, wxRIGHT, 8);

    m_editScheduleBtn = new wxButton(parent, wxID_ANY, L"编辑", wxDefaultPosition, wxSize(70, 30));
    m_editScheduleBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                       false, L"微软雅黑"));
    m_editScheduleBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnEditSchedule, this);
    btnSizer->Add(m_editScheduleBtn, 0, wxRIGHT, 8);

    m_deleteScheduleBtn = new wxButton(parent, wxID_ANY, L"删除", wxDefaultPosition, wxSize(70, 30));
    m_deleteScheduleBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                         false, L"微软雅黑"));
    m_deleteScheduleBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnDeleteSchedule, this);
    btnSizer->Add(m_deleteScheduleBtn, 0, wxRIGHT, 8);

    m_runNowScheduleBtn = new wxButton(parent, wxID_ANY, L"立即执行", wxDefaultPosition, wxSize(90, 30));
    m_runNowScheduleBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
    m_runNowScheduleBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnRunNowSchedule, this);
    btnSizer->Add(m_runNowScheduleBtn, 0);

    sizer->Add(btnSizer, 0, wxBOTTOM, 16);

    auto* line = new wxStaticLine(parent, wxID_ANY);
    sizer->Add(line, 0, wxEXPAND | wxBOTTOM, 12);

    // 初始加载任务列表
    CallAfter([this]() { RefreshScheduledTasks(); });
}

void SettingsPanel::RefreshScheduledTasks() {
    m_scheduleListCtrl->DeleteAllItems();

    IceClean::Core::Optimizer::ScheduledCleanManager manager;
    m_scheduledTasks = manager.GetScheduledTasks();

    for (int i = 0; i < static_cast<int>(m_scheduledTasks.size()); ++i) {
        const auto& task = m_scheduledTasks[i];
        long idx = m_scheduleListCtrl->InsertItem(i, task.taskName);
        m_scheduleListCtrl->SetItem(idx, 1, GetScheduleTypeString(task.scheduleType));
        m_scheduleListCtrl->SetItem(idx, 2, GetScheduleTimeString(task));
        m_scheduleListCtrl->SetItem(idx, 3, task.enabled ? L"已启用" : L"已禁用");
        m_scheduleListCtrl->SetItem(idx, 4, task.lastRunTime.empty() ? L"未运行" : task.lastRunTime);
    }
}

wxString SettingsPanel::GetScheduleTypeString(
    IceClean::Core::Optimizer::ScheduledCleanTask::ScheduleType type) const {
    using Type = IceClean::Core::Optimizer::ScheduledCleanTask::ScheduleType;
    switch (type) {
    case Type::Daily:   return L"每天";
    case Type::Weekly:  return L"每周";
    case Type::Monthly: return L"每月";
    default:            return L"未知";
    }
}

wxString SettingsPanel::GetScheduleTimeString(
    const IceClean::Core::Optimizer::ScheduledCleanTask& task) const {
    wxString timeStr = wxString::Format(L"%02d:%02d", task.hour, task.minute);
    return timeStr;
}

void SettingsPanel::OnAddSchedule(wxCommandEvent& event) {
    // 使用简单对话框创建定时清理任务
    wxDialog dlg(this, wxID_ANY, L"新建定时清理计划", wxDefaultPosition, wxSize(400, 500),
                 wxDEFAULT_DIALOG_STYLE);
    dlg.SetBackgroundColour(ThemeManager::Instance().GetColors().background);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(12);

    // 任务名称
    auto* nameLabel = new wxStaticText(&dlg, wxID_ANY, L"任务名称:");
    nameLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    sizer->Add(nameLabel, 0, wxLEFT | wxRIGHT, 16);
    auto* nameText = new wxTextCtrl(&dlg, wxID_ANY, L"定时清理");
    nameText->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    sizer->Add(nameText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);

    // 频率
    auto* freqLabel = new wxStaticText(&dlg, wxID_ANY, L"执行频率:");
    freqLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    sizer->Add(freqLabel, 0, wxLEFT | wxRIGHT, 16);
    wxChoice* freqChoice = new wxChoice(&dlg, wxID_ANY);
    freqChoice->Append(L"每天");
    freqChoice->Append(L"每周");
    freqChoice->Append(L"每月");
    freqChoice->SetSelection(0);
    freqChoice->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    sizer->Add(freqChoice, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);

    // 执行时间
    auto* timeLabel = new wxStaticText(&dlg, wxID_ANY, L"执行时间 (时:分):");
    timeLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    sizer->Add(timeLabel, 0, wxLEFT | wxRIGHT, 16);
    auto* timeSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* hourSpin = new wxSpinCtrl(&dlg, wxID_ANY, L"3", wxDefaultPosition, wxSize(60, -1),
                                     wxSP_ARROW_KEYS, 0, 23, 3);
    auto* sepLabel = new wxStaticText(&dlg, wxID_ANY, L":");
    auto* minSpin = new wxSpinCtrl(&dlg, wxID_ANY, L"0", wxDefaultPosition, wxSize(60, -1),
                                    wxSP_ARROW_KEYS, 0, 59, 0);
    timeSizer->Add(hourSpin, 0, wxRIGHT, 4);
    timeSizer->Add(sepLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
    timeSizer->Add(minSpin, 0);
    sizer->Add(timeSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 16);

    // 清理选项
    auto* optionLabel = new wxStaticText(&dlg, wxID_ANY, L"清理选项:");
    optionLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    sizer->Add(optionLabel, 0, wxLEFT | wxRIGHT, 16);

    auto* cleanTempCheck = new wxCheckBox(&dlg, wxID_ANY, L"系统临时文件");
    cleanTempCheck->SetValue(true);
    sizer->Add(cleanTempCheck, 0, wxLEFT | wxRIGHT, 20);

    auto* cleanBrowserCheck = new wxCheckBox(&dlg, wxID_ANY, L"浏览器缓存");
    cleanBrowserCheck->SetValue(true);
    sizer->Add(cleanBrowserCheck, 0, wxLEFT | wxRIGHT, 20);

    auto* cleanRecycleCheck = new wxCheckBox(&dlg, wxID_ANY, L"回收站");
    sizer->Add(cleanRecycleCheck, 0, wxLEFT | wxRIGHT, 20);

    auto* cleanThumbCheck = new wxCheckBox(&dlg, wxID_ANY, L"缩略图缓存");
    cleanThumbCheck->SetValue(true);
    sizer->Add(cleanThumbCheck, 0, wxLEFT | wxRIGHT, 20);

    auto* cleanLogCheck = new wxCheckBox(&dlg, wxID_ANY, L"系统日志");
    cleanLogCheck->SetValue(true);
    sizer->Add(cleanLogCheck, 0, wxLEFT | wxRIGHT, 20);

    auto* shutdownCheck = new wxCheckBox(&dlg, wxID_ANY, L"清理后关机");
    sizer->Add(shutdownCheck, 0, wxLEFT | wxRIGHT | wxBOTTOM, 16);

    // 按钮
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer();
    auto* okBtn = new wxButton(&dlg, wxID_OK, L"创建");
    auto* cancelBtn = new wxButton(&dlg, wxID_CANCEL, L"取消");
    btnSizer->Add(okBtn, 0, wxRIGHT, 8);
    btnSizer->Add(cancelBtn, 0);
    sizer->Add(btnSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 16);

    dlg.SetSizer(sizer);

    if (dlg.ShowModal() == wxID_OK) {
        IceClean::Core::Optimizer::ScheduledCleanTask task;
        task.taskName = nameText->GetValue().ToStdWstring();
        task.scheduleType = static_cast<IceClean::Core::Optimizer::ScheduledCleanTask::ScheduleType>(
            freqChoice->GetSelection());
        task.hour = hourSpin->GetValue();
        task.minute = minSpin->GetValue();
        task.cleanTemp = cleanTempCheck->GetValue();
        task.cleanBrowserCache = cleanBrowserCheck->GetValue();
        task.cleanRecycleBin = cleanRecycleCheck->GetValue();
        task.cleanThumbnails = cleanThumbCheck->GetValue();
        task.cleanLogs = cleanLogCheck->GetValue();
        task.shutdownAfterClean = shutdownCheck->GetValue();

        IceClean::Core::Optimizer::ScheduledCleanManager manager;
        task.taskId = std::to_wstring(
            std::chrono::system_clock::now().time_since_epoch().count());

        if (manager.CreateScheduledTask(task)) {
            RefreshScheduledTasks();
            wxMessageBox(L"定时清理计划创建成功！", L"IceClean", wxOK | wxICON_INFORMATION, this);
        } else {
            wxMessageBox(L"创建定时清理计划失败，请确保以管理员权限运行。", L"IceClean",
                         wxOK | wxICON_WARNING, this);
        }
    }
}

void SettingsPanel::OnEditSchedule(wxCommandEvent& event) {
    long sel = m_scheduleListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_scheduledTasks.size())) {
        wxMessageBox(L"请先选择一个计划任务。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    // 切换启用/禁用状态
    auto& task = m_scheduledTasks[sel];
    IceClean::Core::Optimizer::ScheduledCleanManager manager;
    manager.EnableScheduledTask(task.taskId, !task.enabled);
    RefreshScheduledTasks();
}

void SettingsPanel::OnDeleteSchedule(wxCommandEvent& event) {
    long sel = m_scheduleListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_scheduledTasks.size())) {
        wxMessageBox(L"请先选择一个计划任务。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    if (wxMessageBox(L"确定要删除此定时清理计划吗？", L"确认",
                     wxYES_NO | wxICON_QUESTION) == wxYES) {
        IceClean::Core::Optimizer::ScheduledCleanManager manager;
        manager.DeleteScheduledTask(m_scheduledTasks[sel].taskId);
        RefreshScheduledTasks();
    }
}

void SettingsPanel::OnRunNowSchedule(wxCommandEvent& event) {
    long sel = m_scheduleListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_scheduledTasks.size())) {
        wxMessageBox(L"请先选择一个计划任务。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    IceClean::Core::Optimizer::ScheduledCleanManager manager;
    if (manager.RunNow(m_scheduledTasks[sel])) {
        wxMessageBox(L"已启动清理任务。", L"IceClean", wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(L"启动清理任务失败。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void SettingsPanel::CreateContextMenuSection(wxWindow* parent, wxSizer* sizer) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"右键菜单管理");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(colors.textPrimary);
    sizer->Add(sectionLabel, 0, wxTOP | wxBOTTOM, 8);

    auto* descLabel = new wxStaticText(parent, wxID_ANY,
        L"管理Windows右键菜单项，可禁用或删除不需要的菜单项以加速右键响应。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    descLabel->Wrap(550);
    sizer->Add(descLabel, 0, wxLEFT | wxBOTTOM, 8);

    // 列表
    m_contextMenuListCtrl = new wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 150),
                                             wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_contextMenuListCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_contextMenuListCtrl->AppendColumn(L"菜单项", wxLIST_FORMAT_LEFT, 180);
    m_contextMenuListCtrl->AppendColumn(L"位置", wxLIST_FORMAT_LEFT, 80);
    m_contextMenuListCtrl->AppendColumn(L"命令", wxLIST_FORMAT_LEFT, 200);
    m_contextMenuListCtrl->AppendColumn(L"状态", wxLIST_FORMAT_LEFT, 60);
    sizer->Add(m_contextMenuListCtrl, 0, wxEXPAND | wxBOTTOM, 8);

    // 按钮
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    m_scanCtxMenuBtn = new wxButton(parent, wxID_ANY, L"扫描", wxDefaultPosition, wxSize(70, 30));
    m_scanCtxMenuBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
    m_scanCtxMenuBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnScanContextMenu, this);
    btnSizer->Add(m_scanCtxMenuBtn, 0, wxRIGHT, 8);

    m_disableCtxMenuBtn = new wxButton(parent, wxID_ANY, L"禁用", wxDefaultPosition, wxSize(70, 30));
    m_disableCtxMenuBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                         false, L"微软雅黑"));
    m_disableCtxMenuBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnDisableContextMenu, this);
    btnSizer->Add(m_disableCtxMenuBtn, 0, wxRIGHT, 8);

    m_enableCtxMenuBtn = new wxButton(parent, wxID_ANY, L"启用", wxDefaultPosition, wxSize(70, 30));
    m_enableCtxMenuBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                        false, L"微软雅黑"));
    m_enableCtxMenuBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnEnableContextMenu, this);
    btnSizer->Add(m_enableCtxMenuBtn, 0, wxRIGHT, 8);

    m_deleteCtxMenuBtn = new wxButton(parent, wxID_ANY, L"删除", wxDefaultPosition, wxSize(70, 30));
    m_deleteCtxMenuBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                         false, L"微软雅黑"));
    m_deleteCtxMenuBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnDeleteContextMenu, this);
    btnSizer->Add(m_deleteCtxMenuBtn, 0);

    sizer->Add(btnSizer, 0, wxBOTTOM, 16);

    auto* line = new wxStaticLine(parent, wxID_ANY);
    sizer->Add(line, 0, wxEXPAND | wxBOTTOM, 12);
}

void SettingsPanel::OnScanContextMenu(wxCommandEvent& event) {
    m_scanCtxMenuBtn->Enable(false);

    std::thread([this]() {
        IceClean::Core::Optimizer::ContextMenuManager manager;
        auto items = manager.ScanContextMenu();

        CallAfter([this, items = std::move(items)]() mutable {
            m_contextMenuItems = std::move(items);
            m_contextMenuListCtrl->DeleteAllItems();

            for (int i = 0; i < static_cast<int>(m_contextMenuItems.size()); ++i) {
                const auto& item = m_contextMenuItems[i];
                long idx = m_contextMenuListCtrl->InsertItem(i, item.displayText);
                m_contextMenuListCtrl->SetItem(idx, 1,
                    IceClean::Core::Optimizer::ContextMenuManager::GetLocationName(item.location));
                // 截断过长的命令
                wxString cmd = item.command;
                if (cmd.length() > 50) cmd = cmd.Mid(0, 50) + L"...";
                m_contextMenuListCtrl->SetItem(idx, 2, cmd);
                m_contextMenuListCtrl->SetItem(idx, 3, item.isEnabled ? L"启用" : L"禁用");
            }

            m_scanCtxMenuBtn->Enable(true);
        });
    }).detach();
}

void SettingsPanel::OnDisableContextMenu(wxCommandEvent& event) {
    long sel = m_contextMenuListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_contextMenuItems.size())) {
        wxMessageBox(L"请先选择一个菜单项。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    if (!m_contextMenuItems[sel].isEnabled) {
        wxMessageBox(L"该菜单项已被禁用。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    IceClean::Core::Optimizer::ContextMenuManager manager;
    if (manager.DisableItem(m_contextMenuItems[sel])) {
        wxMessageBox(L"已禁用该右键菜单项。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        OnScanContextMenu(event);
    } else {
        wxMessageBox(L"禁用失败，请以管理员权限运行。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void SettingsPanel::OnEnableContextMenu(wxCommandEvent& event) {
    long sel = m_contextMenuListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_contextMenuItems.size())) {
        wxMessageBox(L"请先选择一个菜单项。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    if (m_contextMenuItems[sel].isEnabled) {
        wxMessageBox(L"该菜单项已启用。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    IceClean::Core::Optimizer::ContextMenuManager manager;
    if (manager.EnableItem(m_contextMenuItems[sel])) {
        wxMessageBox(L"已启用该右键菜单项。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        OnScanContextMenu(event);
    } else {
        wxMessageBox(L"启用失败。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void SettingsPanel::OnDeleteContextMenu(wxCommandEvent& event) {
    long sel = m_contextMenuListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_contextMenuItems.size())) {
        wxMessageBox(L"请先选择一个菜单项。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    if (wxMessageBox(L"确定要永久删除该右键菜单项吗？此操作不可恢复！",
                     L"警告", wxYES_NO | wxICON_WARNING) == wxYES) {
        IceClean::Core::Optimizer::ContextMenuManager manager;
        if (manager.DeleteItem(m_contextMenuItems[sel])) {
            wxMessageBox(L"已删除该右键菜单项。", L"IceClean", wxOK | wxICON_INFORMATION, this);
            OnScanContextMenu(event);
        } else {
            wxMessageBox(L"删除失败，请以管理员权限运行。", L"IceClean", wxOK | wxICON_WARNING, this);
        }
    }
}

void SettingsPanel::CreateWhitelistSection(wxWindow* parent, wxSizer* sizer) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"白名单管理");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(colors.textPrimary);
    sizer->Add(sectionLabel, 0, wxTOP | wxBOTTOM, 8);

    auto* descLabel = new wxStaticText(parent, wxID_ANY,
        L"白名单中的路径不会被清理。添加需要保护的文件或文件夹路径。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    descLabel->Wrap(550);
    sizer->Add(descLabel, 0, wxLEFT | wxBOTTOM, 8);

    m_whitelistCtrl = new wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 150),
                                     wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_whitelistCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
    m_whitelistCtrl->AppendColumn(L"路径", wxLIST_FORMAT_LEFT, 500);
    sizer->Add(m_whitelistCtrl, 0, wxEXPAND | wxBOTTOM, 8);

    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    m_addWhitelistBtn = new wxButton(parent, wxID_ANY, L"添加路径", wxDefaultPosition, wxSize(100, 30));
    m_addWhitelistBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
    m_addWhitelistBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnAddWhitelist, this);
    btnSizer->Add(m_addWhitelistBtn, 0, wxRIGHT, 8);

    m_removeWhitelistBtn = new wxButton(parent, wxID_ANY, L"删除选中", wxDefaultPosition, wxSize(100, 30));
    m_removeWhitelistBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                         false, L"微软雅黑"));
    m_removeWhitelistBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnRemoveWhitelist, this);
    btnSizer->Add(m_removeWhitelistBtn, 0);

    sizer->Add(btnSizer, 0, wxBOTTOM, 16);

    auto* line = new wxStaticLine(parent, wxID_ANY);
    sizer->Add(line, 0, wxEXPAND | wxBOTTOM, 12);
}

void SettingsPanel::CreateLogSection(wxWindow* parent, wxSizer* sizer) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"操作日志");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(colors.textPrimary);
    sizer->Add(sectionLabel, 0, wxTOP | wxBOTTOM, 8);

    m_logCtrl = new wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 200),
                               wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_logCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    m_logCtrl->AppendColumn(L"时间", wxLIST_FORMAT_LEFT, 140);
    m_logCtrl->AppendColumn(L"操作", wxLIST_FORMAT_LEFT, 80);
    m_logCtrl->AppendColumn(L"描述", wxLIST_FORMAT_LEFT, 250);
    m_logCtrl->AppendColumn(L"结果", wxLIST_FORMAT_LEFT, 60);
    sizer->Add(m_logCtrl, 0, wxEXPAND | wxBOTTOM, 8);

    m_clearLogBtn = new wxButton(parent, wxID_ANY, L"清空日志", wxDefaultPosition, wxSize(100, 30));
    m_clearLogBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                  false, L"微软雅黑"));
    m_clearLogBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnClearLog, this);
    sizer->Add(m_clearLogBtn, 0, wxBOTTOM, 16);

    auto* line = new wxStaticLine(parent, wxID_ANY);
    sizer->Add(line, 0, wxEXPAND | wxBOTTOM, 12);
}

void SettingsPanel::CreateAboutSection(wxWindow* parent, wxSizer* sizer) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"关于");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(colors.textPrimary);
    sizer->Add(sectionLabel, 0, wxTOP | wxBOTTOM, 8);

    auto* aboutSizer = new wxBoxSizer(wxVERTICAL);

    auto* nameLabel = new wxStaticText(parent, wxID_ANY, L"IceClean - 智能C盘清理与迁移工具");
    nameLabel->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                              false, L"微软雅黑"));
    nameLabel->SetForegroundColour(colors.accent);
    aboutSizer->Add(nameLabel, 0, wxBOTTOM, 4);

    auto versionStr = wxString::Format(L"版本: %d.%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_BUILD);
    auto* versionLabel = new wxStaticText(parent, wxID_ANY, versionStr);
    versionLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    versionLabel->SetForegroundColour(colors.textSecondary);
    aboutSizer->Add(versionLabel, 0, wxBOTTOM, 4);

    // 更新检查区域
    auto* updateSizer = new wxBoxSizer(wxHORIZONTAL);
    m_checkUpdateBtn = new wxButton(parent, wxID_ANY, L"检查更新", wxDefaultPosition, wxSize(100, 30));
    m_checkUpdateBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
    m_checkUpdateBtn->Bind(wxEVT_BUTTON, &SettingsPanel::OnCheckUpdate, this);
    updateSizer->Add(m_checkUpdateBtn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_updateStatusText = new wxStaticText(parent, wxID_ANY, L"");
    m_updateStatusText->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                        false, L"微软雅黑"));
    m_updateStatusText->SetForegroundColour(colors.textSecondary);
    updateSizer->Add(m_updateStatusText, 0, wxALIGN_CENTER_VERTICAL);
    aboutSizer->Add(updateSizer, 0, wxBOTTOM, 8);

    auto* techLabel = new wxStaticText(parent, wxID_ANY,
        L"技术栈: C++20 + wxWidgets 3.3 + CMake + vcpkg");
    techLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    techLabel->SetForegroundColour(colors.textDisabled);
    aboutSizer->Add(techLabel, 0, wxBOTTOM, 4);

    auto* copyrightLabel = new wxStaticText(parent, wxID_ANY, L"© 2025 IceClean. All rights reserved.");
    copyrightLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    copyrightLabel->SetForegroundColour(colors.textDisabled);
    aboutSizer->Add(copyrightLabel);

    sizer->Add(aboutSizer, 0, wxLEFT | wxBOTTOM, 8);
}

void SettingsPanel::OnAddWhitelist(wxCommandEvent& event) {
    wxDirDialog dlg(this, L"选择要添加到白名单的文件夹");
    if (dlg.ShowModal() == wxID_OK) {
        wxString path = dlg.GetPath();
        m_whitelistCtrl->InsertItem(m_whitelistCtrl->GetItemCount(), path);
    }
}

void SettingsPanel::OnRemoveWhitelist(wxCommandEvent& event) {
    long sel = m_whitelistCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel >= 0) {
        m_whitelistCtrl->DeleteItem(sel);
    }
}

void SettingsPanel::OnClearLog(wxCommandEvent& event) {
    if (wxMessageBox(L"确定要清空所有操作日志吗？", L"确认",
                     wxYES_NO | wxICON_QUESTION) == wxYES) {
        IceClean::Core::Safety::OperationLogger::ClearLog();
        m_logCtrl->DeleteAllItems();
    }
}

void SettingsPanel::OnCheckUpdate(wxCommandEvent& event) {
    m_checkUpdateBtn->Enable(false);
    m_updateStatusText->SetLabelText(L"正在检查更新...");
    m_updateStatusText->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);

    IceClean::Core::Safety::UpdateChecker::Instance().CheckForUpdate(
        [this](const IceClean::Models::UpdateCheckResult& result) {
            CallAfter([this, result]() {
                m_checkUpdateBtn->Enable(true);

                if (result.networkError) {
                    m_updateStatusText->SetLabelText(L"检查失败: " + result.errorMessage);
                    m_updateStatusText->SetForegroundColour(ThemeManager::Instance().GetColors().danger);
                } else if (result.hasUpdate) {
                    auto newVer = result.latestVersion.version;
                    m_updateStatusText->SetLabelText(L"发现新版本: " + newVer);
                    m_updateStatusText->SetForegroundColour(ThemeManager::Instance().GetColors().success);

                    wxString msg = wxString::Format(
                        L"发现新版本 %s！\n\n当前版本: %d.%d.%d.%d\n最新版本: %s\n\n%s\n\n是否前往下载页面？",
                        newVer.c_str(),
                        result.currentVersion.major, result.currentVersion.minor,
                        result.currentVersion.patch, result.currentVersion.build,
                        newVer.c_str(),
                        result.latestVersion.releaseNotes.empty() ? L"" : result.latestVersion.releaseNotes.c_str());

                    int answer = wxMessageBox(msg, L"发现新版本",
                        wxYES_NO | wxICON_INFORMATION, this);

                    if (answer == wxYES && !result.latestVersion.downloadUrl.empty()) {
                        wxLaunchDefaultBrowser(result.latestVersion.downloadUrl);
                    }
                } else {
                    m_updateStatusText->SetLabelText(L"当前已是最新版本");
                    m_updateStatusText->SetForegroundColour(ThemeManager::Instance().GetColors().success);
                }
            });
        });
}

void SettingsPanel::CheckForUpdate() {
    auto& updater = IceClean::Core::Safety::UpdateChecker::Instance();
    if (updater.ShouldCheckUpdate()) {
        wxCommandEvent dummy;
        OnCheckUpdate(dummy);
    }
}

// ── 公开接口 ──

bool SettingsPanel::IsAutoRestoreEnabled() const {
    return m_autoRestoreCheck && m_autoRestoreCheck->GetValue();
}

bool SettingsPanel::IsMinimizeToTrayEnabled() const {
    return m_minimizeToTrayCheck && m_minimizeToTrayCheck->GetValue();
}

std::vector<int> SettingsPanel::GetEnabledCleanCategories() const {
    std::vector<int> enabled;
    for (int i = 0; i < static_cast<int>(m_cleanRuleChecks.size()); ++i) {
        if (m_cleanRuleChecks[i]->GetValue()) {
            enabled.push_back(i);
        }
    }
    return enabled;
}

// ── 持久化 ──

void SettingsPanel::LoadSettings() {
    auto configPath = IceClean::Utils::JsonUtil::GetConfigPath() + L"\\settings.json";
    auto json = IceClean::Utils::JsonUtil::LoadJson(configPath);
    if (json.is_null()) return;

    if (json.contains("autoRestore") && json["autoRestore"].is_boolean()) {
        m_autoRestoreCheck->SetValue(json["autoRestore"].get<bool>());
    }
    if (json.contains("minimizeToTray") && json["minimizeToTray"].is_boolean()) {
        m_minimizeToTrayCheck->SetValue(json["minimizeToTray"].get<bool>());
    }
    if (json.contains("cleanCategories") && json["cleanCategories"].is_array()) {
        auto cats = json["cleanCategories"];
        for (int i = 0; i < static_cast<int>(m_cleanRuleChecks.size()) && i < static_cast<int>(cats.size()); ++i) {
            if (cats[i].is_boolean()) {
                m_cleanRuleChecks[i]->SetValue(cats[i].get<bool>());
            }
        }
    }
}

void SettingsPanel::SaveSettings() {
    nlohmann::json json;
    json["autoRestore"] = m_autoRestoreCheck->GetValue();
    json["minimizeToTray"] = m_minimizeToTrayCheck->GetValue();

    auto cats = nlohmann::json::array();
    for (const auto* check : m_cleanRuleChecks) {
        cats.push_back(check->GetValue());
    }
    json["cleanCategories"] = cats;

    auto configPath = IceClean::Utils::JsonUtil::GetConfigPath() + L"\\settings.json";
    IceClean::Utils::JsonUtil::SaveJson(configPath, json);
}

// ── 日志刷新 ──

void SettingsPanel::RefreshLog() {
    m_logCtrl->DeleteAllItems();

    auto records = IceClean::Core::Safety::OperationLogger::GetRecentOperations(50);
    for (size_t i = 0; i < records.size(); ++i) {
        const auto& record = records[i];

        // 时间格式化
        auto timeT = std::chrono::system_clock::to_time_t(record.timestamp);
        struct tm tmBuf;
        localtime_s(&tmBuf, &timeT);
        wchar_t timeStr[64];
        wcsftime(timeStr, 64, L"%Y-%m-%d %H:%M", &tmBuf);

        wxString typeStr;
        switch (record.type) {
            case IceClean::Models::OperationType::Clean:    typeStr = L"清理"; break;
            case IceClean::Models::OperationType::Migrate:  typeStr = L"迁移"; break;
            case IceClean::Models::OperationType::Optimize: typeStr = L"优化"; break;
            case IceClean::Models::OperationType::Restore:  typeStr = L"还原"; break;
        }

        long idx = m_logCtrl->InsertItem(static_cast<long>(i), timeStr);
        m_logCtrl->SetItem(idx, 1, typeStr);
        m_logCtrl->SetItem(idx, 2, record.description);
        m_logCtrl->SetItem(idx, 3, record.success ? L"成功" : L"失败");
    }
}

} // namespace IceClean::Gui
