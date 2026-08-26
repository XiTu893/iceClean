#include "DriverPanel.h"
#include "core/driver/DeviceDriverScanner.h"
#include "core/driver/PnpUtilRunner.h"
#include "core/driver/DriverBackupManager.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/controls/ThemeManager.h"
#include "utils/FormatUtil.h"
#include <algorithm>
#include <thread>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(DriverPanel, wxPanel)
wxEND_EVENT_TABLE()

// ───────────────────────── DriverPanel 容器 ─────────────────────────
DriverPanel::DriverPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id) {
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();
}

void DriverPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(12);

    auto* title = new wxStaticText(this, wxID_ANY, L"驱动管理");
    title->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    title->SetForegroundColour(colors.textPrimary);
    sizer->Add(title, 0, wxLEFT | wxRIGHT, 20);
    sizer->AddSpacer(4);

    auto* tip = new wxStaticText(this, wxID_ANY,
        L"查看已安装设备与驱动，支持驱动备份与还原。");
    tip->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    tip->SetForegroundColour(colors.textSecondary);
    sizer->Add(tip, 0, wxLEFT | wxRIGHT, 20);
    sizer->AddSpacer(8);

    m_notebook = new wxNotebook(this, wxID_ANY);
    m_devicePage = new DeviceDriverPage(m_notebook);
    m_packagePage = new PackageDriverPage(m_notebook);
    m_notebook->AddPage(m_devicePage, L"设备 & 驱动");
    m_notebook->AddPage(m_packagePage, L"驱动包");
    sizer->Add(m_notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    sizer->AddSpacer(12);

    SetSizer(sizer);
}

// ───────────────────────── 设备 & 驱动页 ─────────────────────────
DeviceDriverPage::DeviceDriverPage(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id) {
    CreateControls();
    CallAfter([this]() { RefreshList(); });
}

void DeviceDriverPage::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
    toolbar->AddSpacer(8);

    m_search = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(220, 30));
    m_search->SetDescriptiveText(L"搜索设备/驱动...");
    m_search->Bind(wxEVT_TEXT, &DeviceDriverPage::OnSearch, this);
    toolbar->Add(m_search, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_filter = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(120, 30));
    m_filter->Append(L"全部设备");
    m_filter->Append(L"异常设备");
    m_filter->Append(L"第三方驱动");
    m_filter->Append(L"微软驱动");
    m_filter->SetSelection(0);
    m_filter->Bind(wxEVT_CHOICE, &DeviceDriverPage::OnFilter, this);
    toolbar->Add(m_filter, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_refresh = new wxButton(this, wxID_ANY, L"刷新", wxDefaultPosition, wxSize(70, 30));
    m_refresh->Bind(wxEVT_BUTTON, &DeviceDriverPage::OnRefresh, this);
    toolbar->Add(m_refresh, 0, wxALIGN_CENTER_VERTICAL);

    toolbar->AddStretchSpacer();
    m_status = new wxStaticText(this, wxID_ANY, L"");
    m_status->SetForegroundColour(colors.textSecondary);
    toolbar->Add(m_status, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    sizer->Add(toolbar, 0, wxEXPAND);

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_list->AppendColumn(L"设备名称", wxLIST_FORMAT_LEFT, 220);
    m_list->AppendColumn(L"厂商", wxLIST_FORMAT_LEFT, 120);
    m_list->AppendColumn(L"类别", wxLIST_FORMAT_LEFT, 90);
    m_list->AppendColumn(L"驱动版本", wxLIST_FORMAT_LEFT, 90);
    m_list->AppendColumn(L"日期", wxLIST_FORMAT_LEFT, 90);
    m_list->AppendColumn(L"状态", wxLIST_FORMAT_LEFT, 80);
    m_list->AppendColumn(L"签名", wxLIST_FORMAT_LEFT, 80);
    sizer->Add(m_list, 1, wxEXPAND | wxALL, 8);

    SetSizer(sizer);
}

void DeviceDriverPage::RefreshList() {
    m_status->SetLabelText(L"正在加载设备与驱动...");
    m_refresh->Enable(false);
    std::thread([this]() {
        auto list = Core::Driver::DeviceDriverScanner::Enumerate();
        CallAfter([this, list = std::move(list)]() mutable {
            m_all = std::move(list);
            m_refresh->Enable(true);
            ApplyFilterAndSearch();
        });
    }).detach();
}

void DeviceDriverPage::Populate(const std::vector<Core::Driver::DeviceDriverInfo>& items) {
    m_list->DeleteAllItems();
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const auto& d = items[i];
        const auto name = d.deviceName.empty() ? d.driverDesc : d.deviceName;
        long idx = m_list->InsertItem(i, name);
        m_list->SetItem(idx, 1, d.manufacturer);
        m_list->SetItem(idx, 2, d.deviceClass);
        m_list->SetItem(idx, 3, d.version);
        m_list->SetItem(idx, 4, d.date);
        m_list->SetItem(idx, 5, d.statusText);
        m_list->SetItem(idx, 6, Core::Driver::SignatureToText(d.signature));
        m_list->SetItemData(idx, i);
    }
    m_status->SetLabelText(
        wxString::Format(L"共 %d 个设备/驱动", static_cast<int>(items.size())));
}

void DeviceDriverPage::OnSearch(wxCommandEvent& event) {
    ApplyFilterAndSearch();
}

void DeviceDriverPage::OnFilter(wxCommandEvent& event) {
    ApplyFilterAndSearch();
}

void DeviceDriverPage::OnRefresh(wxCommandEvent& event) {
    RefreshList();
}

// 过滤 + 搜索合并处理
void DeviceDriverPage::ApplyFilterAndSearch() {
    const int sel = m_filter->GetSelection();
    const wxString kw = m_search->GetValue().Lower();
    std::vector<Core::Driver::DeviceDriverInfo> out;
    for (const auto& d : m_all) {
        const std::wstring name = d.deviceName.empty() ? d.driverDesc : d.deviceName;
        switch (sel) {
        case 1: if (d.statusText == L"正常") continue; break;
        case 2: if (!d.isThirdParty) continue; break;
        case 3: if (d.isThirdParty) continue; break;
        default: break;
        }
        if (!kw.empty()) {
            std::wstring hay = name + L" " + d.driverDesc + L" " + d.manufacturer +
                               L" " + d.provider;
            std::transform(hay.begin(), hay.end(), hay.begin(), ::towlower);
            if (hay.find(kw.ToStdWstring()) == std::wstring::npos) continue;
        }
        out.push_back(d);
    }
    Populate(out);
}

// ───────────────────────── 驱动包页 ─────────────────────────
PackageDriverPage::PackageDriverPage(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id) {
    CreateControls();
    CallAfter([this]() { RefreshList(); });
}

void PackageDriverPage::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
    toolbar->AddSpacer(8);

    m_search = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, 30));
    m_search->SetDescriptiveText(L"搜索驱动包...");
    m_search->Bind(wxEVT_TEXT, &PackageDriverPage::OnSearch, this);
    toolbar->Add(m_search, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_filter = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(120, 30));
    m_filter->Append(L"全部驱动");
    m_filter->Append(L"第三方驱动");
    m_filter->Append(L"系统驱动");
    m_filter->SetSelection(0);
    m_filter->Bind(wxEVT_CHOICE, &PackageDriverPage::OnFilter, this);
    toolbar->Add(m_filter, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_refresh = new wxButton(this, wxID_ANY, L"刷新", wxDefaultPosition, wxSize(70, 30));
    m_refresh->Bind(wxEVT_BUTTON, &PackageDriverPage::OnRefresh, this);
    toolbar->Add(m_refresh, 0, wxALIGN_CENTER_VERTICAL);

    toolbar->AddStretchSpacer();
    m_ticker = new wxStaticText(this, wxID_ANY, L"");
    m_ticker->SetForegroundColour(colors.textSecondary);
    toolbar->Add(m_ticker, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    sizer->Add(toolbar, 0, wxEXPAND);

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_list->AppendColumn(L"名称", wxLIST_FORMAT_LEFT, 200);
    m_list->AppendColumn(L"提供商", wxLIST_FORMAT_LEFT, 120);
    m_list->AppendColumn(L"类别", wxLIST_FORMAT_LEFT, 90);
    m_list->AppendColumn(L"版本", wxLIST_FORMAT_LEFT, 90);
    m_list->AppendColumn(L"日期", wxLIST_FORMAT_LEFT, 90);
    m_list->AppendColumn(L"oem.inf", wxLIST_FORMAT_LEFT, 100);
    m_list->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent&) { m_backupSelected->Enable(true); });
    m_list->Bind(wxEVT_LIST_ITEM_DESELECTED, [this](wxListEvent&) { m_backupSelected->Enable(false); });
    m_list->Bind(wxEVT_LIST_COL_CLICK, [this](wxListEvent& e) {
        const int col = e.GetColumn();
        if (m_sortColumn == col) m_sortAsc = !m_sortAsc;
        else { m_sortColumn = col; m_sortAsc = true; }
        std::sort(m_filtered.begin(), m_filtered.end(),
                  [this, col](const auto& a, const auto& b) {
                      std::wstring na = col == 0 ? (a.originalName.empty() ? a.publishedName : a.originalName)
                                                 : a.provider;
                      std::wstring nb = col == 0 ? (b.originalName.empty() ? b.publishedName : b.originalName)
                                                 : b.provider;
                      if (col == 1) { na = a.provider; nb = b.provider; }
                      else if (col == 2) { na = a.className; nb = b.className; }
                      else if (col == 3) { na = a.version; nb = b.version; }
                      else if (col == 4) { na = a.date; nb = b.date; }
                      else if (col == 5) { na = a.publishedName; nb = b.publishedName; }
                      const bool less = _wcsicmp(na.c_str(), nb.c_str()) < 0;
                      return m_sortAsc ? less : !less;
                  });
        Populate(m_filtered);
    });
    sizer->Add(m_list, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    sizer->AddSpacer(8);

    auto* bottom = new wxBoxSizer(wxHORIZONTAL);
    bottom->AddSpacer(8);
    m_backupAll = new wxButton(this, wxID_ANY, L"备份所有第三方驱动", wxDefaultPosition, wxSize(160, 36));
    m_backupAll->SetBackgroundColour(colors.accent);
    m_backupAll->SetForegroundColour(*wxWHITE);
    m_backupAll->Bind(wxEVT_BUTTON, &PackageDriverPage::OnBackupAll, this);
    bottom->Add(m_backupAll, 0, wxRIGHT, 8);

    m_backupSelected = new wxButton(this, wxID_ANY, L"备份选中驱动", wxDefaultPosition, wxSize(120, 36));
    m_backupSelected->Enable(false);
    m_backupSelected->Bind(wxEVT_BUTTON, &PackageDriverPage::OnBackupSelected, this);
    bottom->Add(m_backupSelected, 0, wxRIGHT, 8);

    m_restore = new wxButton(this, wxID_ANY, L"从备份还原", wxDefaultPosition, wxSize(120, 36));
    m_restore->Bind(wxEVT_BUTTON, &PackageDriverPage::OnRestore, this);
    bottom->Add(m_restore, 0, wxRIGHT, 8);

    m_cleanup = new wxButton(this, wxID_ANY, L"清理旧驱动", wxDefaultPosition, wxSize(120, 36));
    m_cleanup->Bind(wxEVT_BUTTON, &PackageDriverPage::OnCleanup, this);
    bottom->Add(m_cleanup, 0, wxRIGHT, 8);

    bottom->AddStretchSpacer();
    m_status = new wxStaticText(this, wxID_ANY, L"");
    m_status->SetForegroundColour(colors.textSecondary);
    bottom->Add(m_status, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(bottom, 0, wxEXPAND);
    sizer->AddSpacer(12);

    SetSizer(sizer);
}

void PackageDriverPage::RefreshList() {
    m_status->SetLabelText(L"正在加载驱动包...");
    m_refresh->Enable(false);
    m_backupAll->Enable(false);
    std::thread([this]() {
        auto list = Core::Driver::PnpUtilRunner::EnumDrivers();
        CallAfter([this, list = std::move(list)]() mutable {
            m_all = std::move(list);
            m_refresh->Enable(true);
            m_backupAll->Enable(true);
            ApplyFilterAndSearch();
        });
    }).detach();
}

void PackageDriverPage::Populate(const std::vector<Core::Driver::DriverPackageInfo>& items) {
    m_list->DeleteAllItems();
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const auto& p = items[i];
        const auto name = p.originalName.empty() ? p.publishedName : p.originalName;
        long idx = m_list->InsertItem(i, name);
        m_list->SetItem(idx, 1, p.provider);
        m_list->SetItem(idx, 2, p.className);
        m_list->SetItem(idx, 3, p.version);
        m_list->SetItem(idx, 4, p.date);
        m_list->SetItem(idx, 5, p.publishedName);
        m_list->SetItemData(idx, i);
    }
    m_status->SetLabelText(
        wxString::Format(L"共 %d 个驱动包", static_cast<int>(items.size())));
}

void PackageDriverPage::OnSearch(wxCommandEvent& event) { ApplyFilterAndSearch(); }
void PackageDriverPage::OnFilter(wxCommandEvent& event) { ApplyFilterAndSearch(); }
void PackageDriverPage::OnRefresh(wxCommandEvent& event) { RefreshList(); }

void PackageDriverPage::ApplyFilterAndSearch() {
    const int sel = m_filter->GetSelection();
    const wxString kw = m_search->GetValue().Lower();
    std::vector<Core::Driver::DriverPackageInfo> out;
    for (const auto& p : m_all) {
        switch (sel) {
        case 1: if (!p.provider.empty() &&
                    p.provider.find(L"Microsoft") == std::wstring::npos) { /*第三方*/ }
                else continue; break;
        case 2: if (p.provider.find(L"Microsoft") == std::wstring::npos) continue; break;
        default: break;
        }
        if (!kw.empty()) {
            std::wstring hay = (p.originalName.empty() ? p.publishedName : p.originalName) +
                               L" " + p.provider + L" " + p.className;
            std::transform(hay.begin(), hay.end(), hay.begin(), ::towlower);
            if (hay.find(kw.ToStdWstring()) == std::wstring::npos) continue;
        }
        out.push_back(p);
    }
    m_filtered = out;
    Populate(out);
}

Core::Driver::DriverPackageInfo PackageDriverPage::GetSelected() const {
    long sel = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_filtered.size())) {
        return Core::Driver::DriverPackageInfo{};
    }
    return m_filtered[sel];
}

void PackageDriverPage::OnBackupAll(wxCommandEvent& event) {
    const auto root = Core::Driver::DriverBackupManager::GetDefaultBackupRoot();
    wxDirDialog dlg(this, L"选择驱动备份保存目录", root, wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    const auto backupDir = dlg.GetPath().ToStdWstring();
    m_status->SetLabelText(L"正在备份驱动...");
    m_ticker->SetLabelText(L"");
    m_backupAll->Enable(false);
    m_backupSelected->Enable(false);

    std::thread([this, backupDir]() {
        const auto result = Core::Driver::DriverBackupManager::BackupAll(
            backupDir, [this](int cur, int total, const std::wstring& name) {
                CallAfter([this, cur, total, name]() {
                    m_ticker->SetLabelText(wxString::Format(
                        L"已导出 %d 个文件 · %s", cur, name.c_str()));
                });
            });
        CallAfter([this, result]() {
            m_backupAll->Enable(true);
            if (result.success) {
                wxMessageBox(wxString::Format(
                    L"驱动备份完成！成功导出 %d 个驱动包到:\n%s",
                    result.packageCount, result.backupDir.c_str()),
                    L"IceClean", wxOK | wxICON_INFORMATION, this);
            } else {
                wxMessageBox(L"驱动备份失败，请确认有足够磁盘空间与管理员权限。",
                             L"IceClean", wxOK | wxICON_WARNING, this);
            }
            m_status->SetLabelText(L"");
            m_ticker->SetLabelText(L"");
        });
    }).detach();
}

void PackageDriverPage::OnBackupSelected(wxCommandEvent& event) {
    const auto pkg = GetSelected();
    if (pkg.publishedName.empty()) return;

    const auto root = Core::Driver::DriverBackupManager::GetDefaultBackupRoot();
    wxDirDialog dlg(this, L"选择驱动备份保存目录", root, wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    namespace fs = std::filesystem;
    const auto dest = dlg.GetPath().ToStdWstring() + L"\\" + pkg.publishedName;
    std::error_code ec;
    fs::create_directories(dest, ec);

    m_status->SetLabelText(L"正在备份选中驱动...");
    std::thread([this, pkg, dest]() {
        const bool ok = Core::Driver::PnpUtilRunner::ExportOne(pkg.publishedName, dest);
        CallAfter([this, ok, pkg]() {
            m_status->SetLabelText(L"");
            wxMessageBox(ok ? L"驱动备份成功！" : L"驱动备份失败，请确认权限。",
                         L"IceClean", wxOK | (ok ? wxICON_INFORMATION : wxICON_WARNING), this);
        });
    }).detach();
}

void PackageDriverPage::OnRestore(wxCommandEvent& event) {
    wxDirDialog dlg(this, L"选择历史备份目录（含 manifest.json）",
                    Core::Driver::DriverBackupManager::GetDefaultBackupRoot(),
                    wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    const auto dir = dlg.GetPath().ToStdWstring();
    if (!std::filesystem::exists(dir + L"\\manifest.json")) {
        wxMessageBox(L"所选目录不是有效的驱动备份（缺少 manifest.json）。",
                     L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    ConfirmDialog cd(this, L"从备份还原驱动",
        L"即将把所选备份中的驱动包导入并安装到当前系统。\n\n"
        L"此操作会创建系统还原点，并在安装前尽量保证可回滚。\n"
        L"是否继续？",
        ConfirmDialog::DangerLevel::Caution, L"还原", L"取消");
    if (cd.ShowModal() != wxID_OK) return;

    m_status->SetLabelText(L"正在还原驱动（已创建还原点）...");
    m_restore->Enable(false);
    std::thread([this, dir]() {
        const bool ok = Core::Driver::DriverBackupManager::RestoreFrom(dir);
        CallAfter([this, ok]() {
            m_restore->Enable(true);
            m_status->SetLabelText(L"");
            wxMessageBox(ok ? L"驱动还原完成！" : L"驱动还原失败，请查看操作日志。",
                         L"IceClean", wxOK | (ok ? wxICON_INFORMATION : wxICON_WARNING), this);
        });
    }).detach();
}

void PackageDriverPage::OnCleanup(wxCommandEvent& event) {
    ConfirmDialog cd(this, L"清理旧驱动",
        L"确定要清理 Windows 驱动存储中不再使用的旧驱动包吗？\n\n"
        L"此操作通过系统组件清理释放磁盘空间，当前正在使用的驱动不受影响。",
        ConfirmDialog::DangerLevel::Caution, L"清理", L"取消");
    if (cd.ShowModal() != wxID_OK) return;

    m_status->SetLabelText(L"正在清理旧驱动...");
    m_cleanup->Enable(false);
    std::thread([this]() {
        const auto freed = Core::Driver::DriverBackupManager::CleanupOldBackups();
        CallAfter([this, freed]() {
            m_cleanup->Enable(true);
            m_status->SetLabelText(L"");
            wxMessageBox(wxString::Format(L"清理完成！释放了 %s 磁盘空间。",
                             IceClean::Utils::FormatUtil::FormatFileSize(freed).c_str()),
                         L"IceClean", wxOK | wxICON_INFORMATION, this);
        });
    }).detach();
}

} // namespace IceClean::Gui
