#include "StartupPanel.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/Events.h"

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(StartupPanel, wxPanel)
wxEND_EVENT_TABLE()

StartupPanel::StartupPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(*wxWHITE);
    CreateControls();
}

void StartupPanel::CreateControls() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"启动加速");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    // 预估启动时间
    auto* bootSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* bootIcon = new wxStaticText(this, wxID_ANY, L"⏱");
    bootIcon->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    bootSizer->Add(bootIcon, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_bootTimeLabel = new wxStaticText(this, wxID_ANY, L"预估启动时间: 计算中...");
    m_bootTimeLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
    m_bootTimeLabel->SetForegroundColour(wxColour(0x00, 0x78, 0xD4));
    bootSizer->Add(m_bootTimeLabel, 0, wxALIGN_CENTER_VERTICAL);
    mainSizer->Add(bootSizer, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // ── 启动项区域 ──
    auto* startupLabel = new wxStaticText(this, wxID_ANY, L"启动项");
    startupLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    startupLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    mainSizer->Add(startupLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    m_startupScroller = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 200),
                                              wxVSCROLL | wxBORDER_NONE);
    m_startupScroller->SetBackgroundColour(*wxWHITE);
    m_startupScroller->SetScrollRate(0, 10);
    m_startupSizer = new wxBoxSizer(wxVERTICAL);
    m_startupScroller->SetSizer(m_startupSizer);
    mainSizer->Add(m_startupScroller, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // ── 服务区域 ──
    auto* serviceLabel = new wxStaticText(this, wxID_ANY, L"服务");
    serviceLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    serviceLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    mainSizer->Add(serviceLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    m_serviceScroller = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 200),
                                              wxVSCROLL | wxBORDER_NONE);
    m_serviceScroller->SetBackgroundColour(*wxWHITE);
    m_serviceScroller->SetScrollRate(0, 10);
    m_serviceSizer = new wxBoxSizer(wxVERTICAL);
    m_serviceScroller->SetSizer(m_serviceSizer);
    mainSizer->Add(m_serviceScroller, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // ── 底部优化按钮 ──
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    bottomSizer->AddStretchSpacer();

    m_optimizeButton = new wxButton(this, wxID_ANY, L"优化", wxDefaultPosition, wxSize(140, 40));
    m_optimizeButton->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                     false, L"微软雅黑"));
    m_optimizeButton->SetBackgroundColour(wxColour(0x00, 0x78, 0xD4));
    m_optimizeButton->SetForegroundColour(*wxWHITE);
    m_optimizeButton->Bind(wxEVT_BUTTON, &StartupPanel::OnOptimizeButton, this);
    bottomSizer->Add(m_optimizeButton, 0, wxRIGHT, 20);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxBOTTOM, 12);

    SetSizer(mainSizer);
}

wxPanel* StartupPanel::CreateToggleSwitch(wxWindow* parent, bool isOn, bool canToggle) {
    auto* panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(44, 22));
    panel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    panel->SetCursor(canToggle ? wxCURSOR_HAND : wxCURSOR_ARROW);
    DrawToggle(panel, isOn, canToggle);

    if (canToggle) {
        panel->Bind(wxEVT_LEFT_DOWN, &StartupPanel::OnToggleClick, this);
    }

    return panel;
}

void StartupPanel::DrawToggle(wxPanel* panel, bool isOn, bool canToggle) {
    if (!panel) return;

    wxClientDC dc(panel);
    dc.SetBackground(*wxWHITE);
    dc.Clear();

    const int w = panel->GetSize().GetWidth();
    const int h = panel->GetSize().GetHeight();
    const int radius = h / 2;

    // 轨道背景
    wxColour trackColor;
    if (!canToggle) {
        trackColor = wxColour(0xCC, 0xCC, 0xCC); // 灰色 - 不可操作
    } else if (isOn) {
        trackColor = wxColour(0x00, 0x78, 0xD4); // 蓝色 - 开启
    } else {
        trackColor = wxColour(0xCC, 0xCC, 0xCC); // 灰色 - 关闭
    }

    dc.SetBrush(wxBrush(trackColor));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRoundedRectangle(0, 0, w, h, radius);

    // 滑块
    int knobX = isOn ? (w - h) : 0;
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawCircle(knobX + radius, radius, radius - 2);
}

void StartupPanel::OnToggleClick(wxMouseEvent& event) {
    wxPanel* clickedPanel = static_cast<wxPanel*>(event.GetEventObject());

    // 查找对应的ToggleItem
    auto toggleItem = [&clickedPanel](std::vector<ToggleItem>& items,
                                       std::vector<IceClean::Models::StartupItem>& data) -> bool {
        for (auto& item : items) {
            if (item.togglePanel == clickedPanel) {
                if (item.isSystemCritical) return false;
                item.isOn = !item.isOn;
                data[item.itemIndex].isEnabled = item.isOn;
                DrawToggle(item.togglePanel, item.isOn, true);
                return true;
            }
        }
        return false;
    };

    if (toggleItem(m_startupToggles, m_startupItems)) {
        UpdateBootTimeEstimate();
        return;
    }
    if (toggleItem(m_serviceToggles, m_services)) {
        UpdateBootTimeEstimate();
    }
}

void StartupPanel::SetStartupItems(const std::vector<IceClean::Models::StartupItem>& items) {
    m_startupItems = items;
    BuildStartupList();
    UpdateBootTimeEstimate();
}

void StartupPanel::SetServices(const std::vector<IceClean::Models::StartupItem>& services) {
    m_services = services;
    BuildServiceList();
    UpdateBootTimeEstimate();
}

void StartupPanel::BuildStartupList() {
    m_startupSizer->Clear(true);
    m_startupToggles.clear();

    for (size_t i = 0; i < m_startupItems.size(); ++i) {
        const auto& item = m_startupItems[i];
        ToggleItem ti;
        ti.itemIndex = static_cast<int>(i);
        ti.isOn = item.isEnabled;
        ti.isSystemCritical = item.isSystemCritical;

        ti.rowPanel = new wxPanel(m_startupScroller, wxID_ANY);
        ti.rowPanel->SetBackgroundColour(*wxWHITE);
        auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->AddSpacer(8);

        // 名称
        ti.nameLabel = new wxStaticText(ti.rowPanel, wxID_ANY, item.name);
        ti.nameLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
        rowSizer->Add(ti.nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        // 发布者
        ti.publisherLabel = new wxStaticText(ti.rowPanel, wxID_ANY, item.publisher);
        ti.publisherLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
        ti.publisherLabel->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
        rowSizer->Add(ti.publisherLabel, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        // 系统关键标识
        if (item.isSystemCritical) {
            auto* criticalLabel = new wxStaticText(ti.rowPanel, wxID_ANY, L"[系统关键]");
            criticalLabel->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
            criticalLabel->SetForegroundColour(wxColour(0xE8, 0x11, 0x23));
            rowSizer->Add(criticalLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        }

        // Toggle开关
        ti.togglePanel = CreateToggleSwitch(ti.rowPanel, item.isEnabled, item.canDisable);
        rowSizer->Add(ti.togglePanel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        ti.rowPanel->SetSizer(rowSizer);
        m_startupSizer->Add(ti.rowPanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 2);

        m_startupToggles.push_back(std::move(ti));
    }

    m_startupScroller->FitInside();
}

void StartupPanel::BuildServiceList() {
    m_serviceSizer->Clear(true);
    m_serviceToggles.clear();

    for (size_t i = 0; i < m_services.size(); ++i) {
        const auto& item = m_services[i];
        ToggleItem ti;
        ti.itemIndex = static_cast<int>(i);
        ti.isOn = item.isEnabled;
        ti.isSystemCritical = item.isSystemCritical;

        ti.rowPanel = new wxPanel(m_serviceScroller, wxID_ANY);
        ti.rowPanel->SetBackgroundColour(*wxWHITE);
        auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->AddSpacer(8);

        // 名称
        ti.nameLabel = new wxStaticText(ti.rowPanel, wxID_ANY, item.name);
        ti.nameLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
        rowSizer->Add(ti.nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        // 发布者
        ti.publisherLabel = new wxStaticText(ti.rowPanel, wxID_ANY, item.publisher);
        ti.publisherLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
        ti.publisherLabel->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
        rowSizer->Add(ti.publisherLabel, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        // 系统关键标识
        if (item.isSystemCritical) {
            auto* criticalLabel = new wxStaticText(ti.rowPanel, wxID_ANY, L"[系统关键]");
            criticalLabel->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
            criticalLabel->SetForegroundColour(wxColour(0xE8, 0x11, 0x23));
            rowSizer->Add(criticalLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        }

        // Toggle开关
        ti.togglePanel = CreateToggleSwitch(ti.rowPanel, item.isEnabled, item.canDisable);
        rowSizer->Add(ti.togglePanel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        ti.rowPanel->SetSizer(rowSizer);
        m_serviceSizer->Add(ti.rowPanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 2);

        m_serviceToggles.push_back(std::move(ti));
    }

    m_serviceScroller->FitInside();
}

void StartupPanel::UpdateBootTimeEstimate() {
    // 简单估算: 每个启用的启动项约增加1-3秒
    int enabledStartup = 0;
    int enabledServices = 0;

    for (const auto& item : m_startupItems) {
        if (item.isEnabled) enabledStartup++;
    }
    for (const auto& item : m_services) {
        if (item.isEnabled) enabledServices++;
    }

    int estimatedSeconds = 5 + enabledStartup * 2 + enabledServices / 3;
    m_bootTimeLabel->SetLabel(wxString::Format(L"预估启动时间: 约%d秒 (优化可减少%d秒)",
        estimatedSeconds, enabledStartup * 2));
}

std::vector<IceClean::Models::StartupItem> StartupPanel::GetModifiedStartupItems() const {
    std::vector<IceClean::Models::StartupItem> modified;
    for (size_t i = 0; i < m_startupItems.size() && i < m_startupToggles.size(); ++i) {
        if (m_startupItems[i].isEnabled != m_startupToggles[i].isOn) {
            auto item = m_startupItems[i];
            item.isEnabled = m_startupToggles[i].isOn;
            modified.push_back(item);
        }
    }
    return modified;
}

std::vector<IceClean::Models::StartupItem> StartupPanel::GetModifiedServices() const {
    std::vector<IceClean::Models::StartupItem> modified;
    for (size_t i = 0; i < m_services.size() && i < m_serviceToggles.size(); ++i) {
        if (m_services[i].isEnabled != m_serviceToggles[i].isOn) {
            auto item = m_services[i];
            item.isEnabled = m_serviceToggles[i].isOn;
            modified.push_back(item);
        }
    }
    return modified;
}

void StartupPanel::OnOptimizeButton(wxCommandEvent& event) {
    auto modifiedStartup = GetModifiedStartupItems();
    auto modifiedServices = GetModifiedServices();

    if (modifiedStartup.empty() && modifiedServices.empty()) {
        wxMessageBox(L"没有需要优化的项目", L"提示", wxOK | wxICON_INFORMATION);
        return;
    }

    // 确认对话框
    wxString desc = L"即将执行以下优化操作:\n\n";
    if (!modifiedStartup.empty()) {
        desc += wxString::Format(L"• 禁用 %d 个启动项\n", static_cast<int>(modifiedStartup.size()));
    }
    if (!modifiedServices.empty()) {
        desc += wxString::Format(L"• 禁用 %d 个服务\n", static_cast<int>(modifiedServices.size()));
    }
    desc += L"\n系统关键项不会被修改。确定继续？";

    ConfirmDialog dlg(this, L"确认优化", desc,
                      ConfirmDialog::DangerLevel::Caution, L"确认优化", L"取消");

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    // 发送优化事件
    wxThreadEvent cleanEvt(wxEVT_CLEAN_PROGRESS);
    cleanEvt.SetInt(2); // 2 = 启动优化
    wxPostEvent(GetParent(), cleanEvt);
}

} // namespace IceClean::Gui
