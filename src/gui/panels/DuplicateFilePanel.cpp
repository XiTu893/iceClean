#include "DuplicateFilePanel.h"
#include "core/analyzer/DuplicateFileFinder.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/controls/ThemeManager.h"
#include "utils/FormatUtil.h"
#include <wx/spinctrl.h>
#include <thread>
#include <algorithm>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(DuplicateFilePanel, wxPanel)
wxEND_EVENT_TABLE()

DuplicateFilePanel::DuplicateFilePanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();
}

void DuplicateFilePanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"重复文件查找");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* tipLabel = new wxStaticText(this, wxID_ANY,
        L"扫描指定目录查找重复文件，释放磁盘空间。");
    tipLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    tipLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(tipLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 扫描设置
    auto* scanSizer = new wxBoxSizer(wxHORIZONTAL);
    scanSizer->AddSpacer(20);

    auto* pathLabel = new wxStaticText(this, wxID_ANY, L"扫描路径:");
    pathLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    scanSizer->Add(pathLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_pathText = new wxTextCtrl(this, wxID_ANY, L"C:\\Users", wxDefaultPosition, wxSize(300, 28));
    m_pathText->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    scanSizer->Add(m_pathText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_browseButton = new wxButton(this, wxID_ANY, L"浏览", wxDefaultPosition, wxSize(60, 28));
    m_browseButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_browseButton->Bind(wxEVT_BUTTON, &DuplicateFilePanel::OnBrowse, this);
    scanSizer->Add(m_browseButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    auto* minLabel = new wxStaticText(this, wxID_ANY, L"最小大小(KB):");
    minLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    scanSizer->Add(minLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_minSizeSpin = new wxSpinCtrl(this, wxID_ANY, L"1024", wxDefaultPosition, wxSize(80, 28),
                                    wxSP_ARROW_KEYS, 1, 1024000, 1024);
    m_minSizeSpin->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    scanSizer->Add(m_minSizeSpin, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    m_scanButton = new wxButton(this, wxID_ANY, L"开始扫描", wxDefaultPosition, wxSize(90, 30));
    m_scanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_scanButton->SetBackgroundColour(colors.accent);
    m_scanButton->SetForegroundColour(*wxWHITE);
    m_scanButton->Bind(wxEVT_BUTTON, &DuplicateFilePanel::OnScan, this);
    scanSizer->Add(m_scanButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_stopButton = new wxButton(this, wxID_ANY, L"停止", wxDefaultPosition, wxSize(60, 30));
    m_stopButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_stopButton->Enable(false);
    m_stopButton->Bind(wxEVT_BUTTON, &DuplicateFilePanel::OnStop, this);
    scanSizer->Add(m_stopButton, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(scanSizer, 0, wxEXPAND);
    mainSizer->AddSpacer(4);

    // 进度条
    m_progressGauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL);
    m_progressGauge->Hide();
    mainSizer->Add(m_progressGauge, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    // 分割区域 - 上方显示重复组列表，下方显示组内文件详情
    auto* splitSizer = new wxBoxSizer(wxVERTICAL);

    // 重复组列表
    auto* groupLabel = new wxStaticText(this, wxID_ANY, L"重复文件组:");
    groupLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    splitSizer->Add(groupLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->Add(splitSizer, 0, wxEXPAND);

    m_resultListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 180),
                                       wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_resultListCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_resultListCtrl->AppendColumn(L"文件大小", wxLIST_FORMAT_RIGHT, 100);
    m_resultListCtrl->AppendColumn(L"重复数量", wxLIST_FORMAT_CENTER, 80);
    m_resultListCtrl->AppendColumn(L"浪费空间", wxLIST_FORMAT_RIGHT, 100);
    m_resultListCtrl->AppendColumn(L"文件路径(首个)", wxLIST_FORMAT_LEFT, 400);
    m_resultListCtrl->Bind(wxEVT_LIST_ITEM_SELECTED, &DuplicateFilePanel::OnGroupSelected, this);
    m_resultListCtrl->Bind(wxEVT_LIST_ITEM_DESELECTED, &DuplicateFilePanel::OnGroupDeselected, this);
    mainSizer->Add(m_resultListCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    // 组内文件详情
    auto* detailLabel = new wxStaticText(this, wxID_ANY, L"组内文件详情:");
    detailLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    mainSizer->Add(detailLabel, 0, wxLEFT | wxRIGHT, 20);

    m_detailListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_detailListCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_detailListCtrl->AppendColumn(L"文件路径", wxLIST_FORMAT_LEFT, 500);
    m_detailListCtrl->AppendColumn(L"文件大小", wxLIST_FORMAT_RIGHT, 100);
    mainSizer->Add(m_detailListCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 底部按钮栏
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    m_deleteSelectedButton = new wxButton(this, wxID_ANY, L"删除重复(保留首个)", wxDefaultPosition, wxSize(160, 36));
    m_deleteSelectedButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_deleteSelectedButton->Enable(false);
    m_deleteSelectedButton->Bind(wxEVT_BUTTON, &DuplicateFilePanel::OnDeleteSelected, this);
    bottomSizer->Add(m_deleteSelectedButton, 0, wxRIGHT, 8);

    m_moveSelectedButton = new wxButton(this, wxID_ANY, L"移动重复文件", wxDefaultPosition, wxSize(120, 36));
    m_moveSelectedButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_moveSelectedButton->Enable(false);
    m_moveSelectedButton->Bind(wxEVT_BUTTON, &DuplicateFilePanel::OnMoveSelected, this);
    bottomSizer->Add(m_moveSelectedButton, 0, wxRIGHT, 8);

    bottomSizer->AddStretchSpacer();

    m_wastedLabel = new wxStaticText(this, wxID_ANY, L"");
    m_wastedLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    m_wastedLabel->SetForegroundColour(colors.danger);
    bottomSizer->Add(m_wastedLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    m_statusLabel = new wxStaticText(this, wxID_ANY, L"");
    m_statusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_statusLabel->SetForegroundColour(colors.textSecondary);
    bottomSizer->Add(m_statusLabel, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);
}

void DuplicateFilePanel::OnBrowse(wxCommandEvent& event) {
    wxDirDialog dlg(this, L"选择扫描目录", m_pathText->GetValue());
    if (dlg.ShowModal() == wxID_OK) {
        m_pathText->SetValue(dlg.GetPath());
    }
}

void DuplicateFilePanel::OnScan(wxCommandEvent& event) {
    auto scanPath = m_pathText->GetValue().ToStdWstring();
    if (scanPath.empty()) {
        wxMessageBox(L"请输入扫描路径。", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    m_isScanning = true;
    m_scanButton->Enable(false);
    m_stopButton->Enable(true);
    m_resultListCtrl->DeleteAllItems();
    m_detailListCtrl->DeleteAllItems();
    m_duplicateGroups.clear();
    m_progressGauge->Show();
    m_progressGauge->SetRange(100);
    m_progressGauge->SetValue(0);
    Layout();

    uint64_t minSize = static_cast<uint64_t>(m_minSizeSpin->GetValue()) * 1024;

    std::thread([this, scanPath, minSize]() {
        IceClean::Core::Analyzer::DuplicateFileFinder finder;
        auto results = finder.Scan(scanPath,
            [this](const IceClean::Models::DuplicateScanProgress& progress) {
                CallAfter([this, progress]() {
                    m_statusLabel->SetLabelText(
                        wxString::Format(L"扫描中: %s (已扫描 %d 个文件, 发现 %d 组重复)",
                            progress.currentPath.c_str(), progress.scannedFiles, progress.duplicateGroups));
                });
            },
            minSize);

        CallAfter([this, results = std::move(results)]() mutable {
            m_duplicateGroups = std::move(results);
            m_isScanning = false;
            m_scanButton->Enable(true);
            m_stopButton->Enable(false);
            m_progressGauge->Hide();
            UpdateResultList();
            UpdateWastedSpace();

            if (m_duplicateGroups.empty()) {
                m_statusLabel->SetLabelText(L"未发现重复文件。");
            } else {
                m_statusLabel->SetLabelText(
                    wxString::Format(L"发现 %d 组重复文件", static_cast<int>(m_duplicateGroups.size())));
            }
        });
    }).detach();
}

void DuplicateFilePanel::OnStop(wxCommandEvent& event) {
    // TODO: signal cancellation to the finder
    m_isScanning = false;
    m_scanButton->Enable(true);
    m_stopButton->Enable(false);
    m_statusLabel->SetLabelText(L"扫描已停止。");
}

void DuplicateFilePanel::OnGroupSelected(wxListEvent& event) {
    int sel = event.GetIndex();
    if (sel < 0 || sel >= static_cast<int>(m_duplicateGroups.size())) return;

    const auto& group = m_duplicateGroups[sel];
    m_detailListCtrl->DeleteAllItems();

    for (int i = 0; i < static_cast<int>(group.filePaths.size()); ++i) {
        long idx = m_detailListCtrl->InsertItem(i, group.filePaths[i]);
        m_detailListCtrl->SetItem(idx, 1, FormatSize(group.fileSize));
    }

    m_deleteSelectedButton->Enable(true);
    m_moveSelectedButton->Enable(true);
}

void DuplicateFilePanel::OnGroupDeselected(wxListEvent& event) {
    m_detailListCtrl->DeleteAllItems();
    m_deleteSelectedButton->Enable(false);
    m_moveSelectedButton->Enable(false);
}

void DuplicateFilePanel::OnDeleteSelected(wxCommandEvent& event) {
    long sel = m_resultListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0) return;

    const auto& group = m_duplicateGroups[sel];

    ConfirmDialog dlg(this, L"删除重复文件",
        wxString::Format(L"确定要删除这组重复文件吗？\n\n"
            L"将保留第一个文件，删除其余 %d 个重复文件。\n"
            L"可释放 %s 磁盘空间。",
            static_cast<int>(group.filePaths.size()) - 1,
            FormatSize(group.wastedSpace).c_str()),
        ConfirmDialog::DangerLevel::Dangerous, L"删除", L"取消");

    if (dlg.ShowModal() != wxID_OK) return;

    IceClean::Core::Analyzer::DuplicateFileFinder finder;
    std::vector<int> groupIndices = { static_cast<int>(sel) };
    int deleted = finder.DeleteDuplicates(m_duplicateGroups, groupIndices);

    // 移除已处理的组
    m_duplicateGroups.erase(m_duplicateGroups.begin() + sel);
    UpdateResultList();
    UpdateWastedSpace();
    m_detailListCtrl->DeleteAllItems();

    wxString msg = wxString::Format(L"已删除 %d 个重复文件。", deleted);
    wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
}

void DuplicateFilePanel::OnMoveSelected(wxCommandEvent& event) {
    long sel = m_resultListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (sel < 0) return;

    wxDirDialog dlg(this, L"选择移动目标目录");
    if (dlg.ShowModal() != wxID_OK) return;

    auto targetDir = dlg.GetPath().ToStdWstring();
    IceClean::Core::Analyzer::DuplicateFileFinder finder;
    std::vector<int> groupIndices = { static_cast<int>(sel) };
    int moved = finder.MoveDuplicates(m_duplicateGroups, groupIndices, targetDir);

    m_duplicateGroups.erase(m_duplicateGroups.begin() + sel);
    UpdateResultList();
    UpdateWastedSpace();
    m_detailListCtrl->DeleteAllItems();

    wxString msg = wxString::Format(L"已移动 %d 个重复文件。", moved);
    wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
}

void DuplicateFilePanel::UpdateResultList() {
    m_resultListCtrl->DeleteAllItems();

    for (int i = 0; i < static_cast<int>(m_duplicateGroups.size()); ++i) {
        const auto& group = m_duplicateGroups[i];
        long idx = m_resultListCtrl->InsertItem(i, FormatSize(group.fileSize));
        m_resultListCtrl->SetItem(idx, 1, wxString::Format(L"%d", static_cast<int>(group.filePaths.size())));
        m_resultListCtrl->SetItem(idx, 2, FormatSize(group.wastedSpace));
        if (!group.filePaths.empty()) {
            m_resultListCtrl->SetItem(idx, 3, group.filePaths[0]);
        }
    }
}

void DuplicateFilePanel::UpdateWastedSpace() {
    uint64_t totalWasted = IceClean::Core::Analyzer::DuplicateFileFinder::GetTotalWastedSpace(m_duplicateGroups);
    m_wastedLabel->SetLabelText(
        wxString::Format(L"浪费空间: %s", FormatSize(totalWasted).c_str()));
}

wxString DuplicateFilePanel::FormatSize(uint64_t bytes) const {
    return IceClean::Utils::FormatUtil::FormatFileSize(bytes);
}

} // namespace IceClean::Gui
