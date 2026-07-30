#include "CustomTitleBar.h"
#include "ThemeManager.h"

namespace IceClean::Gui {

// 标题栏高度
static constexpr int TITLE_BAR_HEIGHT = 36;
// 按钮尺寸
static constexpr int BTN_SIZE = 46;

wxBEGIN_EVENT_TABLE(CustomTitleBar, wxPanel)
    EVT_PAINT(CustomTitleBar::OnPaint)
    EVT_LEFT_DOWN(CustomTitleBar::OnLeftDown)
    EVT_LEFT_UP(CustomTitleBar::OnLeftUp)
    EVT_MOTION(CustomTitleBar::OnMouseMove)
    EVT_LEAVE_WINDOW(CustomTitleBar::OnMouseLeave)
wxEND_EVENT_TABLE()

CustomTitleBar::CustomTitleBar(wxWindow* parent, wxFrame* ownerFrame)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, TITLE_BAR_HEIGHT))
    , m_ownerFrame(ownerFrame)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.sidebar);
    SetMinSize(wxSize(-1, TITLE_BAR_HEIGHT));
    SetMaxSize(wxSize(-1, TITLE_BAR_HEIGHT));

    // 右侧按钮容器
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    // 最小化按钮
    m_minBtn = new wxButton(this, wxID_ANY, L"─", wxDefaultPosition, wxSize(BTN_SIZE, TITLE_BAR_HEIGHT),
                            wxBORDER_NONE | wxBU_EXACTFIT);
    m_minBtn->SetBackgroundColour(colors.sidebar);
    m_minBtn->SetForegroundColour(colors.sidebarText);
    m_minBtn->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    m_minBtn->Bind(wxEVT_BUTTON, &CustomTitleBar::OnMinimize, this);
    // 悬停效果
    m_minBtn->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) {
        m_minBtn->SetBackgroundColour(wxColour(70, 90, 140));
        m_minBtn->SetForegroundColour(*wxWHITE);
    });
    m_minBtn->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
        const auto& c = ThemeManager::Instance().GetColors();
        m_minBtn->SetBackgroundColour(c.sidebar);
        m_minBtn->SetForegroundColour(c.sidebarText);
    });

    // 最大化/还原按钮
    m_maxBtn = new wxButton(this, wxID_ANY, L"□", wxDefaultPosition, wxSize(BTN_SIZE, TITLE_BAR_HEIGHT),
                            wxBORDER_NONE | wxBU_EXACTFIT);
    m_maxBtn->SetBackgroundColour(colors.sidebar);
    m_maxBtn->SetForegroundColour(colors.sidebarText);
    m_maxBtn->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    m_maxBtn->Bind(wxEVT_BUTTON, &CustomTitleBar::OnMaximize, this);
    m_maxBtn->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) {
        m_maxBtn->SetBackgroundColour(wxColour(70, 90, 140));
        m_maxBtn->SetForegroundColour(*wxWHITE);
    });
    m_maxBtn->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
        const auto& c = ThemeManager::Instance().GetColors();
        m_maxBtn->SetBackgroundColour(c.sidebar);
        m_maxBtn->SetForegroundColour(c.sidebarText);
    });

    // 关闭按钮
    m_closeBtn = new wxButton(this, wxID_ANY, L"✕", wxDefaultPosition, wxSize(BTN_SIZE, TITLE_BAR_HEIGHT),
                              wxBORDER_NONE | wxBU_EXACTFIT);
    m_closeBtn->SetBackgroundColour(colors.sidebar);
    m_closeBtn->SetForegroundColour(colors.sidebarText);
    m_closeBtn->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    m_closeBtn->Bind(wxEVT_BUTTON, &CustomTitleBar::OnClose, this);
    // 关闭按钮悬停变红
    m_closeBtn->Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) {
        m_closeBtn->SetBackgroundColour(wxColour(232, 17, 35));
        m_closeBtn->SetForegroundColour(*wxWHITE);
    });
    m_closeBtn->Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) {
        const auto& c = ThemeManager::Instance().GetColors();
        m_closeBtn->SetBackgroundColour(c.sidebar);
        m_closeBtn->SetForegroundColour(c.sidebarText);
    });

    btnSizer->Add(m_minBtn, 0, wxEXPAND);
    btnSizer->Add(m_maxBtn, 0, wxEXPAND);
    btnSizer->Add(m_closeBtn, 0, wxEXPAND);

    // 整体布局：左侧标题 + 右侧按钮
    auto* mainSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->AddStretchSpacer();  // 标题由 OnPaint 绘制
    mainSizer->Add(btnSizer, 0, wxEXPAND);

    SetSizer(mainSizer);

    // 注册主题变更回调
    ThemeManager::Instance().RegisterChangeCallback(
        [this](const ThemeColors& colors) { OnThemeChanged(colors); });
}

void CustomTitleBar::SetTitle(const wxString& title) {
    m_title = title;
    Refresh();
}

void CustomTitleBar::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    const auto& colors = ThemeManager::Instance().GetColors();

    // 渐变背景（与侧边栏同色系）
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (gc) {
        wxGraphicsBrush bgBrush = gc->CreateLinearGradientBrush(
            0, 0, 0, GetSize().y,
            colors.sidebar, colors.sidebarGradientEnd);
        gc->SetBrush(bgBrush);
        gc->DrawRectangle(wxRect2DDouble(0, 0, GetSize().x, GetSize().y));

        // 标题文字 - 在按钮区域左侧绘制
        gc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"),
                    colors.sidebarText);
        double textW = 0, textH = 0;
        gc->GetTextExtent(m_title, &textW, &textH);
        // 标题居中偏左（避开按钮区域）
        double textX = 16;
        double textY = (TITLE_BAR_HEIGHT - textH) / 2.0;
        gc->DrawText(m_title, textX, textY);

        delete gc;
    }
}

// ── 拖拽移动窗口 ──

void CustomTitleBar::OnLeftDown(wxMouseEvent& event) {
    m_isDragging = true;
    m_dragStartPos = event.GetPosition();
    CaptureMouse();
}

void CustomTitleBar::OnLeftUp(wxMouseEvent& event) {
    if (m_isDragging) {
        m_isDragging = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    }
}

void CustomTitleBar::OnMouseMove(wxMouseEvent& event) {
    if (m_isDragging && event.Dragging() && event.LeftIsDown()) {
        wxPoint currentPos = ClientToScreen(event.GetPosition());
        wxPoint framePos = m_ownerFrame->GetPosition();
        wxPoint delta = event.GetPosition() - m_dragStartPos;
        m_ownerFrame->Move(framePos + delta);
    }
}

void CustomTitleBar::OnMouseLeave(wxMouseEvent& event) {
    if (m_isDragging) {
        m_isDragging = false;
        if (HasCapture()) {
            ReleaseMouse();
        }
    }
}

// ── 按钮事件 ──

void CustomTitleBar::OnMinimize(wxCommandEvent& event) {
    m_ownerFrame->Iconize(true);
}

void CustomTitleBar::OnMaximize(wxCommandEvent& event) {
    if (m_ownerFrame->IsMaximized()) {
        m_ownerFrame->Restore();
        m_maxBtn->SetLabel(L"□");
    } else {
        m_ownerFrame->Maximize();
        m_maxBtn->SetLabel(L"❐");
    }
}

void CustomTitleBar::OnClose(wxCommandEvent& event) {
    m_ownerFrame->Close();
}

// ── 主题变更 ──

void CustomTitleBar::OnThemeChanged(const ThemeColors& colors) {
    SetBackgroundColour(colors.sidebar);
    m_minBtn->SetBackgroundColour(colors.sidebar);
    m_minBtn->SetForegroundColour(colors.sidebarText);
    m_maxBtn->SetBackgroundColour(colors.sidebar);
    m_maxBtn->SetForegroundColour(colors.sidebarText);
    m_closeBtn->SetBackgroundColour(colors.sidebar);
    m_closeBtn->SetForegroundColour(colors.sidebarText);
    Refresh();
}

} // namespace IceClean::Gui
