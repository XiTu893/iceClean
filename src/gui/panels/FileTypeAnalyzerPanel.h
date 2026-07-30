#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/choice.h>
#include "core/analyzer/FileTypeAnalyzer.h"

namespace IceClean::Gui {

class FileTypeAnalyzerPanel : public wxPanel {
public:
    FileTypeAnalyzerPanel(wxWindow* parent);

private:
    void OnSelectDir(wxCommandEvent& event);
    void OnStartAnalysis(wxCommandEvent& event);
    void OnExportHtml(wxCommandEvent& event);
    void OnExportTxt(wxCommandEvent& event);

    void PerformAnalysis(const std::wstring& path);

    Core::Analyzer::FileTypeAnalyzer m_analyzer;
    Core::Analyzer::FileTypeReport m_report;

    wxStaticText* m_pathLabel = nullptr;
    wxButton* m_selectDirBtn = nullptr;
    wxButton* m_startBtn = nullptr;
    wxListCtrl* m_resultList = nullptr;
    wxStaticText* m_summaryLabel = nullptr;
    wxButton* m_exportHtmlBtn = nullptr;
    wxButton* m_exportTxtBtn = nullptr;

    std::wstring m_selectedPath;
    bool m_isAnalyzing = false;
};

} // namespace IceClean::Gui
