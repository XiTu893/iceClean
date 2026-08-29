#pragma once
#include <wx/wx.h>
#include <functional>

namespace IceClean::Gui {

enum class ScanInfoPanelState {
    Normal,        // "正在扫描各类缓存..."
    Processing,    // "正在清理 wxWidgets 缓存..."
    Paused,        // "暂停中..."
    Error          // "发生错误：\n\t详细信息"
};

class ScanInfoPanel : public wxPanel {
public:
    ScanInfoPanel(wxWindow* parent, wxWindowID id = wxID_ANY,
                  const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize);
    ~ScanInfoPanel() override = default;

    void SetState(ScanInfoPanelState state);
    void SetProcessingItem(const wxString& itemName);
    void SetProgressInfo(int current, int total, uint64_t size);
    void SetETA(const wxString& eta);
    void SetError(const wxString& errorMsg);
    void SetStatusText(const wxString& text); // 新增

    wxString GetStateString() const;
    wxString GetProcessingItem() const { return m_processingItem; }

protected:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);

private:
    ScanInfoPanelState m_state = ScanInfoPanelState::Normal;
    wxString m_processingItem;
    int m_current = 0;
    int m_total = 0;
    uint64_t m_currentSize = 0;
    wxString m_eta;
    wxString m_errorMsg;
    wxString m_statusText;

    wxFont m_labelFont;
    wxFont m_valueFont;
    wxColour m_normalColor;
    wxColour m_processingColor;
    wxColour m_pausedColor;
    wxColour m_errorColor;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui