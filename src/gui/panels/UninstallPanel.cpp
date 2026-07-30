#include "UninstallPanel.h"
#include "core/cleaner/SoftwareUninstaller.h"
#include "core/analyzer/SoftwareUpdateChecker.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "utils/FormatUtil.h"
#include "utils/Win32Util.h"
#include "gui/controls/ThemeManager.h"
#include <shellapi.h>
#include <algorithm>
#include <thread>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(UninstallPanel, wxPanel)
wxEND_EVENT_TABLE()

UninstallPanel::UninstallPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();
}

void UninstallPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"软件管理");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    // 提示
    auto* tipLabel = new wxStaticText(this, wxID_ANY,
        L"查看和管理已安装的软件，支持卸载和清理残留文件。");
    tipLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
    tipLabel->SetForegroundColour(colors.textDisabled);
    mainSizer->Add(tipLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // Notebook（已安装 / 可升级）
    m_notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP);
    m_notebook->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));

    // ===== Tab 1: 已安装 =====
    auto* installedPage = new wxPanel(m_notebook, wxID_ANY);
    installedPage->SetBackgroundColour(colors.background);
    auto* installedSizer = new wxBoxSizer(wxVERTICAL);
    installedSizer->AddSpacer(4);

    // 搜索栏 + 操作按钮
    auto* toolbarSizer = new wxBoxSizer(wxHORIZONTAL);

    m_searchCtrl = new wxSearchCtrl(installedPage, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(240, 30));
    m_searchCtrl->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    m_searchCtrl->SetDescriptiveText(L"搜索软件...");
    m_searchCtrl->Bind(wxEVT_TEXT, &UninstallPanel::OnSearch, this);
    toolbarSizer->Add(m_searchCtrl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    m_refreshButton = new wxButton(installedPage, wxID_ANY, L"刷新", wxDefaultPosition, wxSize(70, 30));
    m_refreshButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
    m_refreshButton->Bind(wxEVT_BUTTON, &UninstallPanel::OnRefresh, this);
    toolbarSizer->Add(m_refreshButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    toolbarSizer->AddStretchSpacer();

    m_totalSizeLabel = new wxStaticText(installedPage, wxID_ANY, L"");
    m_totalSizeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
    m_totalSizeLabel->SetForegroundColour(colors.textSecondary);
    toolbarSizer->Add(m_totalSizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    installedSizer->Add(toolbarSizer, 0, wxEXPAND);

    // 软件列表
    m_softwareListCtrl = new wxListCtrl(installedPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_softwareListCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                       false, L"微软雅黑"));

    m_softwareListCtrl->AppendColumn(L"软件名称", wxLIST_FORMAT_LEFT, 240);
    m_softwareListCtrl->AppendColumn(L"发布者", wxLIST_FORMAT_LEFT, 140);
    m_softwareListCtrl->AppendColumn(L"版本", wxLIST_FORMAT_LEFT, 90);
    m_softwareListCtrl->AppendColumn(L"大小", wxLIST_FORMAT_RIGHT, 80);
    m_softwareListCtrl->AppendColumn(L"安装日期", wxLIST_FORMAT_LEFT, 100);

    m_softwareListCtrl->Bind(wxEVT_LIST_ITEM_SELECTED, &UninstallPanel::OnItemSelected, this);
    m_softwareListCtrl->Bind(wxEVT_LIST_ITEM_DESELECTED, &UninstallPanel::OnItemDeselected, this);
    m_softwareListCtrl->Bind(wxEVT_LIST_COL_CLICK, &UninstallPanel::OnColumnClick, this);
    m_softwareListCtrl->Bind(wxEVT_LIST_ITEM_ACTIVATED, &UninstallPanel::OnItemActivated, this);

    installedSizer->Add(m_softwareListCtrl, 1, wxEXPAND | wxTOP, 4);
    installedSizer->AddSpacer(8);

    // 底部按钮栏
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    m_uninstallButton = new wxButton(installedPage, wxID_ANY, L"卸载", wxDefaultPosition, wxSize(100, 36));
    m_uninstallButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                       false, L"微软雅黑"));
    m_uninstallButton->SetBackgroundColour(colors.accent);
    m_uninstallButton->SetForegroundColour(*wxWHITE);
    m_uninstallButton->SetName("btn_primary_uninstall");
    m_uninstallButton->Enable(false);
    m_uninstallButton->Bind(wxEVT_BUTTON, &UninstallPanel::OnUninstall, this);
    bottomSizer->Add(m_uninstallButton, 0, wxRIGHT, 8);

    m_silentUninstallButton = new wxButton(installedPage, wxID_ANY, L"静默卸载", wxDefaultPosition, wxSize(100, 36));
    m_silentUninstallButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                             false, L"微软雅黑"));
    m_silentUninstallButton->Enable(false);
    m_silentUninstallButton->Bind(wxEVT_BUTTON, &UninstallPanel::OnSilentUninstall, this);
    bottomSizer->Add(m_silentUninstallButton, 0, wxRIGHT, 8);

    m_cleanResidualButton = new wxButton(installedPage, wxID_ANY, L"清理残留", wxDefaultPosition, wxSize(100, 36));
    m_cleanResidualButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                            false, L"微软雅黑"));
    m_cleanResidualButton->Enable(false);
    m_cleanResidualButton->Bind(wxEVT_BUTTON, &UninstallPanel::OnCleanResidual, this);
    bottomSizer->Add(m_cleanResidualButton, 0, wxRIGHT, 8);

    m_forceUninstallButton = new wxButton(installedPage, wxID_ANY, L"强制卸载", wxDefaultPosition, wxSize(100, 36));
    m_forceUninstallButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                             false, L"微软雅黑"));
    m_forceUninstallButton->SetBackgroundColour(colors.danger);  // 红色
    m_forceUninstallButton->SetForegroundColour(*wxWHITE);
    m_forceUninstallButton->SetName("btn_danger_force_uninstall");
    m_forceUninstallButton->Enable(false);
    m_forceUninstallButton->Bind(wxEVT_BUTTON, &UninstallPanel::OnForceUninstall, this);
    bottomSizer->Add(m_forceUninstallButton, 0, wxRIGHT, 8);

    bottomSizer->AddStretchSpacer();

    m_statusLabel = new wxStaticText(installedPage, wxID_ANY, L"");
    m_statusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_statusLabel->SetForegroundColour(colors.textDisabled);
    bottomSizer->Add(m_statusLabel, 0, wxALIGN_CENTER_VERTICAL);

    installedSizer->Add(bottomSizer, 0, wxEXPAND);

    installedPage->SetSizer(installedSizer);
    m_notebook->AddPage(installedPage, L"已安装");

    // ===== Tab 2: 可升级 =====
    auto* updatePage = new wxPanel(m_notebook, wxID_ANY);
    updatePage->SetBackgroundColour(colors.background);
    auto* updateSizer = new wxBoxSizer(wxVERTICAL);
    updateSizer->AddSpacer(4);

    // 检查更新按钮
    auto* updateToolbarSizer = new wxBoxSizer(wxHORIZONTAL);

    m_checkUpdateButton = new wxButton(updatePage, wxID_ANY, L"检查更新", wxDefaultPosition, wxSize(120, 36));
    m_checkUpdateButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                         false, L"微软雅黑"));
    m_checkUpdateButton->SetBackgroundColour(colors.accent);
    m_checkUpdateButton->SetForegroundColour(*wxWHITE);
    m_checkUpdateButton->SetName("btn_primary_check_update");
    m_checkUpdateButton->Bind(wxEVT_BUTTON, &UninstallPanel::OnCheckUpdate, this);
    updateToolbarSizer->Add(m_checkUpdateButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    updateToolbarSizer->AddStretchSpacer();

    m_openDownloadButton = new wxButton(updatePage, wxID_ANY, L"打开下载页", wxDefaultPosition, wxSize(120, 36));
    m_openDownloadButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
    m_openDownloadButton->Enable(false);
    m_openDownloadButton->Bind(wxEVT_BUTTON, &UninstallPanel::OnOpenDownload, this);
    updateToolbarSizer->Add(m_openDownloadButton, 0, wxALIGN_CENTER_VERTICAL);

    updateSizer->Add(updateToolbarSizer, 0, wxEXPAND);

    // 可升级软件列表
    m_updateListCtrl = new wxListCtrl(updatePage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_updateListCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));

    m_updateListCtrl->AppendColumn(L"软件名称", wxLIST_FORMAT_LEFT, 240);
    m_updateListCtrl->AppendColumn(L"当前版本", wxLIST_FORMAT_LEFT, 120);
    m_updateListCtrl->AppendColumn(L"最新版本", wxLIST_FORMAT_LEFT, 120);
    m_updateListCtrl->AppendColumn(L"发布者", wxLIST_FORMAT_LEFT, 140);

    m_updateListCtrl->Bind(wxEVT_LIST_ITEM_SELECTED, &UninstallPanel::OnUpdateItemSelected, this);
    m_updateListCtrl->Bind(wxEVT_LIST_ITEM_DESELECTED, &UninstallPanel::OnUpdateItemDeselected, this);

    updateSizer->Add(m_updateListCtrl, 1, wxEXPAND | wxTOP, 4);

    updatePage->SetSizer(updateSizer);
    m_notebook->AddPage(updatePage, L"可升级");

    mainSizer->Add(m_notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);

    // 延迟加载软件列表
    CallAfter([this]() { RefreshSoftwareList(); });
}

void UninstallPanel::RefreshSoftwareList() {
    m_statusLabel->SetLabelText(L"正在加载软件列表...");
    m_refreshButton->Enable(false);

    // 在后台线程加载
    std::thread([this]() {
        IceClean::Core::Cleaner::SoftwareUninstaller uninstaller;
        auto software = uninstaller.GetInstalledSoftware();

        CallAfter([this, software = std::move(software)]() mutable {
            m_softwareList = std::move(software);
            PopulateList(m_softwareList);
            m_refreshButton->Enable(true);
            UpdateStatus();
        });
    }).detach();
}

void UninstallPanel::PopulateList(const std::vector<IceClean::Models::InstalledSoftware>& items) {
    m_softwareListCtrl->DeleteAllItems();
    m_filteredList = items;

    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const auto& sw = items[i];
        long idx = m_softwareListCtrl->InsertItem(i, sw.displayName);
        m_softwareListCtrl->SetItem(idx, 1, sw.publisher);
        m_softwareListCtrl->SetItem(idx, 2, sw.version);
        m_softwareListCtrl->SetItem(idx, 3, FormatSize(sw.estimatedSize));
        m_softwareListCtrl->SetItem(idx, 4, sw.installDate);

        // 存储指针以关联数据
        m_softwareListCtrl->SetItemData(idx, i);
    }
}

void UninstallPanel::OnSearch(wxCommandEvent& event) {
    wxString keyword = m_searchCtrl->GetValue().Lower();
    if (keyword.empty()) {
        PopulateList(m_softwareList);
        return;
    }

    std::vector<IceClean::Models::InstalledSoftware> filtered;
    for (const auto& sw : m_softwareList) {
        std::wstring name = sw.displayName;
        std::wstring pub = sw.publisher;
        std::transform(name.begin(), name.end(), name.begin(), ::towlower);
        std::transform(pub.begin(), pub.end(), pub.begin(), ::towlower);

        std::wstring kw = keyword.ToStdWstring();
        std::transform(kw.begin(), kw.end(), kw.begin(), ::towlower);

        if (name.find(kw) != std::wstring::npos || pub.find(kw) != std::wstring::npos) {
            filtered.push_back(sw);
        }
    }

    PopulateList(filtered);
}

void UninstallPanel::OnRefresh(wxCommandEvent& event) {
    RefreshSoftwareList();
}

void UninstallPanel::OnUninstall(wxCommandEvent& event) {
    auto software = GetSelectedSoftware();
    if (software.displayName.empty()) return;

    ConfirmDialog dlg(this, L"确认卸载",
        wxString::Format(L"确定要卸载 \"%s\" 吗？\n\n如果软件正在运行，请先关闭软件再卸载。", software.displayName.c_str()),
        ConfirmDialog::DangerLevel::Caution, L"卸载", L"取消");

    if (dlg.ShowModal() != wxID_OK) return;

    IceClean::Core::Cleaner::SoftwareUninstaller uninstaller;
    if (uninstaller.Uninstall(software)) {
        m_statusLabel->SetLabelText(L"已启动卸载程序，请在卸载完成后点击\"刷新\"更新列表。");
    } else {
        wxMessageBox(L"无法启动卸载程序，请尝试手动卸载。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void UninstallPanel::OnSilentUninstall(wxCommandEvent& event) {
    auto software = GetSelectedSoftware();
    if (software.displayName.empty()) return;

    ConfirmDialog dlg(this, L"确认静默卸载",
        wxString::Format(L"确定要静默卸载 \"%s\" 吗？\n\n"
                          L"静默卸载将不显示卸载界面直接执行。部分软件可能不支持静默卸载。", software.displayName.c_str()),
        ConfirmDialog::DangerLevel::Dangerous, L"静默卸载", L"取消");

    if (dlg.ShowModal() != wxID_OK) return;

    IceClean::Core::Cleaner::SoftwareUninstaller uninstaller;
    if (uninstaller.SilentUninstall(software)) {
        m_statusLabel->SetLabelText(L"已启动静默卸载，请等待卸载完成后点击\"刷新\"。");
    } else {
        wxMessageBox(L"无法启动静默卸载。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void UninstallPanel::OnCleanResidual(wxCommandEvent& event) {
    auto software = GetSelectedSoftware();
    if (software.displayName.empty()) return;

    IceClean::Core::Cleaner::SoftwareUninstaller uninstaller;
    auto residual = uninstaller.ScanResidualFiles(software);

    if (residual.empty()) {
        wxMessageBox(L"未发现残留文件。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxString residualList;
    for (const auto& path : residual) {
        residualList += L"• " + path + L"\n";
    }

    ConfirmDialog dlg(this, L"清理残留文件",
        L"发现以下残留文件/目录：\n\n" + residualList + L"\n确认删除？",
        ConfirmDialog::DangerLevel::Caution, L"清理", L"取消");

    if (dlg.ShowModal() != wxID_OK) return;

    if (uninstaller.CleanResidual(residual)) {
        m_statusLabel->SetLabelText(L"残留文件已清理完成。");
    } else {
        wxMessageBox(L"部分残留文件清理失败，可能需要管理员权限。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void UninstallPanel::OnForceUninstall(wxCommandEvent& event) {
    auto software = GetSelectedSoftware();
    if (software.displayName.empty()) return;

    ConfirmDialog dlg(this, L"⚠ 强制卸载",
        wxString::Format(L"确定要强制卸载 \"%s\" 吗？\n\n"
                          L"强制卸载将：\n"
                          L"1. 终止该软件的所有运行进程\n"
                          L"2. 强制删除安装目录\n"
                          L"3. 清理注册表残留项\n\n"
                          L"此操作不可撤销，请谨慎操作！",
                          software.displayName.c_str()),
        ConfirmDialog::DangerLevel::Dangerous, L"强制卸载", L"取消");

    if (dlg.ShowModal() != wxID_OK) return;

    m_statusLabel->SetLabelText(L"正在强制卸载...");
    m_forceUninstallButton->Enable(false);

    // 在后台线程执行强制卸载
    std::thread([this, software]() {
        IceClean::Core::Cleaner::SoftwareUninstaller uninstaller;
        auto result = uninstaller.ForceUninstall(software);

        CallAfter([this, result]() {
            if (IsBeingDeleted()) return;

            if (result.success) {
                m_statusLabel->SetLabelText(L"强制卸载完成");
                wxMessageBox(result.message, L"IceClean", wxOK | wxICON_INFORMATION, this);
                RefreshSoftwareList();
            } else {
                m_statusLabel->SetLabelText(L"强制卸载未完全成功");
                wxMessageBox(result.message, L"IceClean", wxOK | wxICON_WARNING, this);
                RefreshSoftwareList();
            }
        });
    }).detach();
}

void UninstallPanel::OnItemSelected(wxListEvent& event) {
    m_uninstallButton->Enable(true);
    m_silentUninstallButton->Enable(true);
    m_cleanResidualButton->Enable(true);
    m_forceUninstallButton->Enable(true);
}

void UninstallPanel::OnItemDeselected(wxListEvent& event) {
    m_uninstallButton->Enable(false);
    m_silentUninstallButton->Enable(false);
    m_cleanResidualButton->Enable(false);
    m_forceUninstallButton->Enable(false);
}

void UninstallPanel::OnColumnClick(wxListEvent& event) {
    int col = event.GetColumn();

    if (m_sortColumn == col) {
        m_sortAsc = !m_sortAsc;
    } else {
        m_sortColumn = col;
        m_sortAsc = true;
    }

    // 排序过滤后的列表
    auto& items = m_filteredList;
    std::sort(items.begin(), items.end(), [this, col](const auto& a, const auto& b) {
        bool less = false;
        switch (col) {
        case 0: less = _wcsicmp(a.displayName.c_str(), b.displayName.c_str()) < 0; break;
        case 1: less = _wcsicmp(a.publisher.c_str(), b.publisher.c_str()) < 0; break;
        case 2: less = _wcsicmp(a.version.c_str(), b.version.c_str()) < 0; break;
        case 3: less = a.estimatedSize < b.estimatedSize; break;
        case 4: less = _wcsicmp(a.installDate.c_str(), b.installDate.c_str()) < 0; break;
        }
        return m_sortAsc ? less : !less;
    });

    PopulateList(items);
}

void UninstallPanel::OnItemActivated(wxListEvent& event) {
    // 双击直接卸载
    wxCommandEvent dummy;
    OnUninstall(dummy);
}

void UninstallPanel::UpdateStatus() {
    m_statusLabel->SetLabelText(
        wxString::Format(L"共 %d 款软件", static_cast<int>(m_softwareList.size())));

    // 计算总大小
    uint64_t totalSize = 0;
    for (const auto& sw : m_softwareList) {
        totalSize += sw.estimatedSize;
    }
    m_totalSizeLabel->SetLabelText(
        wxString::Format(L"总占用: %s", FormatSize(totalSize).c_str()));
}

IceClean::Models::InstalledSoftware UninstallPanel::GetSelectedSoftware() const {
    long sel = m_softwareListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_filteredList.size())) {
        return IceClean::Models::InstalledSoftware();
    }
    return m_filteredList[sel];
}

wxString UninstallPanel::FormatSize(uint64_t bytes) const {
    return IceClean::Utils::FormatUtil::FormatFileSize(bytes);
}

void UninstallPanel::OnCheckUpdate(wxCommandEvent& event) {
    m_checkUpdateButton->Enable(false);
    m_updateListCtrl->DeleteAllItems();
    m_updatableSoftware.clear();
    m_openDownloadButton->Enable(false);

    std::thread([this]() {
        IceClean::Core::Analyzer::SoftwareUpdateChecker checker;
        auto updates = checker.CheckUpdates();

        CallAfter([this, updates = std::move(updates)]() mutable {
            if (IsBeingDeleted()) return;

            m_updatableSoftware = std::move(updates);
            PopulateUpdateList(m_updatableSoftware);
            m_checkUpdateButton->Enable(true);
        });
    }).detach();
}

void UninstallPanel::OnOpenDownload(wxCommandEvent& event) {
    long sel = m_updateListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0 || sel >= static_cast<long>(m_updatableSoftware.size())) return;

    const auto& software = m_updatableSoftware[sel];
    if (!software.downloadUrl.empty()) {
        ShellExecuteW(nullptr, L"open", software.downloadUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    } else {
        wxMessageBox(L"该软件暂无下载链接。", L"IceClean", wxOK | wxICON_INFORMATION, this);
    }
}

void UninstallPanel::OnUpdateItemSelected(wxListEvent& event) {
    m_openDownloadButton->Enable(true);
}

void UninstallPanel::OnUpdateItemDeselected(wxListEvent& event) {
    m_openDownloadButton->Enable(false);
}

void UninstallPanel::PopulateUpdateList(const std::vector<IceClean::Core::Analyzer::UpdatableSoftware>& items) {
    m_updateListCtrl->DeleteAllItems();

    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const auto& sw = items[i];
        long idx = m_updateListCtrl->InsertItem(i, sw.name);
        m_updateListCtrl->SetItem(idx, 1, sw.currentVersion);
        m_updateListCtrl->SetItem(idx, 2, sw.latestVersion);
        m_updateListCtrl->SetItem(idx, 3, sw.publisher);
        m_updateListCtrl->SetItemData(idx, i);
    }
}

} // namespace IceClean::Gui
