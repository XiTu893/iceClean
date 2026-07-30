#include "BackupPanel.h"
#include "gui/controls/ThemeManager.h"
#include "core/safety/SystemBackupManager.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "utils/FormatUtil.h"
#include <thread>
#include <algorithm>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(BackupPanel, wxPanel)
wxEND_EVENT_TABLE()

BackupPanel::BackupPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.background);
    CreateControls();
}

void BackupPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"系统备份");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* tipLabel = new wxStaticText(this, wxID_ANY,
        L"创建和管理系统备份，在优化或清理前保护系统状态。");
    tipLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    tipLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(tipLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 快速备份按钮区
    auto* quickSizer = new wxBoxSizer(wxHORIZONTAL);
    quickSizer->AddSpacer(20);

    m_backupRegistryButton = new wxButton(this, wxID_ANY, L"备份注册表", wxDefaultPosition, wxSize(120, 36));
    m_backupRegistryButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_backupRegistryButton->SetBackgroundColour(colors.accent);
    m_backupRegistryButton->SetForegroundColour(*wxWHITE);
    m_backupRegistryButton->Bind(wxEVT_BUTTON, &BackupPanel::OnBackupRegistry, this);
    quickSizer->Add(m_backupRegistryButton, 0, wxRIGHT, 8);

    m_backupStartupButton = new wxButton(this, wxID_ANY, L"备份启动项", wxDefaultPosition, wxSize(120, 36));
    m_backupStartupButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_backupStartupButton->Bind(wxEVT_BUTTON, &BackupPanel::OnBackupStartup, this);
    quickSizer->Add(m_backupStartupButton, 0, wxRIGHT, 8);

    m_backupFullButton = new wxButton(this, wxID_ANY, L"完整注册表备份", wxDefaultPosition, wxSize(130, 36));
    m_backupFullButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_backupFullButton->Bind(wxEVT_BUTTON, &BackupPanel::OnBackupFull, this);
    quickSizer->Add(m_backupFullButton, 0, wxRIGHT, 8);

    quickSizer->AddStretchSpacer();

    m_totalSizeLabel = new wxStaticText(this, wxID_ANY, L"");
    m_totalSizeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_totalSizeLabel->SetForegroundColour(colors.textSecondary);
    quickSizer->Add(m_totalSizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    mainSizer->Add(quickSizer, 0, wxEXPAND);
    mainSizer->AddSpacer(8);

    // 备份列表
    m_backupListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_backupListCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_backupListCtrl->AppendColumn(L"描述", wxLIST_FORMAT_LEFT, 200);
    m_backupListCtrl->AppendColumn(L"类型", wxLIST_FORMAT_CENTER, 100);
    m_backupListCtrl->AppendColumn(L"创建时间", wxLIST_FORMAT_LEFT, 140);
    m_backupListCtrl->AppendColumn(L"大小", wxLIST_FORMAT_RIGHT, 80);
    m_backupListCtrl->AppendColumn(L"可还原", wxLIST_FORMAT_CENTER, 60);

    m_backupListCtrl->Bind(wxEVT_LIST_ITEM_SELECTED, &BackupPanel::OnItemSelected, this);
    m_backupListCtrl->Bind(wxEVT_LIST_ITEM_DESELECTED, &BackupPanel::OnItemDeselected, this);

    mainSizer->Add(m_backupListCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 底部按钮栏
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    m_restoreButton = new wxButton(this, wxID_ANY, L"还原", wxDefaultPosition, wxSize(80, 36));
    m_restoreButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_restoreButton->SetBackgroundColour(colors.accent);
    m_restoreButton->SetForegroundColour(*wxWHITE);
    m_restoreButton->Enable(false);
    m_restoreButton->Bind(wxEVT_BUTTON, &BackupPanel::OnRestore, this);
    bottomSizer->Add(m_restoreButton, 0, wxRIGHT, 8);

    m_deleteButton = new wxButton(this, wxID_ANY, L"删除备份", wxDefaultPosition, wxSize(100, 36));
    m_deleteButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_deleteButton->Enable(false);
    m_deleteButton->Bind(wxEVT_BUTTON, &BackupPanel::OnDelete, this);
    bottomSizer->Add(m_deleteButton, 0, wxRIGHT, 8);

    m_cleanupButton = new wxButton(this, wxID_ANY, L"清理旧备份", wxDefaultPosition, wxSize(100, 36));
    m_cleanupButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_cleanupButton->Bind(wxEVT_BUTTON, &BackupPanel::OnCleanup, this);
    bottomSizer->Add(m_cleanupButton, 0, wxRIGHT, 8);

    m_refreshButton = new wxButton(this, wxID_ANY, L"刷新", wxDefaultPosition, wxSize(70, 36));
    m_refreshButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_refreshButton->Bind(wxEVT_BUTTON, &BackupPanel::OnRefresh, this);
    bottomSizer->Add(m_refreshButton, 0, wxRIGHT, 8);

    bottomSizer->AddStretchSpacer();

    m_statusLabel = new wxStaticText(this, wxID_ANY, L"");
    m_statusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_statusLabel->SetForegroundColour(colors.textSecondary);
    bottomSizer->Add(m_statusLabel, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);

    CallAfter([this]() { RefreshBackupList(); });
}

void BackupPanel::RefreshBackupList() {
    IceClean::Core::Safety::SystemBackupManager mgr;
    m_backupList = mgr.GetBackupList();
    PopulateList();
    UpdateStatus();
}

void BackupPanel::PopulateList() {
    m_backupListCtrl->DeleteAllItems();

    for (int i = 0; i < static_cast<int>(m_backupList.size()); ++i) {
        const auto& record = m_backupList[i];
        long idx = m_backupListCtrl->InsertItem(i, record.description);
        m_backupListCtrl->SetItem(idx, 1, GetBackupTypeName(record.type));
        m_backupListCtrl->SetItem(idx, 2, FormatTime(record.createTime));
        m_backupListCtrl->SetItem(idx, 3, FormatSize(record.backupSize));
        m_backupListCtrl->SetItem(idx, 4, record.canRestore ? L"是" : L"否");
        m_backupListCtrl->SetItemData(idx, i);
    }
}

void BackupPanel::OnBackupRegistry(wxCommandEvent& event) {
    IceClean::Core::Safety::SystemBackupManager mgr;
    m_statusLabel->SetLabelText(L"正在备份注册表...");

    std::thread([this]() {
        IceClean::Core::Safety::SystemBackupManager mgr;
        std::vector<std::wstring> paths = {
            L"HKEY_LOCAL_MACHINE\\SOFTWARE",
            L"HKEY_CURRENT_USER\\SOFTWARE",
        };
        bool ok = mgr.BackupRegistry(L"注册表备份", paths);

        CallAfter([this, ok]() {
            if (ok) {
                m_statusLabel->SetLabelText(L"注册表备份成功！");
                RefreshBackupList();
            } else {
                m_statusLabel->SetLabelText(L"注册表备份失败。");
            }
        });
    }).detach();
}

void BackupPanel::OnBackupStartup(wxCommandEvent& event) {
    IceClean::Core::Safety::SystemBackupManager mgr;
    m_statusLabel->SetLabelText(L"正在备份启动项...");

    std::thread([this]() {
        IceClean::Core::Safety::SystemBackupManager mgr;
        bool ok = mgr.BackupStartupItems(L"启动项备份");

        CallAfter([this, ok]() {
            if (ok) {
                m_statusLabel->SetLabelText(L"启动项备份成功！");
                RefreshBackupList();
            } else {
                m_statusLabel->SetLabelText(L"启动项备份失败。");
            }
        });
    }).detach();
}

void BackupPanel::OnBackupFull(wxCommandEvent& event) {
    IceClean::Core::Safety::SystemBackupManager mgr;
    m_statusLabel->SetLabelText(L"正在创建完整注册表备份(可能需要几分钟)...");

    std::thread([this]() {
        IceClean::Core::Safety::SystemBackupManager mgr;
        bool ok = mgr.BackupFullRegistry(L"完整注册表备份");

        CallAfter([this, ok]() {
            if (ok) {
                m_statusLabel->SetLabelText(L"完整注册表备份成功！");
                RefreshBackupList();
            } else {
                m_statusLabel->SetLabelText(L"完整注册表备份失败。");
            }
        });
    }).detach();
}

void BackupPanel::OnRestore(wxCommandEvent& event) {
    long sel = m_backupListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_backupList.size())) return;

    const auto& record = m_backupList[sel];

    ConfirmDialog dlg(this, L"还原备份",
        wxString::Format(L"确定要还原备份 \"%s\" 吗？\n\n"
            L"还原注册表将覆盖当前设置，请确保这是您想要的操作。", record.description.c_str()),
        ConfirmDialog::DangerLevel::Dangerous, L"还原", L"取消");

    if (dlg.ShowModal() != wxID_OK) return;

    IceClean::Core::Safety::SystemBackupManager mgr;
    if (mgr.RestoreRegistry(record)) {
        wxMessageBox(L"备份还原成功！部分更改可能需要重启系统生效。", L"IceClean", wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(L"备份还原失败，请尝试以管理员权限运行。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void BackupPanel::OnDelete(wxCommandEvent& event) {
    long sel = m_backupListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_backupList.size())) return;

    const auto& record = m_backupList[sel];

    ConfirmDialog dlg(this, L"删除备份",
        wxString::Format(L"确定要删除备份 \"%s\" 吗？\n\n此操作不可恢复。", record.description.c_str()),
        ConfirmDialog::DangerLevel::Caution, L"删除", L"取消");

    if (dlg.ShowModal() != wxID_OK) return;

    IceClean::Core::Safety::SystemBackupManager mgr;
    if (mgr.DeleteBackup(record.backupId)) {
        RefreshBackupList();
    } else {
        wxMessageBox(L"删除备份失败。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void BackupPanel::OnCleanup(wxCommandEvent& event) {
    IceClean::Core::Safety::SystemBackupManager mgr;
    int deleted = mgr.CleanupOldBackups(10);
    RefreshBackupList();
    wxString msg = wxString::Format(L"已清理 %d 个旧备份。", deleted);
    wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
}

void BackupPanel::OnRefresh(wxCommandEvent& event) {
    RefreshBackupList();
}

void BackupPanel::OnItemSelected(wxListEvent& event) {
    m_restoreButton->Enable(true);
    m_deleteButton->Enable(true);
}

void BackupPanel::OnItemDeselected(wxListEvent& event) {
    m_restoreButton->Enable(false);
    m_deleteButton->Enable(false);
}

void BackupPanel::UpdateStatus() {
    m_statusLabel->SetLabelText(
        wxString::Format(L"共 %d 个备份", static_cast<int>(m_backupList.size())));

    IceClean::Core::Safety::SystemBackupManager mgr;
    uint64_t totalSize = mgr.GetTotalBackupSize();
    m_totalSizeLabel->SetLabelText(
        wxString::Format(L"备份总大小: %s", FormatSize(totalSize).c_str()));
}

wxString BackupPanel::GetBackupTypeName(IceClean::Models::BackupType type) const {
    switch (type) {
        case IceClean::Models::BackupType::RegistryBackup: return L"注册表";
        case IceClean::Models::BackupType::DriverBackup: return L"驱动";
        case IceClean::Models::BackupType::SystemStateBackup: return L"系统状态";
        case IceClean::Models::BackupType::StartupBackup: return L"启动项";
        default: return L"未知";
    }
}

wxString BackupPanel::FormatSize(uint64_t bytes) const {
    return IceClean::Utils::FormatUtil::FormatFileSize(bytes);
}

wxString BackupPanel::FormatTime(std::chrono::system_clock::time_point time) const {
    auto time_t_val = std::chrono::system_clock::to_time_t(time);
    struct tm tm_val;
    localtime_s(&tm_val, &time_t_val);
    wchar_t buf[32] = {};
    wcsftime(buf, 32, L"%Y-%m-%d %H:%M", &tm_val);
    return buf;
}

} // namespace IceClean::Gui
