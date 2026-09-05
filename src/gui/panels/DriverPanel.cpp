#include "DriverPanel.h"
#include "core/driver/DeviceDriverScanner.h"
#include "core/driver/PnpUtilRunner.h"
#include "core/driver/DriverBackupManager.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/controls/ThemeManager.h"
#include "utils/FormatUtil.h"
#include <algorithm>
#include <thread>
#include <filesystem>
#include <map>
#include <set>
#include <shellapi.h>

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
    m_historyPage = new BackupHistoryPage(m_notebook);
    m_notebook->AddPage(m_devicePage, L"设备 & 驱动");
    m_notebook->AddPage(m_packagePage, L"驱动包");
    m_notebook->AddPage(m_historyPage, L"备份历史");
    sizer->Add(m_notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    sizer->AddSpacer(12);

    SetSizer(sizer);
}

namespace {

constexpr wxUIntPtr GROUP_ROW_FLAG = wxUIntPtr(1) << (sizeof(wxUIntPtr) * 8 - 1);

enum DVC_Col {
    DVC_CHECK = 0,
    DVC_DEVICE,
    DVC_MFR,
    DVC_CLASS,
    DVC_VER,
    DVC_DATE,
    DVC_STATUS,
    DVC_SIG,
    DVC_COL_COUNT
};

std::wstring DeviceName(const Core::Driver::DeviceDriverInfo& d) {
    return d.deviceName.empty() ? d.driverDesc : d.deviceName;
}

std::wstring SortKey(const Core::Driver::DeviceDriverInfo& d, int col) {
    std::wstring n;
    switch (col) {
    case DVC_DEVICE: n = DeviceName(d); break;
    case DVC_MFR:    n = d.manufacturer; break;
    case DVC_CLASS:  n = d.deviceClass; break;
    case DVC_VER:    n = d.version; break;
    case DVC_DATE:   n = d.date; break;
    case DVC_STATUS: n = d.statusText; break;
    case DVC_SIG:    n = Core::Driver::SignatureToText(d.signature); break;
    default:         n = DeviceName(d); break;
    }
    std::transform(n.begin(), n.end(), n.begin(), ::towlower);
    return n;
}

} // namespace

// ───────────────────────── 设备 & 驱动页（DataViewListCtrl + 勾选）─────────────────────────
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

    m_search = new wxSearchCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(200, 28));
    m_search->SetDescriptiveText(L"搜索设备/驱动...");
    m_search->Bind(wxEVT_TEXT, &DeviceDriverPage::OnSearch, this);
    toolbar->Add(m_search, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_filter = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(120, 28));
    m_filter->Append(L"全部设备");
    m_filter->Append(L"异常设备");
    m_filter->Append(L"第三方驱动");
    m_filter->Append(L"微软驱动");
    m_filter->SetSelection(0);
    m_filter->Bind(wxEVT_CHOICE, &DeviceDriverPage::OnFilter, this);
    toolbar->Add(m_filter, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_refresh = new wxButton(this, wxID_ANY, L"刷新", wxDefaultPosition, wxSize(60, 28));
    m_refresh->Bind(wxEVT_BUTTON, &DeviceDriverPage::OnRefresh, this);
    toolbar->Add(m_refresh, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_toggleGroup = new wxToggleButton(this, wxID_ANY, L"分组", wxDefaultPosition, wxSize(56, 28));
    m_toggleGroup->SetValue(true);
    m_toggleGroup->Bind(wxEVT_TOGGLEBUTTON, &DeviceDriverPage::OnToggleGroup, this);
    toolbar->Add(m_toggleGroup, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_selectAll = new wxButton(this, wxID_ANY, L"全选", wxDefaultPosition, wxSize(56, 28));
    m_selectAll->Bind(wxEVT_BUTTON, &DeviceDriverPage::OnSelectAll, this);
    toolbar->Add(m_selectAll, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_selectNone = new wxButton(this, wxID_ANY, L"取消", wxDefaultPosition, wxSize(56, 28));
    m_selectNone->Bind(wxEVT_BUTTON, &DeviceDriverPage::OnSelectNone, this);
    toolbar->Add(m_selectNone, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_selectThirdParty = new wxButton(this, wxID_ANY, L"第三方", wxDefaultPosition, wxSize(56, 28));
    m_selectThirdParty->Bind(wxEVT_BUTTON, &DeviceDriverPage::OnSelectThirdParty, this);
    toolbar->Add(m_selectThirdParty, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_backup = new wxButton(this, wxID_ANY, L"备份所选", wxDefaultPosition, wxSize(80, 28));
    m_backup->SetBackgroundColour(colors.accent);
    m_backup->SetForegroundColour(*wxWHITE);
    m_backup->Bind(wxEVT_BUTTON, &DeviceDriverPage::OnBackupSelected, this);
    toolbar->Add(m_backup, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    toolbar->AddStretchSpacer();
    m_status = new wxStaticText(this, wxID_ANY, L"");
    m_status->SetForegroundColour(colors.textSecondary);
    toolbar->Add(m_status, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    sizer->Add(toolbar, 0, wxEXPAND);

    m_view = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxDV_ROW_LINES);
    m_view->AppendToggleColumn(L"", wxDATAVIEW_CELL_ACTIVATABLE, 30, wxALIGN_CENTER);
    m_view->AppendTextColumn(L"设备名称", wxDATAVIEW_CELL_INERT, 220);
    m_view->AppendTextColumn(L"厂商", wxDATAVIEW_CELL_INERT, 120);
    m_view->AppendTextColumn(L"类别", wxDATAVIEW_CELL_INERT, 100);
    m_view->AppendTextColumn(L"驱动版本", wxDATAVIEW_CELL_INERT, 90);
    m_view->AppendTextColumn(L"日期", wxDATAVIEW_CELL_INERT, 90);
    m_view->AppendTextColumn(L"状态", wxDATAVIEW_CELL_INERT, 80);
    m_view->AppendTextColumn(L"签名", wxDATAVIEW_CELL_INERT, 80);
    for (unsigned i = 0; i < 8; ++i) {
        if (auto* col = m_view->GetColumn(i)) col->SetSortable(true);
    }

    m_view->Bind(wxEVT_DATAVIEW_COLUMN_SORTED, &DeviceDriverPage::OnHeaderSort, this);
    m_view->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &DeviceDriverPage::OnValueChanged, this);

    sizer->Add(m_view, 1, wxEXPAND | wxALL, 8);
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

// ───────────────────────── 驱动包页（pnputil，包视角）─────────────────────────
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
                      std::wstring na, nb;
                      if (col == 0) { na = a.originalName.empty() ? a.publishedName : a.originalName;
                                      nb = b.originalName.empty() ? b.publishedName : b.originalName; }
                      else if (col == 1) { na = a.provider; nb = b.provider; }
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

void PackageDriverPage::OnSearch(wxCommandEvent&) { ApplyFilterAndSearch(); }
void PackageDriverPage::OnFilter(wxCommandEvent&) { ApplyFilterAndSearch(); }
void PackageDriverPage::OnRefresh(wxCommandEvent&) { RefreshList(); }

void PackageDriverPage::ApplyFilterAndSearch() {
    const int sel = m_filter->GetSelection();
    const wxString kw = m_search->GetValue().Lower();
    std::vector<Core::Driver::DriverPackageInfo> out;
    for (const auto& p : m_all) {
        switch (sel) {
        case 1: if (p.provider.find(L"Microsoft") != std::wstring::npos) continue; break;
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

void PackageDriverPage::OnBackupAll(wxCommandEvent&) {
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

void PackageDriverPage::OnBackupSelected(wxCommandEvent&) {
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

void PackageDriverPage::OnRestore(wxCommandEvent&) {
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

void PackageDriverPage::OnCleanup(wxCommandEvent&) {
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

// ───────────────────────── 备份历史页 ─────────────────────────
BackupHistoryPage::BackupHistoryPage(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id) {
    CreateControls();
    CallAfter([this]() { Refresh(); });
}

void BackupHistoryPage::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
    toolbar->AddSpacer(8);

    m_refresh = new wxButton(this, wxID_ANY, L"刷新", wxDefaultPosition, wxSize(70, 30));
    m_refresh->Bind(wxEVT_BUTTON, &BackupHistoryPage::OnRefresh, this);
    toolbar->Add(m_refresh, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_openFolder = new wxButton(this, wxID_ANY, L"打开备份目录", wxDefaultPosition, wxSize(100, 30));
    m_openFolder->Bind(wxEVT_BUTTON, &BackupHistoryPage::OnOpenFolder, this);
    toolbar->Add(m_openFolder, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    toolbar->AddStretchSpacer();

    m_restore = new wxButton(this, wxID_ANY, L"还原选中备份", wxDefaultPosition, wxSize(110, 30));
    m_restore->SetBackgroundColour(colors.accent);
    m_restore->SetForegroundColour(*wxWHITE);
    m_restore->Bind(wxEVT_BUTTON, &BackupHistoryPage::OnRestore, this);
    toolbar->Add(m_restore, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_delete = new wxButton(this, wxID_ANY, L"删除选中备份", wxDefaultPosition, wxSize(110, 30));
    m_delete->Bind(wxEVT_BUTTON, &BackupHistoryPage::OnDelete, this);
    toolbar->Add(m_delete, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    sizer->Add(toolbar, 0, wxEXPAND);

    m_view = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxDV_ROW_LINES);
    m_view->AppendColumn(
        new wxDataViewColumn(L"备份时间", new wxDataViewTextRenderer(), 0, 160, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE));
    m_view->AppendColumn(
        new wxDataViewColumn(L"操作系统", new wxDataViewTextRenderer(), 1, 200, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE));
    m_view->AppendColumn(
        new wxDataViewColumn(L"驱动包数", new wxDataViewTextRenderer(), 2, 80, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE));
    m_view->AppendColumn(
        new wxDataViewColumn(L"总大小", new wxDataViewTextRenderer(), 3, 100, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE));
    m_view->AppendColumn(
        new wxDataViewColumn(L"包清单", new wxDataViewTextRenderer(), 4, 400, wxALIGN_LEFT, wxDATAVIEW_COL_RESIZABLE));
    m_view->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &BackupHistoryPage::OnItemActivated, this);

    sizer->Add(m_view, 1, wxEXPAND | wxALL, 8);

    m_status = new wxStaticText(this, wxID_ANY, L"");
    m_status->SetForegroundColour(colors.textSecondary);
    auto* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    statusSizer->AddSpacer(8);
    statusSizer->Add(m_status, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    sizer->Add(statusSizer, 0, wxEXPAND | wxBOTTOM, 8);

    SetSizer(sizer);
}

void BackupHistoryPage::Refresh() {
    m_status->SetLabelText(L"正在加载备份历史...");
    m_refresh->Enable(false);
    std::thread([this]() {
        const auto root = Core::Driver::DriverBackupManager::GetDefaultBackupRoot();
        auto entries = Core::Driver::DriverBackupManager::ListBackups(root);
        CallAfter([this, entries = std::move(entries)]() mutable {
            m_entries = std::move(entries);
            m_refresh->Enable(true);
            Populate(m_entries);
        });
    }).detach();
}

void BackupHistoryPage::Populate(const std::vector<Core::Driver::DriverBackupManager::BackupEntry>& entries) {
    m_view->DeleteAllItems();

    if (entries.empty()) {
        m_status->SetLabelText(L"暂无备份记录");
        return;
    }

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        wxString pkgList;
        if (e.packageNames.size() <= 5) {
            for (const auto& n : e.packageNames) pkgList += n + L" ";
        } else {
            for (size_t j = 0; j < 5; ++j) pkgList += e.packageNames[j] + L" ";
            pkgList += wxString::Format(L"... 等共 %d 个", static_cast<int>(e.packageNames.size()));
        }
        m_view->AppendItem({wxVariant(e.time.c_str()),
                            wxVariant(e.systemVersion.c_str()),
                            wxVariant(wxString::Format(L"%d", e.packageCount)),
                            wxVariant(IceClean::Utils::FormatUtil::FormatFileSize(e.totalSize).c_str()),
                            wxVariant(pkgList.Trim())},
                            static_cast<wxUIntPtr>(i));
    }

    int64_t totalBytes = 0;
    for (const auto& e : entries) totalBytes += static_cast<int64_t>(e.totalSize);
    m_status->SetLabelText(wxString::Format(L"共 %d 次备份，合计 %s",
        static_cast<int>(entries.size()),
        IceClean::Utils::FormatUtil::FormatFileSize(totalBytes).c_str()));
}

void BackupHistoryPage::OnRefresh(wxCommandEvent&) { Refresh(); }

void BackupHistoryPage::OnOpenFolder(wxCommandEvent&) {
    const auto root = Core::Driver::DriverBackupManager::GetDefaultBackupRoot();
    ShellExecuteW(nullptr, L"open", root.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

int BackupHistoryPage::SelectedIndex() const {
    int row = m_view->GetSelectedRow();
    if (row < 0 || row >= static_cast<int>(m_entries.size())) return -1;
    return row;
}

void BackupHistoryPage::OnRestore(wxCommandEvent&) {
    const int idx = SelectedIndex();
    if (idx < 0) {
        wxMessageBox(L"请先选中一个备份条目。", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }
    const auto& e = m_entries[idx];

    ConfirmDialog cd(this, L"从备份还原驱动",
        wxString::Format(L"即将从以下备份还原驱动：\n\n备份时间：%s\n驱动包数：%d\n\n"
                         L"此操作会创建系统还原点，是否继续？",
                         e.time.c_str(), e.packageCount),
        ConfirmDialog::DangerLevel::Caution, L"还原", L"取消");
    if (cd.ShowModal() != wxID_OK) return;

    m_status->SetLabelText(L"正在还原驱动...");
    m_restore->Enable(false);
    const std::wstring dir = e.dir;
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

void BackupHistoryPage::OnDelete(wxCommandEvent&) {
    const int idx = SelectedIndex();
    if (idx < 0) {
        wxMessageBox(L"请先选中一个备份条目。", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }
    const auto& e = m_entries[idx];

    ConfirmDialog cd(this, L"删除备份",
        wxString::Format(L"确定要删除以下备份吗？此操作不可恢复。\n\n备份时间：%s\n驱动包数：%d",
                         e.time.c_str(), e.packageCount),
        ConfirmDialog::DangerLevel::Dangerous, L"删除", L"取消");
    if (cd.ShowModal() != wxID_OK) return;

    namespace fs = std::filesystem;
    std::error_code ec;
    fs::remove_all(fs::path(e.dir), ec);

    Refresh();
}

void BackupHistoryPage::OnItemActivated(wxDataViewEvent&) {
    const int idx = SelectedIndex();
    if (idx < 0) return;
    ShellExecuteW(nullptr, L"explore", m_entries[idx].dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

// ───────────────────────── 设备 & 驱动页（实现）─────────────────────────
void DeviceDriverPage::OnSearch(wxCommandEvent&) { ApplyFilterAndSearch(); }
void DeviceDriverPage::OnFilter(wxCommandEvent&) { ApplyFilterAndSearch(); }
void DeviceDriverPage::OnRefresh(wxCommandEvent&) { RefreshList(); }

void DeviceDriverPage::OnToggleGroup(wxCommandEvent&) {
    m_groupByClass = m_toggleGroup->GetValue();
    ApplyFilterAndSearch();
}

void DeviceDriverPage::OnHeaderSort(wxDataViewEvent& event) {
    auto* col = event.GetDataViewColumn();
    if (!col) return;
    const unsigned modelCol = col->GetModelColumn();
    if (static_cast<int>(modelCol) == DVC_CHECK) {
        ApplyFilterAndSearch();
        return;
    }
    if (static_cast<int>(modelCol) == m_sortColumn) m_sortAsc = !m_sortAsc;
    else { m_sortColumn = static_cast<int>(modelCol); m_sortAsc = true; }
    ApplyFilterAndSearch();
}

void DeviceDriverPage::ApplyFilterAndSearch() {
    const int sel = m_filter->GetSelection();
    const wxString kw = m_search->GetValue().Lower();

    std::vector<Core::Driver::DeviceDriverInfo> filtered;
    for (const auto& d : m_all) {
        const std::wstring name = DeviceName(d);
        switch (sel) {
        case 1: if (d.statusText == L"正常") continue; break;
        case 2: if (!d.isThirdParty) continue; break;
        case 3: if (d.isThirdParty) continue; break;
        default: break;
        }
        if (!kw.empty()) {
            std::wstring hay = name + L" " + d.driverDesc + L" " + d.manufacturer + L" " + d.provider;
            std::transform(hay.begin(), hay.end(), hay.begin(), ::towlower);
            if (hay.find(kw.ToStdWstring()) == std::wstring::npos) continue;
        }
        filtered.push_back(d);
    }

    std::stable_sort(filtered.begin(), filtered.end(),
        [this](const auto& a, const auto& b) {
            auto ka = SortKey(a, m_sortColumn);
            auto kb = SortKey(b, m_sortColumn);
            if (ka == kb) {
                ka = SortKey(a, DVC_DEVICE);
                kb = SortKey(b, DVC_DEVICE);
            }
            return m_sortAsc ? ka < kb : ka > kb;
        });

    Populate(filtered);
}

void DeviceDriverPage::Populate(const std::vector<Core::Driver::DeviceDriverInfo>& items) {
    m_view->DeleteAllItems();
    m_rowToDevice.clear();

    const auto& colors = ThemeManager::Instance().GetColors();

    if (!m_groupByClass) {
        for (const auto& d : items) {
            m_rowToDevice.push_back(d);
            const long row = static_cast<long>(m_rowToDevice.size() - 1);
            m_view->AppendItem({wxVariant(false),
                                wxVariant(DeviceName(d).c_str()),
                                wxVariant(d.manufacturer.c_str()),
                                wxVariant(d.deviceClass.c_str()),
                                wxVariant(d.version.c_str()),
                                wxVariant(d.date.c_str()),
                                wxVariant(d.statusText.c_str()),
                                wxVariant(Core::Driver::SignatureToText(d.signature).c_str())},
                                static_cast<wxUIntPtr>(row));
        }
    } else {
        std::map<std::wstring, std::vector<Core::Driver::DeviceDriverInfo>> buckets;
        for (const auto& d : items) {
            std::wstring key = d.deviceClass;
            std::transform(key.begin(), key.end(), key.begin(), ::towlower);
            key.erase(0, key.find_first_not_of(L" \t\r\n"));
            if (key.empty()) key = L"";
            buckets[key].push_back(d);
        }

        for (auto& [key, members] : buckets) {
            const std::wstring label = key.empty() ? L"未分类" : key;
            wxString headerLine = wxString::Format(L"\u25e2 %s (%d)", label.c_str(),
                                                    static_cast<int>(members.size()));
            m_rowToDevice.push_back({});
            m_rowToDevice.back().deviceName = headerLine.ToStdWstring();
            m_rowToDevice.back().driverDesc = L"__GROUP__";
            m_rowToDevice.back().statusText = label;
            const long groupRow = static_cast<long>(m_rowToDevice.size() - 1);

            m_view->AppendItem({wxVariant(false),
                                wxVariant(headerLine),
                                wxVariant(L""), wxVariant(L""), wxVariant(L""),
                                wxVariant(L""), wxVariant(L""), wxVariant(L"")},
                                static_cast<wxUIntPtr>(groupRow) | GROUP_ROW_FLAG);

            for (const auto& d : members) {
                m_rowToDevice.push_back(d);
                const long row = static_cast<long>(m_rowToDevice.size() - 1);
                m_view->AppendItem({wxVariant(false),
                                    wxVariant(DeviceName(d).c_str()),
                                    wxVariant(d.manufacturer.c_str()),
                                    wxVariant(d.deviceClass.c_str()),
                                    wxVariant(d.version.c_str()),
                                    wxVariant(d.date.c_str()),
                                    wxVariant(d.statusText.c_str()),
                                    wxVariant(Core::Driver::SignatureToText(d.signature).c_str())},
                                    static_cast<wxUIntPtr>(row));
            }
        }
    }

    int deviceCount = 0;
    for (const auto& r : m_rowToDevice) {
        if (r.driverDesc != L"__GROUP__") deviceCount++;
    }
    m_status->SetLabelText(wxString::Format(L"共 %d 个设备/驱动", deviceCount));
}

void DeviceDriverPage::OnValueChanged(wxDataViewEvent& event) {
    if (event.GetColumn() != DVC_CHECK) return;
}

void DeviceDriverPage::OnSelectAll(wxCommandEvent&) {
    const int n = m_view->GetItemCount();
    for (int i = 0; i < n; ++i) {
        const long idx = static_cast<long>(i);
        if (idx >= static_cast<long>(m_rowToDevice.size())) continue;
        if (m_rowToDevice[idx].driverDesc == L"__GROUP__") continue;
        m_view->SetToggleValue(true, i, DVC_CHECK);
    }
}

void DeviceDriverPage::OnSelectNone(wxCommandEvent&) {
    const int n = m_view->GetItemCount();
    for (int i = 0; i < n; ++i) {
        m_view->SetToggleValue(false, i, DVC_CHECK);
    }
}

void DeviceDriverPage::OnSelectThirdParty(wxCommandEvent&) {
    const int n = m_view->GetItemCount();
    for (int i = 0; i < n; ++i) m_view->SetToggleValue(false, i, DVC_CHECK);
    for (int i = 0; i < n; ++i) {
        const long idx = static_cast<long>(i);
        if (idx >= static_cast<long>(m_rowToDevice.size())) continue;
        if (m_rowToDevice[idx].driverDesc == L"__GROUP__") continue;
        if (m_rowToDevice[idx].isThirdParty) {
            m_view->SetToggleValue(true, i, DVC_CHECK);
        }
    }
}

std::vector<std::wstring> DeviceDriverPage::CollectSelectedOemInfs() const {
    std::set<std::wstring> unique;
    const int n = m_view->GetItemCount();
    for (int i = 0; i < n; ++i) {
        if (m_view->GetToggleValue(i, DVC_CHECK)) {
            const long idx = static_cast<long>(i);
            if (idx >= static_cast<long>(m_rowToDevice.size())) continue;
            const auto& d = m_rowToDevice[idx];
            if (d.driverDesc == L"__GROUP__") continue;
            if (!d.oemInf.empty()) unique.insert(d.oemInf);
        }
    }
    return std::vector<std::wstring>(unique.begin(), unique.end());
}

void DeviceDriverPage::OnBackupSelected(wxCommandEvent&) {
    auto selected = CollectSelectedOemInfs();
    if (selected.empty()) {
        wxMessageBox(L"请先勾选要备份的设备/驱动。", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    const auto root = Core::Driver::DriverBackupManager::GetDefaultBackupRoot();
    wxDirDialog dlg(this, L"选择驱动备份保存目录", root, wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;

    m_status->SetLabelText(L"正在备份驱动...");
    m_backup->Enable(false);
    const auto backupRoot = dlg.GetPath().ToStdWstring();
    std::thread([this, backupRoot, selected]() {
        const auto result = Core::Driver::DriverBackupManager::BackupSelected(
            backupRoot, selected, nullptr);
        CallAfter([this, result]() {
            m_backup->Enable(true);
            m_status->SetLabelText(L"");
            if (result.success) {
                wxMessageBox(wxString::Format(
                    L"驱动备份完成！已导出 %d 个驱动包到:\n%s",
                    result.packageCount, result.backupDir.c_str()),
                    L"IceClean", wxOK | wxICON_INFORMATION, this);
            } else {
                wxMessageBox(L"驱动备份失败，请确认有足够磁盘空间与管理员权限。",
                             L"IceClean", wxOK | wxICON_WARNING, this);
            }
        });
    }).detach();
}

} // namespace IceClean::Gui

