#pragma once
#include <wx/wx.h>
#include <wx/graphics.h>

namespace IceClean::Gui {

// 圆形进度条控件（带动画）
class CircularProgress : public wxPanel {
public:
    CircularProgress(wxWindow* parent,
                     wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize);

    ~CircularProgress();

    // 设置进度值 (0-100)，带平滑动画
    void SetValue(int value);
    int GetValue() const { return m_value; }

    // 设置进度弧颜色
    void SetProgressColor(const wxColour& color);
    const wxColour& GetProgressColor() const { return m_progressColor; }

    // 设置标签文字 (显示在圆心)
    void SetLabel(const wxString& label);
    wxString GetLabel() const override { return m_label; }

    // 设置副标签 (显示在主标签下方)
    void SetSubLabel(const wxString& subLabel);
    const wxString& GetSubLabel() const { return m_subLabel; }

    // 设置不确定模式（旋转动画，用于扫描中等无法确定进度的场景）
    void SetIndeterminate(bool indeterminate);
    bool IsIndeterminate() const { return m_indeterminate; }

private:
    int m_value = 0;
    int m_displayValue = 0;  // 动画当前显示值
    int m_targetValue = 0;   // 动画目标值
    wxColour m_progressColor;  // 由构造函数从 ThemeManager 初始化
    wxColour m_trackColor;     // 由构造函数从 ThemeManager 初始化
    wxString m_label;
    wxString m_subLabel;

    // 动画相关
    bool m_indeterminate = false;
    double m_indetAngle = 0.0;  // 不确定模式旋转角度
    wxTimer* m_animTimer = nullptr;
    static const int ANIM_INTERVAL = 16;  // ~60fps
    static const int ANIM_DURATION = 300; // 动画持续时间(ms)
    int m_animElapsed = 0;
    int m_animStartValue = 0;

    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnAnimTimer(wxTimerEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
