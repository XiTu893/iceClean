#pragma once
#include <wx/wx.h>

namespace IceClean::Gui {

class CleanProgressDialog : public wxDialog {
public:
    CleanProgressDialog(wxWindow* parent);

    void SetProgress(int percent);
    void SetCurrentFile(const wxString& fileName);
    void SetCleanedSize(uint64_t bytes);
    void SetCategory(const wxString& category);
    void SetFinished(uint64_t totalCleaned);

private:
    wxGauge* m_progressGauge = nullptr;
    wxStaticText* m_categoryLabel = nullptr;
    wxStaticText* m_fileLabel = nullptr;
    wxStaticText* m_sizeLabel = nullptr;
    wxButton* m_cancelButton = nullptr;
    bool m_cancelled = false;

    void OnCancel(wxCommandEvent& event);

public:
    bool IsCancelled() const { return m_cancelled; }
};

} // namespace IceClean::Gui
