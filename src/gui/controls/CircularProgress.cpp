#include "CircularProgress.h"
#include "ThemeManager.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <wx/dcbuffer.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(CircularProgress, wxPanel)
    EVT_PAINT(CircularProgress::OnPaint)
    EVT_SIZE(CircularProgress::OnSize)
wxEND_EVENT_TABLE()

CircularProgress::CircularProgress(wxWindow* parent, wxWindowID id,
                                   const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE)
    , m_progressColor(ThemeManager::Instance().GetColors().progressBar)
    , m_trackColor(ThemeManager::Instance().GetColors().progressBarBg)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(120, 120));

    m_animTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &CircularProgress::OnAnimTimer, this, m_animTimer->GetId());

    // 注册主题变更回调
    ThemeManager::Instance().RegisterChangeCallback([this](const ThemeColors& newColors) {
        m_trackColor = newColors.progressBarBg;
        // 注意：m_progressColor 由面板逻辑根据状态设置，不自动跟随主题
        Refresh();
    });
}

CircularProgress::~CircularProgress() {
    if (m_animTimer) {
        m_animTimer->Stop();
        delete m_animTimer;
        m_animTimer = nullptr;
    }
}

void CircularProgress::SetValue(int value) {
    m_value = std::clamp(value, 0, 100);
    m_targetValue = m_value;

    // 启动动画：从当前显示值平滑过渡到目标值
    m_animStartValue = m_displayValue;
    m_animElapsed = 0;

    if (!m_animTimer->IsRunning()) {
        m_animTimer->Start(ANIM_INTERVAL);
    }
}

void CircularProgress::SetProgressColor(const wxColour& color) {
    m_progressColor = color;
    Refresh();
}

void CircularProgress::SetLabel(const wxString& label) {
    m_label = label;
    Refresh();
}

void CircularProgress::SetSubLabel(const wxString& subLabel) {
    m_subLabel = subLabel;
    Refresh();
}

void CircularProgress::SetIndeterminate(bool indeterminate) {
    m_indeterminate = indeterminate;
    if (m_indeterminate) {
        m_indetAngle = 0.0;
        if (!m_animTimer->IsRunning()) {
            m_animTimer->Start(ANIM_INTERVAL);
        }
    } else {
        // 停止旋转动画（如果值动画也完成了）
        if (m_displayValue == m_targetValue) {
            m_animTimer->Stop();
        }
    }
}

void CircularProgress::OnAnimTimer(wxTimerEvent& /*event*/) {
    bool needContinue = false;

    if (m_indeterminate) {
        // 不确定模式：旋转弧段
        m_indetAngle += 0.08;  // 旋转速度
        if (m_indetAngle > 2.0 * M_PI) {
            m_indetAngle -= 2.0 * M_PI;
        }
        needContinue = true;
    }

    // 值动画
    if (m_displayValue != m_targetValue) {
        m_animElapsed += ANIM_INTERVAL;

        // 缓动函数 (ease-out)
        double t = std::min(1.0, static_cast<double>(m_animElapsed) / ANIM_DURATION);
        double eased = 1.0 - (1.0 - t) * (1.0 - t);  // quadratic ease-out

        int range = m_targetValue - m_animStartValue;
        m_displayValue = m_animStartValue + static_cast<int>(range * eased);

        if (m_displayValue == m_targetValue || t >= 1.0) {
            m_displayValue = m_targetValue;
        } else {
            needContinue = true;
        }
    }

    Refresh();

    if (!needContinue) {
        m_animTimer->Stop();
    }
}

void CircularProgress::OnPaint(wxPaintEvent& event) {
    const auto& colors = ThemeManager::Instance().GetColors();
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(colors.background));
    dc.Clear();

    auto gc = std::unique_ptr<wxGraphicsContext>(wxGraphicsContext::Create(dc));
    if (!gc) return;

    const int w = GetSize().GetWidth();
    const int h = GetSize().GetHeight();
    const int side = wxMin(w, h);
    const int lineWidth = wxMax(side / 10, 6);
    const int margin = lineWidth / 2 + 2;
    const int diameter = side - 2 * margin;
    const double radius = static_cast<double>(diameter) / 2.0;
    const double cx = w / 2.0;
    const double cy = h / 2.0;

    // 绘制背景轨道
    gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(m_trackColor, lineWidth, wxPENSTYLE_SOLID)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath trackPath = gc->CreatePath();
    trackPath.AddCircle(cx, cy, radius);
    gc->StrokePath(trackPath);

    if (m_indeterminate) {
        // 不确定模式：绘制旋转弧段（约90度的弧段持续旋转）
        double arcSpan = M_PI / 2.0;  // 90度弧段
        double startAngle = m_indetAngle - M_PI / 2.0;
        double endAngle = startAngle + arcSpan;

        // 渐变进度弧
        wxGraphicsBrush progressBrush = gc->CreateLinearGradientBrush(
            cx - side/2, cy - side/2, cx + side/2, cy + side/2,
            m_progressColor, colors.accentGradientEnd);
        gc->SetBrush(progressBrush);
        gc->SetPen(*wxTRANSPARENT_PEN);

        wxGraphicsPath progressPath = gc->CreatePath();
        progressPath.AddArc(cx, cy, radius, startAngle, endAngle, false);
        gc->StrokePath(progressPath);

        // 在弧段末端绘制发光点
        double glowX = cx + radius * cos(endAngle);
        double glowY = cy + radius * sin(endAngle);
        wxColour glowColor = m_progressColor;
        wxGraphicsPath glowPath = gc->CreatePath();
        glowPath.AddCircle(glowX, glowY, lineWidth * 0.8);
        gc->SetBrush(gc->CreateBrush(wxBrush(wxColour(glowColor.Red(), glowColor.Green(), glowColor.Blue(), 80))));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->FillPath(glowPath);
    } else if (m_displayValue > 0) {
        // 正常模式：绘制渐变进度弧
        double startAngle = -M_PI / 2.0;
        double sweepAngle = (m_displayValue / 100.0) * 2.0 * M_PI;

        wxGraphicsBrush progressBrush = gc->CreateLinearGradientBrush(
            cx - side/2, cy - side/2, cx + side/2, cy + side/2,
            m_progressColor, colors.accentGradientEnd);
        gc->SetBrush(progressBrush);
        gc->SetPen(gc->CreatePen(wxGraphicsPenInfo(m_progressColor, lineWidth, wxPENSTYLE_SOLID)
                                     .Cap(wxCAP_ROUND)));
        wxGraphicsPath progressPath = gc->CreatePath();
        progressPath.AddArc(cx, cy, radius, startAngle, startAngle + sweepAngle, false);
        gc->StrokePath(progressPath);

        // 绘制进度弧末端的发光效果
        double endAngle = startAngle + sweepAngle;
        double glowX = cx + cos(endAngle) * (side/2 - lineWidth);
        double glowY = cy + sin(endAngle) * (side/2 - lineWidth);

        // 发光光晕
        gc->SetBrush(gc->CreateBrush(wxBrush(wxColour(colors.accent.Red(), colors.accent.Green(), colors.accent.Blue(), 60))));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawEllipse(glowX - lineWidth, glowY - lineWidth, lineWidth * 2, lineWidth * 2);

        // 实心圆点
        gc->SetBrush(gc->CreateBrush(wxBrush(colors.accent)));
        gc->DrawEllipse(glowX - lineWidth/3, glowY - lineWidth/3, lineWidth*2/3, lineWidth*2/3);
    }

    // 绘制中心文字
    if (!m_label.IsEmpty()) {
        wxFont labelFont(wxMax(side / 5, 12), wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                         false, L"微软雅黑");
        gc->SetFont(labelFont, colors.textPrimary);
        double textW, textH;
        gc->GetTextExtent(m_label, &textW, &textH);

        double labelY = cy - textH / 2.0;
        if (!m_subLabel.IsEmpty()) {
            labelY -= textH * 0.15;
        }
        gc->DrawText(m_label, (w - textW) / 2.0, labelY);
    }

    if (!m_subLabel.IsEmpty()) {
        wxFont subFont(wxMax(side / 10, 8), wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                       false, L"微软雅黑");
        gc->SetFont(subFont, colors.textSecondary);
        double subW, subH;
        gc->GetTextExtent(m_subLabel, &subW, &subH);

        double mainH = 0;
        if (!m_label.IsEmpty()) {
            gc->GetTextExtent(m_label, nullptr, &mainH);
        }
        double subY = cy - mainH / 2.0 + mainH * 0.85;
        gc->DrawText(m_subLabel, (w - subW) / 2.0, subY);
    }
}

void CircularProgress::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

} // namespace IceClean::Gui
