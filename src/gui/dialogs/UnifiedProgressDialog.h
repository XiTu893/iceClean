#pragma once
#include <wx/wx.h>
#include <wx/stattext.h>
#include <wx/gauge.h>
#include <wx/timer.h>
#include <atomic>
#include <string>

namespace IceClean::Gui {

struct UnifiedProgressData {
    int percent = 0;
    wxString stage;
    wxString detail;
    int subPercent = 0;
    wxString subDetail;
    uint64_t processedBytes = 0;
    uint64_t totalBytes = 0;
    int processedItems = 0;
    int totalItems = 0;
    double speedBytesPerSec = 0.0;
};

class UnifiedProgressDialog : public wxDialog {
public:
    UnifiedProgressDialog(wxWindow* parent, const wxString& title = L"正在操作...");

    void UpdateData(const UnifiedProgressData& data);
    void SetFinished(bool success, const wxString& summary);

    bool IsCancelled() const { return m_cancelled; }

private:
    void FormatSpeed(uint64_t bytesPerSec, wxString& out) const;
    void FormatEta(uint64_t remainingBytes, double speed, wxString& out) const;
    void FormatSize(uint64_t bytes, wxString& out) const;
    void OnCancel(wxCommandEvent& event);
    void OnTimerTick(wxTimerEvent& event);

    wxGauge* m_mainGauge = nullptr;
    wxStaticText* m_percentLabel = nullptr;
    wxStaticText* m_stageLabel = nullptr;
    wxStaticText* m_detailLabel = nullptr;
    wxGauge* m_subGauge = nullptr;
    wxStaticText* m_speedLabel = nullptr;
    wxStaticText* m_etaLabel = nullptr;
    wxStaticText* m_itemsLabel = nullptr;
    wxStaticText* m_sizeLabel = nullptr;
    wxButton* m_cancelButton = nullptr;

    wxTimer* m_refreshTimer = nullptr;
    std::atomic<bool> m_cancelled{false};
    bool m_finished = false;

    UnifiedProgressData m_lastData;
};

} // namespace IceClean::Gui
