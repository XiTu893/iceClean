#include "NavSidebar.h"
#include "ThemeManager.h"

#include <wx/dcbuffer.h>
#include <wx/graphics.h>

namespace IceClean::Gui {

wxDEFINE_EVENT(wxEVT_NAV_SELECTION_CHANGED, wxCommandEvent);

NavSidebar::NavSidebar(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(200, -1))
{
    const auto& colors = ThemeManager::Instance().GetColors();
    m_bgColor = colors.sidebar;
    m_selectedColor = colors.sidebarSelected;
    m_hoverColor = colors.sidebarHover;
    m_textColor = colors.sidebarText;
    m_selectedTextColor = colors.sidebarSelectedText;

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(m_bgColor);
    SetName("NavSidebar");
    SetMinSize(wxSize(200, -1));

    // 13 items with separators at index 4 and 8
    m_items = {
        { L"首页",       0 },
        { L"深度清理",   1 },
        { L"智能迁移",   2 },
        { L"加速优化",   3 },
        { L"",          -1 },  // separator
        { L"软件管理",   4 },
        { L"软件推荐",   5 },
        { L"安全防护",   6 },
        { L"网络优化",   7 },
        { L"",          -1 },  // separator
        { L"磁盘分析",   8 },
        { L"文件分类",   9 },
        { L"下载管理",  10 },
        { L"设置",      11 },
        { L"关于",      12 },
    };

    Bind(wxEVT_PAINT, &NavSidebar::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &NavSidebar::OnLeftDown, this);
    Bind(wxEVT_MOTION, &NavSidebar::OnMouseMove, this);
    Bind(wxEVT_LEAVE_WINDOW, &NavSidebar::OnMouseLeave, this);

    ThemeManager::Instance().RegisterChangeCallback([this](const ThemeColors& newColors) {
        m_bgColor = newColors.sidebar;
        m_selectedColor = newColors.sidebarSelected;
        m_hoverColor = newColors.sidebarHover;
        m_textColor = newColors.sidebarText;
        m_selectedTextColor = newColors.sidebarSelectedText;
        SetBackgroundColour(m_bgColor);
        Refresh();
    });
}

void NavSidebar::SetSelection(int index)
{
    if (index >= 0 && index < static_cast<int>(m_items.size()) &&
        index != m_selection && m_items[index].iconId >= 0) {
        m_selection = index;
        Refresh();

        wxCommandEvent event(wxEVT_NAV_SELECTION_CHANGED, GetId());
        event.SetInt(m_selection);
        event.SetEventObject(this);
        ProcessWindowEvent(event);
    }
}

// ── Icon drawing methods ──

void NavSidebar::DrawHomeIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath roof = gc->CreatePath();
    roof.MoveToPoint(cx, cy - s);
    roof.AddLineToPoint(cx - s, cy - s * 0.1);
    roof.AddLineToPoint(cx + s, cy - s * 0.1);
    roof.CloseSubpath();
    gc->StrokePath(roof);
    wxGraphicsPath body = gc->CreatePath();
    body.AddRectangle(cx - s * 0.7, cy - s * 0.1, s * 1.4, s * 0.9);
    gc->StrokePath(body);
    wxGraphicsPath door = gc->CreatePath();
    door.AddRectangle(cx - s * 0.2, cy + s * 0.2, s * 0.4, s * 0.6);
    gc->StrokePath(door);
}

void NavSidebar::DrawCleanIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->StrokeLine(cx - s * 0.5, cy - s, cx + s * 0.3, cy + s * 0.2);
    wxGraphicsPath brush = gc->CreatePath();
    brush.MoveToPoint(cx - s * 0.8, cy + s * 0.2);
    brush.AddLineToPoint(cx + s * 0.3, cy + s * 0.2);
    brush.AddLineToPoint(cx + s * 0.5, cy + s);
    brush.AddLineToPoint(cx - s, cy + s);
    brush.CloseSubpath();
    gc->StrokePath(brush);
    for (int i = 0; i < 3; ++i) {
        double x = cx - s * 0.6 + i * s * 0.4;
        gc->StrokeLine(x, cy + s * 0.4, x, cy + s * 0.9);
    }
}

void NavSidebar::DrawMigrateIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath box = gc->CreatePath();
    box.AddRectangle(cx - s, cy - s * 0.5, s * 1.2, s * 1.0);
    gc->StrokePath(box);
    wxGraphicsPath arrow = gc->CreatePath();
    arrow.MoveToPoint(cx, cy);
    arrow.AddLineToPoint(cx + s * 0.8, cy);
    arrow.MoveToPoint(cx + s * 0.3, cy - s * 0.4);
    arrow.AddLineToPoint(cx + s * 0.8, cy);
    arrow.AddLineToPoint(cx + s * 0.3, cy + s * 0.4);
    gc->StrokePath(arrow);
}

void NavSidebar::DrawSpeedupIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.45;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath bolt = gc->CreatePath();
    bolt.MoveToPoint(cx + s * 0.1, cy - s);
    bolt.AddLineToPoint(cx - s * 0.4, cy + s * 0.05);
    bolt.AddLineToPoint(cx + s * 0.05, cy + s * 0.05);
    bolt.AddLineToPoint(cx - s * 0.1, cy + s);
    bolt.AddLineToPoint(cx + s * 0.4, cy - s * 0.05);
    bolt.AddLineToPoint(cx - s * 0.05, cy - s * 0.05);
    bolt.CloseSubpath();
    gc->StrokePath(bolt);
}

void NavSidebar::DrawSettingsIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    double outerR = s * 0.9;
    double innerR = s * 0.4;
    int toothCount = 6;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    for (int i = 0; i < toothCount; ++i) {
        double angle = i * M_PI * 2.0 / toothCount;
        double x1 = cx + cos(angle) * (outerR - s * 0.1);
        double y1 = cy + sin(angle) * (outerR - s * 0.1);
        double x2 = cx + cos(angle) * (outerR + s * 0.2);
        double y2 = cy + sin(angle) * (outerR + s * 0.2);
        gc->StrokeLine(x1, y1, x2, y2);
    }
    wxGraphicsPath outer = gc->CreatePath();
    outer.AddEllipse(cx - outerR, cy - outerR, outerR * 2, outerR * 2);
    gc->StrokePath(outer);
    wxGraphicsPath inner = gc->CreatePath();
    inner.AddEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);
    gc->StrokePath(inner);
}

void NavSidebar::DrawAboutIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    double r = s * 0.85;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath circle = gc->CreatePath();
    circle.AddEllipse(cx - r, cy - r, r * 2, r * 2);
    gc->StrokePath(circle);
    double dotR = s * 0.12;
    wxGraphicsPath dot = gc->CreatePath();
    dot.AddEllipse(cx - dotR, cy - s * 0.5 - dotR, dotR * 2, dotR * 2);
    gc->FillPath(dot);
    gc->StrokePath(dot);
    gc->StrokeLine(cx, cy - s * 0.2, cx, cy + s * 0.5);
}

void NavSidebar::DrawUninstallIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath box = gc->CreatePath();
    box.AddRectangle(cx - s, cy - s * 0.6, s * 1.4, s * 1.2);
    gc->StrokePath(box);
    gc->StrokeLine(cx + s * 0.2, cy - s * 0.3, cx + s * 0.8, cy + s * 0.3);
    gc->StrokeLine(cx + s * 0.8, cy - s * 0.3, cx + s * 0.2, cy + s * 0.3);
}

void NavSidebar::DrawNetworkIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    double r = s * 0.7;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath globe = gc->CreatePath();
    globe.AddEllipse(cx - r, cy - r, r * 2, r * 2);
    gc->StrokePath(globe);
    gc->StrokeLine(cx - r, cy, cx + r, cy);
    wxGraphicsPath meridian = gc->CreatePath();
    meridian.AddEllipse(cx - r * 0.4, cy - r, r * 0.8, r * 2);
    gc->StrokePath(meridian);
}

void NavSidebar::DrawPopupBlockerIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath shield = gc->CreatePath();
    shield.MoveToPoint(cx, cy - s);
    shield.AddLineToPoint(cx + s * 0.8, cy - s * 0.5);
    shield.AddLineToPoint(cx + s * 0.8, cy + s * 0.2);
    shield.AddLineToPoint(cx, cy + s);
    shield.AddLineToPoint(cx - s * 0.8, cy + s * 0.2);
    shield.AddLineToPoint(cx - s * 0.8, cy - s * 0.5);
    shield.CloseSubpath();
    gc->StrokePath(shield);
    gc->StrokeLine(cx - s * 0.3, cy - s * 0.2, cx + s * 0.3, cy + s * 0.3);
    gc->StrokeLine(cx + s * 0.3, cy - s * 0.2, cx - s * 0.3, cy + s * 0.3);
}

void NavSidebar::DrawRecommendIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    wxGraphicsPath star = gc->CreatePath();
    double outerR = s * 0.9;
    double innerR = s * 0.4;
    for (int i = 0; i < 5; ++i) {
        double outerAngle = -M_PI / 2.0 + i * 2.0 * M_PI / 5.0;
        double innerAngle = outerAngle + M_PI / 5.0;
        double ox = cx + cos(outerAngle) * outerR;
        double oy = cy + sin(outerAngle) * outerR;
        double ix = cx + cos(innerAngle) * innerR;
        double iy = cy + sin(innerAngle) * innerR;
        if (i == 0) star.MoveToPoint(ox, oy);
        else star.AddLineToPoint(ox, oy);
        star.AddLineToPoint(ix, iy);
    }
    star.CloseSubpath();
    gc->StrokePath(star);
}

void NavSidebar::DrawDiskIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    // 硬盘：椭圆 + 内部同心圆
    wxGraphicsPath disk = gc->CreatePath();
    disk.AddEllipse(cx - s, cy - s * 0.7, s * 2, s * 1.4);
    gc->StrokePath(disk);

    wxGraphicsPath inner = gc->CreatePath();
    inner.AddEllipse(cx - s * 0.5, cy - s * 0.35, s, s * 0.7);
    gc->StrokePath(inner);

    gc->StrokeLine(cx - s * 1.1, cy - s * 0.2, cx - s * 0.6, cy - s * 0.2);
    gc->StrokeLine(cx - s * 1.1, cy + s * 0.2, cx - s * 0.6, cy + s * 0.2);
}

void NavSidebar::DrawFileTypeIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    // 文档 + 分类线条
    wxGraphicsPath doc = gc->CreatePath();
    doc.AddRectangle(cx - s * 0.6, cy - s, s * 1.2, s * 2.0);
    gc->StrokePath(doc);

    // 文件夹标签
    gc->StrokeLine(cx - s * 0.6, cy - s * 0.7, cx + s * 0.3, cy - s * 0.7);
    gc->StrokeLine(cx - s * 0.6, cy - s * 0.3, cx + s * 0.4, cy - s * 0.3);

    // 分类线条（3条）
    gc->StrokeLine(cx - s * 0.35, cy + s * 0.1, cx + s * 0.35, cy + s * 0.1);
    gc->StrokeLine(cx - s * 0.35, cy + s * 0.4, cx + s * 0.35, cy + s * 0.4);
    gc->StrokeLine(cx - s * 0.35, cy + s * 0.7, cx + s * 0.2, cy + s * 0.7);
}

void NavSidebar::DrawDownloadIcon(wxGraphicsContext* gc, double cx, double cy, double size, const wxColour& color) {
    double s = size * 0.4;
    gc->SetPen(gc->CreatePen(wxPen(color, 2.0)));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    // 向下箭头 + 底横线
    wxGraphicsPath arrow = gc->CreatePath();
    arrow.MoveToPoint(cx, cy + s);
    arrow.AddLineToPoint(cx - s * 0.5, cy + s * 0.2);
    arrow.MoveToPoint(cx, cy + s);
    arrow.AddLineToPoint(cx + s * 0.5, cy + s * 0.2);
    gc->StrokePath(arrow);

    // 竖线（箭头杆）
    gc->StrokeLine(cx, cy - s, cx, cy + s * 0.6);

    // 底部横线
    gc->StrokeLine(cx - s * 0.7, cy + s * 1.2, cx + s * 0.7, cy + s * 1.2);
}

// ── Paint ──

void NavSidebar::OnPaint(wxPaintEvent& /*event*/)
{
    wxBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);

    if (!gc) {
        dc.SetBackground(m_bgColor);
        dc.Clear();
        return;
    }

    const auto& colors = ThemeManager::Instance().GetColors();

    // 渐变背景
    wxColour gradStart = colors.sidebar;
    wxColour gradEnd = colors.sidebarGradientEnd;
    wxGraphicsBrush gradientBrush = gc->CreateLinearGradientBrush(
        0, 0, 0, GetSize().y, gradStart, gradEnd);
    gc->SetBrush(gradientBrush);
    gc->DrawRectangle(wxRect2DDouble(0, 0, GetSize().x, GetSize().y));

    // 品牌标识
    gc->SetFont(wxFont(13, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"),
                colors.sidebarText);
    gc->DrawText("IceClean", PADDING, 8);

    // 分割线
    int titleAreaHeight = 36;
    gc->SetPen(gc->CreatePen(wxPen(wxColour(255, 255, 255, 40), 1)));
    gc->StrokeLine(PADDING, titleAreaHeight, GetSize().x - PADDING, titleAreaHeight);

    // 图标颜色映射（13个非分隔项）
    const wxColour itemColors[] = {
        wxColour(120, 175, 255),    // 0: 首页 - 蓝
        wxColour(100, 230, 180),    // 1: 深度清理 - 绿
        wxColour(255, 170, 100),    // 2: 智能迁移 - 橙
        wxColour(255, 220, 100),    // 3: 加速优化 - 黄
        wxColour(255, 140, 140),    // 4: 软件管理 - 红
        wxColour(190, 165, 255),    // 5: 软件推荐 - 紫
        wxColour(255, 150, 180),    // 6: 安全防护 - 玫瑰
        wxColour(120, 235, 185),    // 7: 网络优化 - 翡翠
        wxColour(80, 220, 130),     // 8: 磁盘分析 - 亮绿
        wxColour(200, 160, 255),    // 9: 文件分类 - 淡紫
        wxColour(100, 195, 255),    // 10: 下载管理 - 天蓝
        wxColour(180, 186, 200),    // 11: 设置 - 灰
        wxColour(140, 185, 255),    // 12: 关于 - 浅蓝
    };

    // Draw navigation items
    int y = titleAreaHeight + 8;
    int drawIndex = 0;  // 仅非分隔项有图标颜色索引

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        // 分隔符
        if (m_items[i].iconId < 0) {
            y += SEPARATOR_HEIGHT;
            // 画分隔线
            gc->SetPen(gc->CreatePen(wxPen(wxColour(255, 255, 255, 30), 1)));
            gc->StrokeLine(PADDING * 1.5, y - SEPARATOR_HEIGHT / 2,
                          GetSize().x - PADDING * 1.5, y - SEPARATOR_HEIGHT / 2);
            continue;
        }

        int itemY = y;
        wxRect itemRect(PADDING / 2, itemY, GetSize().x - PADDING, ITEM_HEIGHT);

        // Hover/selection background
        if (i == m_selection) {
            wxGraphicsBrush selBrush = gc->CreateLinearGradientBrush(
                itemRect.x, itemRect.y, itemRect.x + itemRect.width, itemRect.y,
                colors.sidebarSelected, colors.accentGradientEnd);
            gc->SetBrush(selBrush);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRoundedRectangle(itemRect.x, itemRect.y, itemRect.width, itemRect.height, 8);
        } else if (i == m_hoverItem) {
            gc->SetBrush(gc->CreateBrush(wxBrush(m_hoverColor)));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRoundedRectangle(itemRect.x, itemRect.y, itemRect.width, itemRect.height, 6);
        }

        // Draw icon
        int iconX = itemRect.x + PADDING + ICON_SIZE / 2;
        int iconY = itemRect.y + ITEM_HEIGHT / 2;

        wxColour iconColor = itemColors[drawIndex];
        if (i == m_selection) iconColor = colors.sidebarSelectedText;

        // 根据项目索引选择正确的图标绘制
        int idx = m_items[i].iconId;
        switch (idx) {
            case 0: DrawHomeIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 1: DrawCleanIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 2: DrawMigrateIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 3: DrawSpeedupIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 4: DrawUninstallIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 5: DrawRecommendIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 6: DrawPopupBlockerIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 7: DrawNetworkIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 8: DrawDiskIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 9: DrawFileTypeIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 10: DrawDownloadIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 11: DrawSettingsIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
            case 12: DrawAboutIcon(gc, iconX, iconY, ICON_SIZE, iconColor); break;
        }

        // Draw label text
        wxColour textColor = (i == m_selection) ? colors.sidebarSelectedText : m_textColor;
        if (i == m_selection) {
            gc->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD), textColor);
        } else {
            gc->SetFont(wxFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL), textColor);
        }
        gc->DrawText(m_items[i].label, itemRect.x + PADDING + ICON_SIZE + 10,
                     itemY + (ITEM_HEIGHT - 20) / 2);

        y += ITEM_HEIGHT;
        drawIndex++;
    }

    delete gc;
}

// ── Mouse events ──

void NavSidebar::OnLeftDown(wxMouseEvent& event)
{
    int titleAreaHeight = 36;
    int y = titleAreaHeight + 8;

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].iconId < 0) {
            y += SEPARATOR_HEIGHT;
            continue;
        }
        wxRect itemRect(0, y, GetSize().x, ITEM_HEIGHT);
        if (itemRect.Contains(event.GetPosition())) {
            SetSelection(i);
            return;
        }
        y += ITEM_HEIGHT;
    }
}

void NavSidebar::OnMouseMove(wxMouseEvent& event)
{
    int titleAreaHeight = 36;
    int y = titleAreaHeight + 8;
    int newHover = -1;

    for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
        if (m_items[i].iconId < 0) {
            y += SEPARATOR_HEIGHT;
            continue;
        }
        wxRect itemRect(0, y, GetSize().x, ITEM_HEIGHT);
        if (itemRect.Contains(event.GetPosition())) {
            newHover = i;
            break;
        }
        y += ITEM_HEIGHT;
    }

    if (newHover != m_hoverItem) {
        m_hoverItem = newHover;
        Refresh();
    }
}

void NavSidebar::OnMouseLeave(wxMouseEvent& /*event*/)
{
    if (m_hoverItem != -1) {
        m_hoverItem = -1;
        Refresh();
    }
}

} // namespace IceClean::Gui
