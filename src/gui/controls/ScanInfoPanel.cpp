#include "ScanInfoPanel.h"
#include "gui/controls/ThemeManager.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <utils/FormatUtil.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(ScanInfoPanel, wxPanel)
    EVT_PAINT(ScanInfoPanel::OnPaint)
    EVT_SIZE(ScanInfoPanel::OnSize)
wxEND_EVENT_TABLE()

ScanInfoPanel::ScanInfoPanel(wxWindow* parent, wxWindowID id,
                              const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size, wxBORDER_NONE)
{
    // 设置双缓冲和背景样式
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);

    // 初始化字体
    m_labelFont = wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑");
    m_valueFont = wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑");

    // 初始化颜色
    const auto& colors = ThemeManager::Instance().GetColors();
    m_normalColor = colors.textPrimary;
    m_processingColor = colors.accent;
    m_pausedColor = colors.warning;
    m_errorColor = colors.danger;

    // 初始化状态显示文本
    m_state = ScanInfoPanelState::Normal;
    // 注意：不预填 m_statusText —— Processing 态会原样显示它，
    // 预填会导致扫描期间残留"等待开始"等旧文案
}

void ScanInfoPanel::SetState(ScanInfoPanelState state) {
    m_state = state;
    Refresh();
}

void ScanInfoPanel::SetProcessingItem(const wxString& itemName) {
    m_processingItem = itemName;
    Refresh();
}

void ScanInfoPanel::SetProgressInfo(int current, int total, uint64_t size) {
    m_current = current;
    m_total = total;
    m_currentSize = size;
    Refresh();
}

void ScanInfoPanel::SetETA(const wxString& eta) {
    m_eta = eta;
    Refresh();
}

void ScanInfoPanel::SetError(const wxString& errorMsg) {
    m_errorMsg = errorMsg;
    m_state = ScanInfoPanelState::Error;
    Refresh();
}

void ScanInfoPanel::SetStatusText(const wxString& text) {
    m_statusText = text;
    Refresh();
}

wxString ScanInfoPanel::GetStateString() const {
    switch (m_state) {
        case ScanInfoPanelState::Normal: return L"等待开始";
        case ScanInfoPanelState::Processing: return L"正在处理";
        case ScanInfoPanelState::Paused: return L"暂停中";
        case ScanInfoPanelState::Error: return L"错误";
    }
    return L"未知";
}

void ScanInfoPanel::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) return;

    wxSize sz = GetClientSize();
    int w = sz.GetWidth();

    // 根据状态选择颜色
    wxColour textColor;
    wxFont labelFont = m_labelFont;
    wxFont valueFont = m_valueFont;
    wxString progressText;
    wxString itemLabel;

    switch (m_state) {
        case ScanInfoPanelState::Normal:
            textColor = m_normalColor;
            labelFont = m_labelFont;
            valueFont = wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑");
            progressText = !m_statusText.empty() ? m_statusText : wxString(L"等待开始");
            break;
        case ScanInfoPanelState::Processing:
            textColor = m_processingColor;
            labelFont = wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑");
            valueFont = wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑");
            itemLabel = m_processingItem;
            progressText = !m_statusText.empty()
                ? m_statusText
                : wxString::Format(L"%d / %d 项", m_current, m_total);
            if (!m_statusText.empty() && m_currentSize > 0) {
                progressText += wxString::Format(L"  (%s)", Utils::FormatUtil::FormatFileSize(m_currentSize));
            }
            break;
        case ScanInfoPanelState::Paused:
            textColor = m_pausedColor;
            labelFont = wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑");
            valueFont = wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑");
            progressText = !m_statusText.empty() ? m_statusText : wxString(L"暂停中");
            break;
        case ScanInfoPanelState::Error:
            textColor = m_errorColor;
            labelFont = wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑");
            valueFont = wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑");
            progressText = L"发生错误";
            if (!m_errorMsg.empty()) {
                progressText += L": " + m_errorMsg;
            }
            break;
    }

    // 绘制背景
    wxBrush bgBrush(GetBackgroundColour());
    dc.SetBrush(bgBrush);
    dc.Clear();

    // 紧凑布局：第一行状态文本，第二行当前处理项（空间足够时）
    int y = 3;
    if (!progressText.empty()) {
        dc.SetFont(labelFont);
        dc.SetTextForeground(textColor);
        wxString line1 = wxControl::Ellipsize(progressText, dc, wxELLIPSIZE_END, w - 16);
        dc.DrawText(line1, 8, y);
        y += dc.GetTextExtent(line1).GetHeight() + 2;
    }

    if (!itemLabel.empty() && m_state == ScanInfoPanelState::Processing) {
        dc.SetFont(m_labelFont);
        dc.SetTextForeground(ThemeManager::Instance().GetColors().textSecondary);
        wxString display = L"▸ " + itemLabel;
        wxString elided = wxControl::Ellipsize(display, dc, wxELLIPSIZE_START, w - 16);
        dc.DrawText(elided, 8, y);
    }
}

void ScanInfoPanel::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

} // namespace IceClean::Gui