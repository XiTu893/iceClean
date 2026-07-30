#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/choice.h>
#include "core/analyzer/DownloadManager.h"

namespace IceClean::Gui {

class DownloadManagerPanel : public wxPanel {
public:
    DownloadManagerPanel(wxWindow* parent);

private:
    void OnSelectDir(wxCommandEvent& event);
    void OnScan(wxCommandEvent& event);
    void OnClean(wxCommandEvent& event);
    void OnMove(wxCommandEvent& event);
    void OnFilterChange(wxCommandEvent& event);

    void PopulateList();
    void UpdateButtonState();

    Core::Analyzer::DownloadManager m_manager;
    std::vector<Core::Analyzer::DownloadItem> m_items;
    std::vector<Core::Analyzer::DownloadItem> m_filteredItems;

    wxStaticText* m_pathLabel = nullptr;
    wxButton* m_selectDirBtn = nullptr;
    wxButton* m_scanBtn = nullptr;
    wxChoice* m_filterChoice = nullptr;
    wxListCtrl* m_fileList = nullptr;
    wxStaticText* m_summaryLabel = nullptr;
    wxButton* m_cleanBtn = nullptr;
    wxButton* m_moveBtn = nullptr;

    std::wstring m_downloadPath;
    bool m_isScanning = false;
};

} // namespace IceClean::Gui
