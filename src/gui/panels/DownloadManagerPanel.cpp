#include "DownloadManagerPanel.h"
#include "gui/controls/ThemeManager.h"
#include "gui/dialogs/UnifiedProgressDialog.h"
#include "gui/Events.h"
#include "core/utils/ProgressReporter.h"
#include <algorithm>
#include <thread>
#include <shellapi.h>
#include <shlobj.h>

namespace IceClean::Gui {

DownloadManagerPanel::DownloadManagerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.background);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // ── 标题 ──
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"下载文件管理");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* descLabel = new wxStaticText(this, wxID_ANY, L"管理下载文件夹中的文件，按类别分组，清理或迁移不需要的下载项。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // ── 路径选择 + 过滤 ──
    auto* topRow = new wxBoxSizer(wxHORIZONTAL);

    auto* pathLabel = new wxStaticText(this, wxID_ANY, L"目录:");
    pathLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    topRow->Add(pathLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    // 获取默认下载路径
    PWSTR knownPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &knownPath))) {
        m_downloadPath = knownPath;
        CoTaskMemFree(knownPath);
    }

    m_pathLabel = new wxStaticText(this, wxID_ANY, m_downloadPath);
    m_pathLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    m_pathLabel->SetForegroundColour(colors.accent);
    topRow->Add(m_pathLabel, 1, wxALIGN_CENTER_VERTICAL);

    m_selectDirBtn = new wxButton(this, wxID_ANY, L"浏览...");
    m_selectDirBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_selectDirBtn->Bind(wxEVT_BUTTON, &DownloadManagerPanel::OnSelectDir, this);
    topRow->Add(m_selectDirBtn, 0, wxLEFT, 8);

    m_scanBtn = new wxButton(this, wxID_ANY, L"扫描");
    m_scanBtn->SetBackgroundColour(colors.accent);
    m_scanBtn->SetForegroundColour(*wxWHITE);
    m_scanBtn->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                              false, L"微软雅黑"));
    m_scanBtn->Bind(wxEVT_BUTTON, &DownloadManagerPanel::OnScan, this);
    topRow->Add(m_scanBtn, 0, wxLEFT, 8);

    mainSizer->Add(topRow, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(8);

    // ── 过滤 + 汇总 ──
    auto* filterRow = new wxBoxSizer(wxHORIZONTAL);
    auto* filterLabel = new wxStaticText(this, wxID_ANY, L"筛选:");
    filterLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    filterRow->Add(filterLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_filterChoice = new wxChoice(this, wxID_ANY);
    m_filterChoice->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_filterChoice->Append(L"全部文件");
    m_filterChoice->Append(L"仅安装包");
    m_filterChoice->Append(L"非活跃文件 (>30天)");
    m_filterChoice->SetSelection(0);
    m_filterChoice->Bind(wxEVT_CHOICE, &DownloadManagerPanel::OnFilterChange, this);
    filterRow->Add(m_filterChoice, 0, wxEXPAND);
    filterRow->AddStretchSpacer();

    m_summaryLabel = new wxStaticText(this, wxID_ANY, L"未扫描");
    m_summaryLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_summaryLabel->SetForegroundColour(colors.textSecondary);
    filterRow->Add(m_summaryLabel, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(filterRow, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(8);

    // ── 文件列表 ──
    m_fileList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 280),
                                wxLC_REPORT | wxLC_SORT_ASCENDING | wxBORDER_SIMPLE);
    m_fileList->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    m_fileList->AppendColumn(L"文件名", wxLIST_FORMAT_LEFT, 200);
    m_fileList->AppendColumn(L"类别", wxLIST_FORMAT_LEFT, 80);
    m_fileList->AppendColumn(L"大小", wxLIST_FORMAT_RIGHT, 100);
    m_fileList->AppendColumn(L"安装包", wxLIST_FORMAT_CENTER, 60);
    m_fileList->AppendColumn(L"最后访问", wxLIST_FORMAT_LEFT, 140);
    mainSizer->Add(m_fileList, 1, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(8);

    // ── 操作按钮 ──
    auto* bottomRow = new wxBoxSizer(wxHORIZONTAL);
    bottomRow->AddStretchSpacer();

    m_cleanBtn = new wxButton(this, wxID_ANY, L"清理选中");
    m_cleanBtn->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    m_cleanBtn->Disable();
    m_cleanBtn->Bind(wxEVT_BUTTON, &DownloadManagerPanel::OnClean, this);
    bottomRow->Add(m_cleanBtn, 0, wxRIGHT, 8);

    m_moveBtn = new wxButton(this, wxID_ANY, L"迁移到...");
    m_moveBtn->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    m_moveBtn->Disable();
    m_moveBtn->Bind(wxEVT_BUTTON, &DownloadManagerPanel::OnMove, this);
    bottomRow->Add(m_moveBtn, 0);

    mainSizer->Add(bottomRow, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);
}

void DownloadManagerPanel::OnSelectDir(wxCommandEvent& /*event*/) {
    wxDirDialog dlg(this, L"选择下载目录", m_downloadPath);
    if (dlg.ShowModal() == wxID_OK) {
        m_downloadPath = dlg.GetPath().ToStdWstring();
        m_pathLabel->SetLabel(m_downloadPath);
    }
}

void DownloadManagerPanel::OnScan(wxCommandEvent& /*event*/) {
    if (m_isScanning || m_downloadPath.empty()) return;
    m_isScanning = true;
    m_scanBtn->Disable();
    m_summaryLabel->SetLabel(L"正在扫描...");
    m_fileList->DeleteAllItems();

    std::wstring path = m_downloadPath;
    std::thread([this, path]() {
        auto items = m_manager.ScanDownloads(path);

        CallAfter([this, items]() {
            m_items = items;
            m_filterChoice->SetSelection(0);
            PopulateList();
            m_isScanning = false;
            m_scanBtn->Enable();
        });
    }).detach();
}

void DownloadManagerPanel::OnFilterChange(wxCommandEvent& /*event*/) {
    PopulateList();
}

void DownloadManagerPanel::PopulateList() {
    m_fileList->DeleteAllItems();
    int filter = m_filterChoice->GetSelection();

    if (filter == 1) {
        m_filteredItems = m_manager.GetInstallers(m_items);
    } else if (filter == 2) {
        m_filteredItems = m_manager.GetInactiveFiles(m_items, 30);
    } else {
        m_filteredItems = m_items;
    }

    // Sort by size descending
    std::sort(m_filteredItems.begin(), m_filteredItems.end(),
              [](const Core::Analyzer::DownloadItem& a, const Core::Analyzer::DownloadItem& b) {
                  return a.size > b.size;
              });

    uint64_t totalSize = 0;
    for (const auto& item : m_filteredItems) {
        long idx = m_fileList->GetItemCount();
        m_fileList->InsertItem(idx, item.name);
        m_fileList->SetItem(idx, 1, item.category);
        m_fileList->SetItem(idx, 2, wxString::Format(L"%llu", item.size));
        m_fileList->SetItem(idx, 3, item.isInstaller ? L"✓" : L"");

        // Format last access time
        SYSTEMTIME st;
        FileTimeToSystemTime(&item.lastAccessTime, &st);
        m_fileList->SetItem(idx, 4, wxString::Format(L"%04d-%02d-%02d %02d:%02d",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute));

        totalSize += item.size;
    }

    int filteredCount = static_cast<int>(m_filteredItems.size());
    m_summaryLabel->SetLabel(wxString::Format(L"共 %d 项，总大小 %llu 字节", filteredCount, totalSize));

    UpdateButtonState();
}

void DownloadManagerPanel::OnClean(wxCommandEvent& /*event*/) {
    // Get selected items
    std::vector<Core::Analyzer::DownloadItem> toClean;
    long itemIdx = -1;
    while ((itemIdx = m_fileList->GetNextItem(itemIdx, wxLIST_NEXT_ALL,
                                               wxLIST_STATE_SELECTED)) != -1) {
        if (itemIdx < static_cast<long>(m_filteredItems.size())) {
            toClean.push_back(m_filteredItems[itemIdx]);
        }
    }

    if (toClean.empty()) {
        wxMessageBox(L"请先选择要清理的文件", L"提示", wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxString msg = wxString::Format(L"确定要永久删除选中的 %zu 个文件吗？\n此操作不可撤销！", toClean.size());
    wxMessageDialog confirm(this, msg, L"确认删除", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
    if (confirm.ShowModal() != wxID_YES) return;

    auto* progressDlg = new UnifiedProgressDialog(this, L"正在清理文件");
    progressDlg->Show();

    std::thread([this, toClean, progressDlg]() {
        auto result = m_manager.CleanItems(toClean,
            [progressDlg](int current, int total) {
                if (progressDlg->IsCancelled()) return;
                Core::Utils::ProgressSnapshot snap;
                snap.percent = total > 0 ? current * 100 / total : 0;
                snap.processedItems = current;
                snap.totalItems = total;

                wxThreadEvent* event = new wxThreadEvent(wxEVT_OPERATION_PROGRESS_UPDATE);
                event->SetPayload(snap);
                wxQueueEvent(progressDlg->GetParent(), event);
            }
        );

        wxString summary = wxString::Format(L"清理完成: 成功 %d 项，失败 %zu 项",
            result.cleanedCount, result.failedItems.size());

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_OPERATION_COMPLETE);
        completeEvt->SetInt(1);
        completeEvt->SetPayload(summary);
        wxQueueEvent(progressDlg->GetParent(), completeEvt);

        // Re-scan on complete
        CallAfter([this]() {
            wxCommandEvent evt;
            OnScan(evt);
        });
    }).detach();
}

void DownloadManagerPanel::OnMove(wxCommandEvent& /*event*/) {
    std::vector<Core::Analyzer::DownloadItem> toMove;
    long itemIdx = -1;
    while ((itemIdx = m_fileList->GetNextItem(itemIdx, wxLIST_NEXT_ALL,
                                               wxLIST_STATE_SELECTED)) != -1) {
        if (itemIdx < static_cast<long>(m_filteredItems.size())) {
            toMove.push_back(m_filteredItems[itemIdx]);
        }
    }

    if (toMove.empty()) {
        wxMessageBox(L"请先选择要迁移的文件", L"提示", wxOK | wxICON_INFORMATION, this);
        return;
    }

    wxDirDialog dlg(this, L"选择目标目录", L"D:\\");
    if (dlg.ShowModal() != wxID_OK) return;

    std::wstring targetPath = dlg.GetPath().ToStdWstring();

    auto* progressDlg = new UnifiedProgressDialog(this, L"正在迁移文件");
    progressDlg->Show();

    std::thread([this, toMove, targetPath, progressDlg]() {
        auto result = m_manager.MoveItems(toMove, targetPath,
            [progressDlg](int current, int total) {
                if (progressDlg->IsCancelled()) return;
                Core::Utils::ProgressSnapshot snap;
                snap.percent = total > 0 ? current * 100 / total : 0;
                snap.processedItems = current;
                snap.totalItems = total;

                wxThreadEvent* event = new wxThreadEvent(wxEVT_OPERATION_PROGRESS_UPDATE);
                event->SetPayload(snap);
                wxQueueEvent(progressDlg->GetParent(), event);
            }
        );

        wxString summary = wxString::Format(L"迁移完成: 成功 %d 项，失败 %zu 项",
            result.movedCount, result.failedItems.size());

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_OPERATION_COMPLETE);
        completeEvt->SetInt(1);
        completeEvt->SetPayload(summary);
        wxQueueEvent(progressDlg->GetParent(), completeEvt);

        CallAfter([this]() {
            wxCommandEvent evt;
            OnScan(evt);
        });
    }).detach();
}

void DownloadManagerPanel::UpdateButtonState() {
    bool hasSelection = m_fileList->GetSelectedItemCount() > 0;
    m_cleanBtn->Enable(hasSelection);
    m_moveBtn->Enable(hasSelection);
}

} // namespace IceClean::Gui
