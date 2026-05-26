#include "SettingsPanel.h"
#include "core/safety/OperationLogger.h"
#include "utils/JsonUtil.h"
#include <nlohmann/json.hpp>
#include <wx/filename.h>
#include <wx/statline.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(SettingsPanel, wxPanel)
wxEND_EVENT_TABLE()

SettingsPanel::SettingsPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(*wxWHITE);
    CreateControls();
}

void SettingsPanel::CreateControls() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"设置");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    auto* scrollWindow = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                               wxVSCROLL | wxBORDER_NONE);
    scrollWindow->SetBackgroundColour(*wxWHITE);
    scrollWindow->SetScrollRate(0, 10);

    auto* scrollSizer = new wxBoxSizer(wxVERTICAL);

    CreateGeneralSection(scrollWindow, scrollSizer);
    CreateCleanRulesSection(scrollWindow, scrollSizer);
    CreateWhitelistSection(scrollWindow, scrollSizer);
    CreateLogSection(scrollWindow, scrollSizer);
    CreateAboutSection(scrollWindow, scrollSizer);

    scrollWindow->SetSizer(scrollSizer);
    mainSizer->Add(scrollWindow, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);

    SetSizer(mainSizer);
}

void SettingsPanel::CreateGeneralSection(wxWindow* parent, wxSizer* sizer) {
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"常规设置");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
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
    sizer->Add(m_minimizeToTrayCheck, 0, wxLEFT | wxBOTTOM, 16);

    auto* line = new wxStaticLine(parent, wxID_ANY);
    sizer->Add(line, 0, wxEXPAND | wxBOTTOM, 12);
}

void SettingsPanel::CreateCleanRulesSection(wxWindow* parent, wxSizer* sizer) {
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"清理规则");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    sizer->Add(sectionLabel, 0, wxTOP | wxBOTTOM, 8);

    auto* descLabel = new wxStaticText(parent, wxID_ANY, L"选择一键扫描时包含的清理类别:");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
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

void SettingsPanel::CreateWhitelistSection(wxWindow* parent, wxSizer* sizer) {
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"白名单管理");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    sizer->Add(sectionLabel, 0, wxTOP | wxBOTTOM, 8);

    auto* descLabel = new wxStaticText(parent, wxID_ANY,
        L"白名单中的路径不会被清理。添加需要保护的文件或文件夹路径。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
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
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"操作日志");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
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
    auto* sectionLabel = new wxStaticText(parent, wxID_ANY, L"关于");
    sectionLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    sectionLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    sizer->Add(sectionLabel, 0, wxTOP | wxBOTTOM, 8);

    auto* aboutSizer = new wxBoxSizer(wxVERTICAL);

    auto* nameLabel = new wxStaticText(parent, wxID_ANY, L"IceClean - 智能C盘清理与迁移工具");
    nameLabel->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                              false, L"微软雅黑"));
    nameLabel->SetForegroundColour(wxColour(0x00, 0x78, 0xD4));
    aboutSizer->Add(nameLabel, 0, wxBOTTOM, 4);

    auto* versionLabel = new wxStaticText(parent, wxID_ANY, L"版本: 1.0.0");
    versionLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    versionLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    aboutSizer->Add(versionLabel, 0, wxBOTTOM, 4);

    auto* techLabel = new wxStaticText(parent, wxID_ANY,
        L"技术栈: C++20 + wxWidgets 3.3 + CMake + vcpkg");
    techLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    techLabel->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
    aboutSizer->Add(techLabel, 0, wxBOTTOM, 4);

    auto* copyrightLabel = new wxStaticText(parent, wxID_ANY, L"© 2025 IceClean. All rights reserved.");
    copyrightLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    copyrightLabel->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
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
