#include "FileTypeAnalyzerPanel.h"
#include "gui/controls/ThemeManager.h"
#include "gui/dialogs/UnifiedProgressDialog.h"
#include "gui/Events.h"
#include "core/utils/ProgressReporter.h"
#include <algorithm>
#include <fstream>

namespace IceClean::Gui {

FileTypeAnalyzerPanel::FileTypeAnalyzerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.background);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // ── 标题 ──
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"文件类型分类统计");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* descLabel = new wxStaticText(this, wxID_ANY, L"扫描指定目录，按文件类型分类统计，帮助了解磁盘空间使用情况。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // ── 路径选择行 ──
    auto* pathRow = new wxBoxSizer(wxHORIZONTAL);
    auto* pathLabel = new wxStaticText(this, wxID_ANY, L"扫描目录:");
    pathLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    pathRow->Add(pathLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_pathLabel = new wxStaticText(this, wxID_ANY, L"C:\\");
    m_pathLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                false, L"微软雅黑"));
    m_pathLabel->SetForegroundColour(colors.accent);
    pathRow->Add(m_pathLabel, 1, wxALIGN_CENTER_VERTICAL);

    m_selectDirBtn = new wxButton(this, wxID_ANY, L"浏览...");
    m_selectDirBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_selectDirBtn->Bind(wxEVT_BUTTON, &FileTypeAnalyzerPanel::OnSelectDir, this);
    pathRow->Add(m_selectDirBtn, 0, wxLEFT, 8);

    m_startBtn = new wxButton(this, wxID_ANY, L"开始分析");
    m_startBtn->SetBackgroundColour(colors.accent);
    m_startBtn->SetForegroundColour(*wxWHITE);
    m_startBtn->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    m_startBtn->Bind(wxEVT_BUTTON, &FileTypeAnalyzerPanel::OnStartAnalysis, this);
    pathRow->Add(m_startBtn, 0, wxLEFT, 8);

    mainSizer->Add(pathRow, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(12);

    // ── 结果列表 ──
    m_resultList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 300),
                                   wxLC_REPORT | wxLC_SORT_ASCENDING | wxBORDER_SIMPLE);
    m_resultList->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    m_resultList->AppendColumn(L"类别", wxLIST_FORMAT_LEFT, 120);
    m_resultList->AppendColumn(L"扩展名", wxLIST_FORMAT_LEFT, 100);
    m_resultList->AppendColumn(L"文件数", wxLIST_FORMAT_RIGHT, 80);
    m_resultList->AppendColumn(L"总大小", wxLIST_FORMAT_RIGHT, 100);
    m_resultList->AppendColumn(L"占比", wxLIST_FORMAT_RIGHT, 80);
    m_resultList->AppendColumn(L"典型路径", wxLIST_FORMAT_LEFT, 200);
    mainSizer->Add(m_resultList, 1, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(4);

    // ── 汇总信息 + 导出按钮 ──
    auto* bottomRow = new wxBoxSizer(wxHORIZONTAL);
    m_summaryLabel = new wxStaticText(this, wxID_ANY, L"准备就绪");
    m_summaryLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_summaryLabel->SetForegroundColour(colors.textSecondary);
    bottomRow->Add(m_summaryLabel, 1, wxALIGN_CENTER_VERTICAL);

    m_exportHtmlBtn = new wxButton(this, wxID_ANY, L"导出 HTML");
    m_exportHtmlBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
    m_exportHtmlBtn->Disable();
    m_exportHtmlBtn->Bind(wxEVT_BUTTON, &FileTypeAnalyzerPanel::OnExportHtml, this);
    bottomRow->Add(m_exportHtmlBtn, 0, wxRIGHT, 8);

    m_exportTxtBtn = new wxButton(this, wxID_ANY, L"导出 TXT");
    m_exportTxtBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_exportTxtBtn->Disable();
    m_exportTxtBtn->Bind(wxEVT_BUTTON, &FileTypeAnalyzerPanel::OnExportTxt, this);
    bottomRow->Add(m_exportTxtBtn, 0);

    mainSizer->Add(bottomRow, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);
}

void FileTypeAnalyzerPanel::OnSelectDir(wxCommandEvent& /*event*/) {
    wxDirDialog dlg(this, L"选择要分析的目录", m_selectedPath.empty() ? L"C:\\" : m_selectedPath);
    if (dlg.ShowModal() == wxID_OK) {
        m_selectedPath = dlg.GetPath().ToStdWstring();
        m_pathLabel->SetLabel(dlg.GetPath());
    }
}

void FileTypeAnalyzerPanel::OnStartAnalysis(wxCommandEvent& /*event*/) {
    if (m_isAnalyzing) return;

    std::wstring path = m_selectedPath;
    if (path.empty()) path = L"C:\\";
    PerformAnalysis(path);
}

void FileTypeAnalyzerPanel::PerformAnalysis(const std::wstring& path) {
    m_isAnalyzing = true;
    m_startBtn->Disable();
    m_resultList->DeleteAllItems();
    m_summaryLabel->SetLabel(L"正在分析，请稍候...");
    m_exportHtmlBtn->Disable();
    m_exportTxtBtn->Disable();

    std::thread([this, path]() {
        auto report = m_analyzer.Analyze(path);

        CallAfter([this, report]() {
            m_report = report;
            m_resultList->DeleteAllItems();

            uint64_t totalSize = report.totalSize;
            int totalCount = report.totalFileCount;

            for (const auto& stat : report.stats) {
                long idx = m_resultList->GetItemCount();
                m_resultList->InsertItem(idx, stat.category);
                m_resultList->SetItem(idx, 1, stat.extension);
                m_resultList->SetItem(idx, 2, wxString::Format(L"%d", stat.fileCount));
                m_resultList->SetItem(idx, 3, wxString::Format(L"%llu", stat.totalSize));
                if (totalSize > 0) {
                    double pct = 100.0 * stat.totalSize / totalSize;
                    m_resultList->SetItem(idx, 4, wxString::Format(L"%.1f%%", pct));
                }
                m_resultList->SetItem(idx, 5, stat.typicalPath);
            }

            m_summaryLabel->SetLabel(wxString::Format(L"共 %d 个文件，总大小 %llu 字节 | 扫描路径: %s",
                totalCount, totalSize, report.scanPath.c_str()));

            m_exportHtmlBtn->Enable(!report.stats.empty());
            m_exportTxtBtn->Enable(!report.stats.empty());
            m_isAnalyzing = false;
            m_startBtn->Enable();
        });
    }).detach();
}

void FileTypeAnalyzerPanel::OnExportHtml(wxCommandEvent& /*event*/) {
    wxFileDialog dlg(this, L"导出 HTML 报告", L"", L"文件类型分析报告.html",
                      L"HTML 文件 (*.html)|*.html", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    std::wstring outputPath = dlg.GetPath().ToStdWstring();
    if (m_analyzer.ExportHtml(m_report, outputPath)) {
        wxMessageBox(wxString::Format(L"报告已导出到: %s", dlg.GetPath()),
                      L"导出成功", wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(L"导出失败，请检查文件权限。", L"错误", wxOK | wxICON_ERROR, this);
    }
}

void FileTypeAnalyzerPanel::OnExportTxt(wxCommandEvent& /*event*/) {
    wxFileDialog dlg(this, L"导出 TXT 报告", L"", L"文件类型分析报告.txt",
                      L"文本文件 (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    std::wstring outputPath = dlg.GetPath().ToStdWstring();
    if (m_analyzer.ExportTxt(m_report, outputPath)) {
        wxMessageBox(wxString::Format(L"报告已导出到: %s", dlg.GetPath()),
                      L"导出成功", wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(L"导出失败，请检查文件权限。", L"错误", wxOK | wxICON_ERROR, this);
    }
}

} // namespace IceClean::Gui
