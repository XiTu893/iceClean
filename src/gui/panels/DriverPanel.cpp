#include "DriverPanel.h"
#include "core/optimizer/DriverManager.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/controls/ThemeManager.h"
#include "utils/FormatUtil.h"
#include <algorithm>
#include <thread>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(DriverPanel, wxPanel)
wxEND_EVENT_TABLE()

DriverPanel::DriverPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();
}

void DriverPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"驱动管理");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* tipLabel = new wxStaticText(this, wxID_ANY,
        L"查看和管理系统驱动程序，支持驱动备份和旧驱动清理。");
    tipLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    tipLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(tipLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 工具栏
    auto* toolbarSizer = new wxBoxSizer(wxHORIZONTAL);
    toolbarSizer->AddSpacer(20);

    m_searchCtrl = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, 30));
    m_searchCtrl->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_searchCtrl->SetDescriptiveText(L"搜索驱动...");
    m_searchCtrl->Bind(wxEVT_TEXT, &DriverPanel::OnSearch, this);
    toolbarSizer->Add(m_searchCtrl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    // 过滤选择
    m_filterChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(120, 30));
    m_filterChoice->Append(L"全部驱动");
    m_filterChoice->Append(L"第三方驱动");
    m_filterChoice->Append(L"系统驱动");
    m_filterChoice->Append(L"可能过时");
    m_filterChoice->SetSelection(0);
    m_filterChoice->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_filterChoice->Bind(wxEVT_CHOICE, &DriverPanel::OnFilterChange, this);
    toolbarSizer->Add(m_filterChoice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_refreshButton = new wxButton(this, wxID_ANY, L"刷新", wxDefaultPosition, wxSize(70, 30));
    m_refreshButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_refreshButton->Bind(wxEVT_BUTTON, &DriverPanel::OnRefresh, this);
    toolbarSizer->Add(m_refreshButton, 0, wxALIGN_CENTER_VERTICAL);

    toolbarSizer->AddStretchSpacer();

    m_totalSizeLabel = new wxStaticText(this, wxID_ANY, L"");
    m_totalSizeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_totalSizeLabel->SetForegroundColour(colors.textSecondary);
    toolbarSizer->Add(m_totalSizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    mainSizer->Add(toolbarSizer, 0, wxEXPAND);

    // 驱动列表
    m_driverListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_driverListCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));

    m_driverListCtrl->AppendColumn(L"设备名称", wxLIST_FORMAT_LEFT, 220);
    m_driverListCtrl->AppendColumn(L"提供商", wxLIST_FORMAT_LEFT, 120);
    m_driverListCtrl->AppendColumn(L"版本", wxLIST_FORMAT_LEFT, 90);
    m_driverListCtrl->AppendColumn(L"日期", wxLIST_FORMAT_LEFT, 90);
    m_driverListCtrl->AppendColumn(L"状态", wxLIST_FORMAT_LEFT, 70);

    m_driverListCtrl->Bind(wxEVT_LIST_ITEM_SELECTED, &DriverPanel::OnItemSelected, this);
    m_driverListCtrl->Bind(wxEVT_LIST_ITEM_DESELECTED, &DriverPanel::OnItemDeselected, this);
    m_driverListCtrl->Bind(wxEVT_LIST_COL_CLICK, &DriverPanel::OnColumnClick, this);

    mainSizer->Add(m_driverListCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 底部按钮栏
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    m_backupAllButton = new wxButton(this, wxID_ANY, L"备份所有第三方驱动", wxDefaultPosition, wxSize(160, 36));
    m_backupAllButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_backupAllButton->SetBackgroundColour(colors.accent);
    m_backupAllButton->SetForegroundColour(*wxWHITE);
    m_backupAllButton->Bind(wxEVT_BUTTON, &DriverPanel::OnBackupAll, this);
    bottomSizer->Add(m_backupAllButton, 0, wxRIGHT, 8);

    m_backupSelectedButton = new wxButton(this, wxID_ANY, L"备份选中驱动", wxDefaultPosition, wxSize(120, 36));
    m_backupSelectedButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_backupSelectedButton->Enable(false);
    m_backupSelectedButton->Bind(wxEVT_BUTTON, &DriverPanel::OnBackupSelected, this);
    bottomSizer->Add(m_backupSelectedButton, 0, wxRIGHT, 8);

    m_cleanupButton = new wxButton(this, wxID_ANY, L"清理旧驱动备份", wxDefaultPosition, wxSize(130, 36));
    m_cleanupButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_cleanupButton->Bind(wxEVT_BUTTON, &DriverPanel::OnCleanup, this);
    bottomSizer->Add(m_cleanupButton, 0, wxRIGHT, 8);

    bottomSizer->AddStretchSpacer();

    m_statusLabel = new wxStaticText(this, wxID_ANY, L"");
    m_statusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_statusLabel->SetForegroundColour(colors.textSecondary);
    bottomSizer->Add(m_statusLabel, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);

    CallAfter([this]() { RefreshDriverList(); });
}

void DriverPanel::RefreshDriverList() {
    m_statusLabel->SetLabelText(L"正在加载驱动列表...");
    m_refreshButton->Enable(false);

    std::thread([this]() {
        IceClean::Core::Optimizer::DriverManager driverMgr;
        auto drivers = driverMgr.GetDrivers();

        CallAfter([this, drivers = std::move(drivers)]() mutable {
            m_driverList = std::move(drivers);
            PopulateList(m_driverList);
            m_refreshButton->Enable(true);
            UpdateStatus();
        });
    }).detach();
}

void DriverPanel::PopulateList(const std::vector<IceClean::Models::DriverInfo>& items) {
    m_driverListCtrl->DeleteAllItems();
    m_filteredList = items;

    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const auto& drv = items[i];
        long idx = m_driverListCtrl->InsertItem(i, drv.deviceName.empty() ? drv.driverDesc : drv.deviceName);
        m_driverListCtrl->SetItem(idx, 1, drv.driverProvider);
        m_driverListCtrl->SetItem(idx, 2, drv.driverVersion);
        m_driverListCtrl->SetItem(idx, 3, drv.driverDate);

        wxString status;
        if (drv.hasUpdate) status = L"可更新";
        else if (drv.isSystemDriver) status = L"系统";
        else status = L"正常";
        m_driverListCtrl->SetItem(idx, 4, status);

        m_driverListCtrl->SetItemData(idx, i);
    }
}

void DriverPanel::OnSearch(wxCommandEvent& event) {
    wxString keyword = m_searchCtrl->GetValue().Lower();
    if (keyword.empty()) {
        PopulateList(m_driverList);
        return;
    }

    std::vector<IceClean::Models::DriverInfo> filtered;
    for (const auto& drv : m_driverList) {
        std::wstring name = drv.deviceName.empty() ? drv.driverDesc : drv.deviceName;
        std::wstring prov = drv.driverProvider;
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        std::transform(prov.begin(), prov.end(), prov.begin(), ::towlower);
        std::wstring kw = keyword.ToStdWstring();
        std::transform(kw.begin(), kw.end(), kw.begin(), ::towlower);

        if (name.find(kw) != std::wstring::npos || prov.find(kw) != std::wstring::npos) {
            filtered.push_back(drv);
        }
    }
    PopulateList(filtered);
}

void DriverPanel::OnRefresh(wxCommandEvent& event) {
    RefreshDriverList();
}

void DriverPanel::OnBackupAll(wxCommandEvent& event) {
    wxDirDialog dlg(this, L"选择驱动备份保存目录", L"", wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    auto backupDir = dlg.GetPath().ToStdWstring();
    m_statusLabel->SetLabelText(L"正在备份驱动...");
    m_backupAllButton->Enable(false);

    std::thread([this, backupDir]() {
        IceClean::Core::Optimizer::DriverManager driverMgr;
        int count = driverMgr.BackupAllDrivers(backupDir,
            [this](int current, int total, const std::wstring& name) {
                CallAfter([this, current, total, name]() {
                    m_statusLabel->SetLabelText(
                        wxString::Format(L"正在备份: %s (%d/%d)", name.c_str(), current, total));
                });
            });

        CallAfter([this, count, backupDir]() {
            m_backupAllButton->Enable(true);
            wxString msg = wxString::Format(L"驱动备份完成！成功备份 %d 个驱动到:\n%s", count, backupDir.c_str());
            wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
            m_statusLabel->SetLabelText(L"");
        });
    }).detach();
}

void DriverPanel::OnBackupSelected(wxCommandEvent& event) {
    auto driver = GetSelectedDriver();
    if (driver.deviceName.empty() && driver.driverDesc.empty()) return;

    wxDirDialog dlg(this, L"选择驱动备份保存目录", L"", wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    auto backupDir = dlg.GetPath().ToStdWstring();
    IceClean::Core::Optimizer::DriverManager driverMgr;
    if (driverMgr.BackupDriver(driver, backupDir)) {
        wxMessageBox(L"驱动备份成功！", L"IceClean", wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(L"驱动备份失败，请确保有足够的磁盘空间和权限。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void DriverPanel::OnCleanup(wxCommandEvent& event) {
    ConfirmDialog dlg(this, L"清理旧驱动备份",
        L"确定要清理Windows驱动存储中的旧驱动备份吗？\n\n"
        L"此操作将删除不再使用的旧版本驱动文件，释放磁盘空间。\n"
        L"当前正在使用的驱动不受影响。",
        ConfirmDialog::DangerLevel::Caution, L"清理", L"取消");

    if (dlg.ShowModal() != wxID_OK) return;

    m_statusLabel->SetLabelText(L"正在清理旧驱动备份...");
    m_cleanupButton->Enable(false);

    std::thread([this]() {
        IceClean::Core::Optimizer::DriverManager driverMgr;
        auto freed = driverMgr.CleanupOldDriverBackups();

        CallAfter([this, freed]() {
            m_cleanupButton->Enable(true);
            wxString msg = wxString::Format(L"清理完成！释放了 %s 磁盘空间。",
                IceClean::Utils::FormatUtil::FormatFileSize(freed).c_str());
            wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
            m_statusLabel->SetLabelText(L"");
        });
    }).detach();
}

void DriverPanel::OnFilterChange(wxCommandEvent& event) {
    int sel = m_filterChoice->GetSelection();
    std::vector<IceClean::Models::DriverInfo> filtered;

    switch (sel) {
        case 0: // 全部
            PopulateList(m_driverList);
            return;
        case 1: // 第三方
            for (const auto& drv : m_driverList) {
                if (!drv.isSystemDriver) filtered.push_back(drv);
            }
            break;
        case 2: // 系统
            for (const auto& drv : m_driverList) {
                if (drv.isSystemDriver) filtered.push_back(drv);
            }
            break;
        case 3: // 可能过时
            for (const auto& drv : m_driverList) {
                if (drv.hasUpdate) filtered.push_back(drv);
            }
            break;
    }
    PopulateList(filtered);
}

void DriverPanel::OnItemSelected(wxListEvent& event) {
    m_backupSelectedButton->Enable(true);
}

void DriverPanel::OnItemDeselected(wxListEvent& event) {
    m_backupSelectedButton->Enable(false);
}

void DriverPanel::OnColumnClick(wxListEvent& event) {
    int col = event.GetColumn();
    if (m_sortColumn == col) {
        m_sortAsc = !m_sortAsc;
    } else {
        m_sortColumn = col;
        m_sortAsc = true;
    }

    auto& items = m_filteredList;
    std::sort(items.begin(), items.end(), [this, col](const auto& a, const auto& b) {
        bool less = false;
        switch (col) {
        case 0: {
            auto na = a.deviceName.empty() ? a.driverDesc : a.deviceName;
            auto nb = b.deviceName.empty() ? b.driverDesc : b.deviceName;
            less = _wcsicmp(na.c_str(), nb.c_str()) < 0;
            break;
        }
        case 1: less = _wcsicmp(a.driverProvider.c_str(), b.driverProvider.c_str()) < 0; break;
        case 2: less = _wcsicmp(a.driverVersion.c_str(), b.driverVersion.c_str()) < 0; break;
        case 3: less = _wcsicmp(a.driverDate.c_str(), b.driverDate.c_str()) < 0; break;
        }
        return m_sortAsc ? less : !less;
    });

    PopulateList(items);
}

void DriverPanel::UpdateStatus() {
    m_statusLabel->SetLabelText(
        wxString::Format(L"共 %d 个驱动", static_cast<int>(m_driverList.size())));

    uint64_t totalSize = 0;
    for (const auto& drv : m_driverList) {
        totalSize += drv.driverSize;
    }
    m_totalSizeLabel->SetLabelText(
        wxString::Format(L"驱动存储: %s", FormatSize(totalSize).c_str()));
}

IceClean::Models::DriverInfo DriverPanel::GetSelectedDriver() const {
    long sel = m_driverListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_filteredList.size())) {
        return IceClean::Models::DriverInfo();
    }
    return m_filteredList[sel];
}

wxString DriverPanel::FormatSize(uint64_t bytes) const {
    return IceClean::Utils::FormatUtil::FormatFileSize(bytes);
}

} // namespace IceClean::Gui
