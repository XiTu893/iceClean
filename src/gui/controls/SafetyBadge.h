#pragma once
#include <wx/wx.h>
#include "models/ScanResult.h"

namespace IceClean::Gui {

// 安全等级标识控件
class SafetyBadge : public wxPanel {
public:
    SafetyBadge(wxWindow* parent,
                wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize);

    // 设置安全等级
    void SetSafetyRating(IceClean::Models::SafetyRating rating);
    IceClean::Models::SafetyRating GetSafetyRating() const { return m_rating; }

    // 获取等级对应的显示文本
    static wxString GetRatingText(IceClean::Models::SafetyRating rating);
    // 获取等级对应的颜色
    static wxColour GetRatingColor(IceClean::Models::SafetyRating rating);

private:
    IceClean::Models::SafetyRating m_rating = IceClean::Models::SafetyRating::Safe;

    void OnPaint(wxPaintEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
