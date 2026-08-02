#include "StartupPanel.h"
#include <map>
#include <algorithm>
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/Events.h"
#include "gui/controls/ThemeManager.h"
#include "utils/Win32Util.h"
#include "utils/FormatUtil.h"
#include "core/optimizer/ScheduledTaskOptimizer.h"

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(StartupPanel, wxPanel)
wxEND_EVENT_TABLE()

StartupPanel::StartupPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();
}

void StartupPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"启动加速");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    // 预估启动时间
    auto* bootSizer = new wxBoxSizer(wxHORIZONTAL);
    m_bootTimeLabel = new wxStaticText(this, wxID_ANY, L"预估启动时间: 计算中...");
    m_bootTimeLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
    m_bootTimeLabel->SetForegroundColour(colors.accent);
    bootSizer->Add(m_bootTimeLabel, 0, wxALIGN_CENTER_VERTICAL);
    mainSizer->Add(bootSizer, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // ── 标签页 ──
    m_notebook = new wxNotebook(this, wxID_ANY);
    m_notebook->SetBackgroundColour(colors.surface);

    // Tab 1: 启动项
    auto* startupPage = new wxPanel(m_notebook, wxID_ANY);
    startupPage->SetBackgroundColour(colors.surface);
    auto* startupPageSizer = new wxBoxSizer(wxVERTICAL);
    m_startupScroller = new wxScrolledWindow(startupPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                              wxVSCROLL | wxBORDER_NONE);
    m_startupScroller->SetBackgroundColour(colors.surface);
    m_startupScroller->SetScrollRate(0, 10);
    m_startupSizer = new wxBoxSizer(wxVERTICAL);
    m_startupScroller->SetSizer(m_startupSizer);
    startupPageSizer->Add(m_startupScroller, 1, wxEXPAND);
    startupPage->SetSizer(startupPageSizer);
    m_notebook->AddPage(startupPage, L"启动项");

    // Tab 2: 服务
    auto* servicePage = new wxPanel(m_notebook, wxID_ANY);
    servicePage->SetBackgroundColour(colors.surface);
    auto* servicePageSizer = new wxBoxSizer(wxVERTICAL);
    m_serviceScroller = new wxScrolledWindow(servicePage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                              wxVSCROLL | wxBORDER_NONE);
    m_serviceScroller->SetBackgroundColour(colors.surface);
    m_serviceScroller->SetScrollRate(0, 10);
    m_serviceSizer = new wxBoxSizer(wxVERTICAL);
    m_serviceScroller->SetSizer(m_serviceSizer);
    servicePageSizer->Add(m_serviceScroller, 1, wxEXPAND);
    servicePage->SetSizer(servicePageSizer);
    m_notebook->AddPage(servicePage, L"服务");

    // Tab 3: 计划任务
    auto* taskPage = new wxPanel(m_notebook, wxID_ANY);
    taskPage->SetBackgroundColour(colors.surface);
    auto* taskPageSizer = new wxBoxSizer(wxVERTICAL);

    auto* taskDesc = new wxStaticText(taskPage, wxID_ANY,
        L"开机启动的计划任务，禁用可加速开机");
    taskDesc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    taskDesc->SetForegroundColour(colors.textDisabled);
    taskPageSizer->Add(taskDesc, 0, wxALL, 4);

    m_taskScroller = new wxScrolledWindow(taskPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          wxVSCROLL | wxBORDER_NONE);
    m_taskScroller->SetBackgroundColour(colors.surface);
    m_taskScroller->SetScrollRate(0, 10);
    m_taskSizer = new wxBoxSizer(wxVERTICAL);
    m_taskScroller->SetSizer(m_taskSizer);
    taskPageSizer->Add(m_taskScroller, 1, wxEXPAND);
    taskPage->SetSizer(taskPageSizer);
    m_notebook->AddPage(taskPage, L"计划任务");

    // Tab 4: 进程管理
    auto* processPage = new wxPanel(m_notebook, wxID_ANY);
    processPage->SetBackgroundColour(colors.surface);
    auto* processPageSizer = new wxBoxSizer(wxVERTICAL);

    // 刷新按钮
    auto* processBtnSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* processDesc = new wxStaticText(processPage, wxID_ANY,
        L"以下为非系统自带进程，可安全终止的守护进程已标绿");
    processDesc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    processDesc->SetForegroundColour(colors.textDisabled);
    processBtnSizer->Add(processDesc, 1, wxALIGN_CENTER_VERTICAL);

    m_refreshProcessButton = new wxButton(processPage, wxID_ANY, L"刷新",
        wxDefaultPosition, wxSize(60, 28));
    m_refreshProcessButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_refreshProcessButton->Bind(wxEVT_BUTTON, &StartupPanel::OnRefreshProcess, this);
    processBtnSizer->Add(m_refreshProcessButton, 0);
    processPageSizer->Add(processBtnSizer, 0, wxEXPAND | wxALL, 4);

    m_processList = new wxListCtrl(processPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxLC_REPORT | wxBORDER_SIMPLE);
    m_processList->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_processList->AppendColumn(L"进程名", wxLIST_FORMAT_LEFT, 150);
    m_processList->AppendColumn(L"类型", wxLIST_FORMAT_CENTER, 80);
    m_processList->AppendColumn(L"公司", wxLIST_FORMAT_LEFT, 120);
    m_processList->AppendColumn(L"内存", wxLIST_FORMAT_RIGHT, 100);
    m_processList->AppendColumn(L"安全等级", wxLIST_FORMAT_CENTER, 80);
    m_processList->AppendColumn(L"操作", wxLIST_FORMAT_CENTER, 60);
    m_processList->Bind(wxEVT_LIST_COL_CLICK, &StartupPanel::OnProcessColumnClick, this);
    m_processList->Bind(wxEVT_LIST_ITEM_ACTIVATED, &StartupPanel::OnKillProcess, this);

    processPageSizer->Add(m_processList, 1, wxEXPAND | wxALL, 4);
    processPage->SetSizer(processPageSizer);
    m_notebook->AddPage(processPage, L"进程管理");

    mainSizer->Add(m_notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // ── 底部优化按钮 ──
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    bottomSizer->AddStretchSpacer();

    m_optimizeButton = new wxButton(this, wxID_ANY, L"优化", wxDefaultPosition, wxSize(140, 40));
    m_optimizeButton->SetName("btn_primary_optimize");
    m_optimizeButton->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                     false, L"微软雅黑"));
    m_optimizeButton->SetBackgroundColour(colors.accent);
    m_optimizeButton->SetForegroundColour(*wxWHITE);
    m_optimizeButton->Bind(wxEVT_BUTTON, &StartupPanel::OnOptimizeButton, this);
    bottomSizer->Add(m_optimizeButton, 0, wxRIGHT, 20);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxBOTTOM, 12);

    // 绑定标签页切换事件
    m_notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &StartupPanel::OnNotebookPageChanged, this);

    SetSizer(mainSizer);

    // 计划任务延迟加载（在 OnNotebookPageChanged 中首次切换到时加载）
}

wxPanel* StartupPanel::CreateToggleSwitch(wxWindow* parent, bool isOn, bool canToggle) {
    auto* panel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(44, 22));
    panel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    panel->SetCursor(canToggle ? wxCURSOR_HAND : wxCURSOR_ARROW);

    // 绑定绘制事件 - 从ToggleItem向量中查找状态
    panel->Bind(wxEVT_PAINT, [this, panel, canToggle](wxPaintEvent&) {
        // 在所有ToggleItem向量中查找此面板的状态
        bool state = false;
        bool found = false;
        for (const auto& item : m_startupToggles) {
            if (item.togglePanel == panel) { state = item.isOn; found = true; break; }
        }
        if (!found) {
            for (const auto& item : m_serviceToggles) {
                if (item.togglePanel == panel) { state = item.isOn; found = true; break; }
            }
        }
        if (!found) {
            for (const auto& item : m_taskToggles) {
                if (item.togglePanel == panel) { state = item.isOn; found = true; break; }
            }
        }
        DrawToggle(panel, found ? state : false, canToggle);
    });

    if (canToggle) {
        panel->Bind(wxEVT_LEFT_DOWN, &StartupPanel::OnToggleClick, this);
    }

    // 初始绘制
    DrawToggle(panel, isOn, canToggle);

    return panel;
}

void StartupPanel::DrawToggle(wxPanel* panel, bool isOn, bool canToggle) {
    if (!panel) return;

    const auto& colors = ThemeManager::Instance().GetColors();
    const int w = panel->GetSize().GetWidth();
    const int h = panel->GetSize().GetHeight();
    const int radius = h / 2;

    // 轨道背景颜色
    wxColour trackColor;
    if (!canToggle) {
        trackColor = colors.border;
    } else if (isOn) {
        trackColor = colors.accent;
    } else {
        trackColor = colors.border;
    }

    // 使用wxClientDC进行绘制（Refresh时触发）
    wxClientDC dc(panel);
    dc.SetBackground(colors.background);
    dc.Clear();

    dc.SetBrush(wxBrush(trackColor));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRoundedRectangle(0, 0, w, h, radius);

    // 滑块
    int knobX = isOn ? (w - h) : 0;
    dc.SetBrush(wxBrush(colors.surface));
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
                // 刷新Toggle显示
                item.togglePanel->Refresh();
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
        return;
    }
    if (toggleItem(m_taskToggles, m_scheduledTasks)) {
        UpdateBootTimeEstimate();
    }
}

void StartupPanel::SetStartupItems(const std::vector<IceClean::Models::StartupItem>& items) {
    m_startupItems = items;
    m_startupOriginalState.clear();
    for (const auto& item : items) {
        m_startupOriginalState.push_back(item.isEnabled);
    }
    BuildStartupList();
    UpdateBootTimeEstimate();
}

void StartupPanel::SetServices(const std::vector<IceClean::Models::StartupItem>& services) {
    m_services = services;
    m_serviceOriginalState.clear();
    for (const auto& item : services) {
        m_serviceOriginalState.push_back(item.isEnabled);
    }
    BuildServiceList();
    UpdateBootTimeEstimate();
}

void StartupPanel::BuildStartupList() {
    const auto& colors = ThemeManager::Instance().GetColors();
    m_startupSizer->Clear(true);
    m_startupToggles.clear();

    for (size_t i = 0; i < m_startupItems.size(); ++i) {
        const auto& item = m_startupItems[i];
        ToggleItem ti;
        ti.itemIndex = static_cast<int>(i);
        ti.isOn = item.isEnabled;
        ti.isSystemCritical = item.isSystemCritical;

        // 检查关联进程是否正在运行
        std::wstring processName = Utils::Win32Util::ExtractProcessName(item.path);
        ti.isProcessRunning = !processName.empty() && Utils::Win32Util::IsProcessRunning(processName);

        ti.rowPanel = new wxPanel(m_startupScroller, wxID_ANY);
        ti.rowPanel->SetBackgroundColour(colors.surface);
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
        ti.publisherLabel->SetForegroundColour(colors.textDisabled);
        rowSizer->Add(ti.publisherLabel, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        // 系统关键标识
        if (item.isSystemCritical) {
            auto* criticalLabel = new wxStaticText(ti.rowPanel, wxID_ANY, L"[系统关键]");
            criticalLabel->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
            criticalLabel->SetForegroundColour(colors.danger);
            rowSizer->Add(criticalLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        }

        // 运行状态标签
        ti.statusLabel = new wxStaticText(ti.rowPanel, wxID_ANY,
            ti.isProcessRunning ? L"运行中" : L"");
        ti.statusLabel->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                       false, L"微软雅黑"));
        if (ti.isProcessRunning) {
            ti.statusLabel->SetForegroundColour(colors.success);
        }
        rowSizer->Add(ti.statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

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
    const auto& colors = ThemeManager::Instance().GetColors();
    m_serviceSizer->Clear(true);
    m_serviceToggles.clear();

    for (size_t i = 0; i < m_services.size(); ++i) {
        const auto& item = m_services[i];
        ToggleItem ti;
        ti.itemIndex = static_cast<int>(i);
        ti.isOn = item.isEnabled;
        ti.isSystemCritical = item.isSystemCritical;

        // 检查关联进程是否正在运行
        std::wstring processName = Utils::Win32Util::ExtractProcessName(item.path);
        ti.isProcessRunning = !processName.empty() && Utils::Win32Util::IsProcessRunning(processName);

        ti.rowPanel = new wxPanel(m_serviceScroller, wxID_ANY);
        ti.rowPanel->SetBackgroundColour(colors.surface);
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
        ti.publisherLabel->SetForegroundColour(colors.textDisabled);
        rowSizer->Add(ti.publisherLabel, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        // 系统关键标识
        if (item.isSystemCritical) {
            auto* criticalLabel = new wxStaticText(ti.rowPanel, wxID_ANY, L"[系统关键]");
            criticalLabel->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
            criticalLabel->SetForegroundColour(colors.danger);
            rowSizer->Add(criticalLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        }

        // 运行状态标签
        ti.statusLabel = new wxStaticText(ti.rowPanel, wxID_ANY,
            ti.isProcessRunning ? L"运行中" : L"");
        ti.statusLabel->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                       false, L"微软雅黑"));
        if (ti.isProcessRunning) {
            ti.statusLabel->SetForegroundColour(colors.success);
        }
        rowSizer->Add(ti.statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

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
    int enabledTasks = 0;

    for (const auto& item : m_startupItems) {
        if (item.isEnabled) enabledStartup++;
    }
    for (const auto& item : m_services) {
        if (item.isEnabled) enabledServices++;
    }
    for (const auto& item : m_scheduledTasks) {
        if (item.isEnabled) enabledTasks++;
    }

    int estimatedSeconds = 5 + enabledStartup * 2 + enabledServices / 3 + enabledTasks;
    m_bootTimeLabel->SetLabel(wxString::Format(L"预估启动时间: 约%d秒 (优化可减少%d秒)",
        estimatedSeconds, enabledStartup * 2 + enabledTasks));
}

std::vector<IceClean::Models::StartupItem> StartupPanel::GetModifiedStartupItems() const {
    std::vector<IceClean::Models::StartupItem> modified;
    for (size_t i = 0; i < m_startupItems.size() && i < m_startupToggles.size(); ++i) {
        bool originalState = (i < m_startupOriginalState.size()) ? m_startupOriginalState[i] : m_startupItems[i].isEnabled;
        if (m_startupToggles[i].isOn != originalState) {
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
        bool originalState = (i < m_serviceOriginalState.size()) ? m_serviceOriginalState[i] : m_services[i].isEnabled;
        if (m_serviceToggles[i].isOn != originalState) {
            auto item = m_services[i];
            item.isEnabled = m_serviceToggles[i].isOn;
            modified.push_back(item);
        }
    }
    return modified;
}

std::vector<IceClean::Models::StartupItem> StartupPanel::GetModifiedScheduledTasks() const {
    std::vector<IceClean::Models::StartupItem> modified;
    for (size_t i = 0; i < m_scheduledTasks.size() && i < m_taskToggles.size(); ++i) {
        bool originalState = (i < m_scheduledTaskOriginalState.size()) ? m_scheduledTaskOriginalState[i] : m_scheduledTasks[i].isEnabled;
        if (m_taskToggles[i].isOn != originalState) {
            auto item = m_scheduledTasks[i];
            item.isEnabled = m_taskToggles[i].isOn;
            modified.push_back(item);
        }
    }
    return modified;
}

void StartupPanel::OnOptimizeButton(wxCommandEvent& event) {
    auto modifiedStartup = GetModifiedStartupItems();
    auto modifiedServices = GetModifiedServices();
    auto modifiedTasks = GetModifiedScheduledTasks();

    if (modifiedStartup.empty() && modifiedServices.empty() && modifiedTasks.empty()) {
        wxMessageBox(L"没有需要优化的项目", L"提示", wxOK | wxICON_INFORMATION);
        return;
    }

    // 统计需要终止进程的项目
    int runningCount = 0;
    for (const auto& ti : m_startupToggles) {
        if (!ti.isOn && ti.isProcessRunning) runningCount++;
    }
    for (const auto& ti : m_serviceToggles) {
        if (!ti.isOn && ti.isProcessRunning) runningCount++;
    }
    for (const auto& ti : m_taskToggles) {
        if (!ti.isOn && ti.isProcessRunning) runningCount++;
    }

    // 确认对话框
    wxString desc = L"即将执行以下优化操作:\n\n";
    if (!modifiedStartup.empty()) {
        desc += wxString::Format(L"• 禁用 %d 个启动项\n", static_cast<int>(modifiedStartup.size()));
    }
    if (!modifiedServices.empty()) {
        desc += wxString::Format(L"• 禁用 %d 个服务\n", static_cast<int>(modifiedServices.size()));
    }
    if (!modifiedTasks.empty()) {
        desc += wxString::Format(L"• 禁用 %d 个计划任务\n", static_cast<int>(modifiedTasks.size()));
    }
    if (runningCount > 0) {
        desc += wxString::Format(L"\n⚠ 将终止 %d 个运行中的进程", runningCount);
    }
    desc += L"\n\n系统关键项不会被修改。确定继续？";

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

void StartupPanel::LoadScheduledTasks() {
    IceClean::Core::Optimizer::ScheduledTaskOptimizer optimizer;
    m_scheduledTasks = optimizer.GetDisablableTasks();
    m_scheduledTaskOriginalState.clear();
    for (const auto& item : m_scheduledTasks) {
        m_scheduledTaskOriginalState.push_back(item.isEnabled);
    }
    BuildScheduledTaskList();
    UpdateBootTimeEstimate();
}

void StartupPanel::BuildScheduledTaskList() {
    const auto& colors = ThemeManager::Instance().GetColors();
    m_taskSizer->Clear(true);
    m_taskToggles.clear();

    for (size_t i = 0; i < m_scheduledTasks.size(); ++i) {
        const auto& item = m_scheduledTasks[i];
        ToggleItem ti;
        ti.itemIndex = static_cast<int>(i);
        ti.isOn = item.isEnabled;
        ti.isSystemCritical = item.isSystemCritical;

        // 检查关联进程是否正在运行
        std::wstring processName = Utils::Win32Util::ExtractProcessName(item.path);
        ti.isProcessRunning = !processName.empty() && Utils::Win32Util::IsProcessRunning(processName);

        ti.rowPanel = new wxPanel(m_taskScroller, wxID_ANY);
        ti.rowPanel->SetBackgroundColour(colors.surface);
        auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->AddSpacer(8);

        // 名称
        ti.nameLabel = new wxStaticText(ti.rowPanel, wxID_ANY, item.name);
        ti.nameLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
        rowSizer->Add(ti.nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        // 路径（截断显示）
        wxString displayPath = item.path;
        if (displayPath.length() > 50) {
            displayPath = displayPath.Mid(0, 47) + L"...";
        }
        ti.publisherLabel = new wxStaticText(ti.rowPanel, wxID_ANY, displayPath);
        ti.publisherLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
        ti.publisherLabel->SetForegroundColour(colors.textDisabled);
        rowSizer->Add(ti.publisherLabel, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        // 系统关键标识
        if (item.isSystemCritical) {
            auto* criticalLabel = new wxStaticText(ti.rowPanel, wxID_ANY, L"[系统关键]");
            criticalLabel->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
            criticalLabel->SetForegroundColour(colors.danger);
            rowSizer->Add(criticalLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        }

        // 运行状态标签
        ti.statusLabel = new wxStaticText(ti.rowPanel, wxID_ANY,
            ti.isProcessRunning ? L"运行中" : L"");
        ti.statusLabel->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                       false, L"微软雅黑"));
        if (ti.isProcessRunning) {
            ti.statusLabel->SetForegroundColour(colors.success);
        }
        rowSizer->Add(ti.statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        // Toggle开关
        ti.togglePanel = CreateToggleSwitch(ti.rowPanel, item.isEnabled, item.canDisable);
        rowSizer->Add(ti.togglePanel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        ti.rowPanel->SetSizer(rowSizer);
        m_taskSizer->Add(ti.rowPanel, 0, wxEXPAND | wxTOP | wxBOTTOM, 2);

        m_taskToggles.push_back(std::move(ti));
    }

    m_taskScroller->FitInside();
}

void StartupPanel::OnNotebookPageChanged(wxBookCtrlEvent& event) {
    int page = event.GetSelection();
    // Page 2: 计划任务 (0=启动项, 1=服务, 2=计划任务, 3=进程管理)
    if (page == 2 && m_scheduledTasks.empty()) {
        LoadScheduledTasks();
    }
    // Page 3: 进程管理
    if (page == 3 && m_processes.empty()) {
        LoadProcessList();
    }
    event.Skip();
}

void StartupPanel::LoadProcessList() {
    m_processes = IceClean::Core::Analyzer::ProcessAnalyzer::GetRunningProcesses();
    BuildProcessList();
}

void StartupPanel::BuildProcessList() {
    const auto& colors = ThemeManager::Instance().GetColors();
    m_processList->DeleteAllItems();
    m_processGroups.clear();

    // 1. 按进程名分组
    std::map<std::wstring, std::vector<size_t>> nameToIndices;
    for (size_t i = 0; i < m_processes.size(); ++i) {
        nameToIndices[m_processes[i].name].push_back(i);
    }

    // 2. 构建分组数据
    for (auto& [name, indices] : nameToIndices) {
        ProcessGroup group;
        group.name = name;
        group.companyName = m_processes[indices[0]].companyName;
        group.safety = m_processes[indices[0]].safety;
        group.isSystemProcess = m_processes[indices[0]].isSystemProcess;
        group.instanceCount = static_cast<int>(indices.size());
        group.totalMemory = 0;
        for (size_t idx : indices) {
            group.totalMemory += m_processes[idx].memoryUsage;
        }
        group.processIndices = std::move(indices);
        m_processGroups.push_back(std::move(group));
    }

    // 3. 排序
    SortProcessGroups();

    // 4. 填充列表
    for (size_t i = 0; i < m_processGroups.size(); ++i) {
        const auto& group = m_processGroups[i];

        // 进程名
        long idx = m_processList->InsertItem(static_cast<long>(i), group.name);

        // 类型（系统进程/应用进程）
        wxString typeStr = group.isSystemProcess ? L"系统进程" : L"应用进程";
        m_processList->SetItem(idx, 1, typeStr);

        // 公司名
        wxString company = group.companyName.empty() ? L"未知" : group.companyName;
        m_processList->SetItem(idx, 2, company);

        // 内存占用
        double memMB = static_cast<double>(group.totalMemory) / (1024.0 * 1024.0);
        wxString memStr;
        if (group.instanceCount > 1) {
            memStr = wxString::Format(L"%.1f MB (%d)", memMB, group.instanceCount);
        } else {
            memStr = wxString::Format(L"%.1f MB", memMB);
        }
        m_processList->SetItem(idx, 3, memStr);

        // 安全等级
        wxString safetyText;
        if (group.safety == IceClean::Core::Analyzer::ProcessSafety::Safe) {
            safetyText = L"可终止";
        } else if (group.safety == IceClean::Core::Analyzer::ProcessSafety::Caution) {
            safetyText = L"谨慎";
        } else {
            safetyText = L"系统关键";
        }
        m_processList->SetItem(idx, 4, safetyText);

        // 操作
        if (group.safety != IceClean::Core::Analyzer::ProcessSafety::Critical) {
            m_processList->SetItem(idx, 5, L"终止");
        }
    }

    // 设置行颜色
    for (long i = 0; i < m_processList->GetItemCount(); ++i) {
        const auto& group = m_processGroups[i];
        if (group.safety == IceClean::Core::Analyzer::ProcessSafety::Critical) {
            m_processList->SetItemTextColour(i, colors.danger);
        } else if (group.safety == IceClean::Core::Analyzer::ProcessSafety::Caution) {
            m_processList->SetItemTextColour(i, colors.warning);
        }
    }
}

void StartupPanel::SortProcessGroups() {
    // 默认排序：安全等级升序 + 内存降序
    if (m_processSortColumn < 0) {
        std::sort(m_processGroups.begin(), m_processGroups.end(),
            [](const ProcessGroup& a, const ProcessGroup& b) {
                if (a.safety != b.safety) return a.safety < b.safety;
                return a.totalMemory > b.totalMemory;
            });
        return;
    }

    auto cmp = [this](const ProcessGroup& a, const ProcessGroup& b) {
        bool result = false;
        switch (m_processSortColumn) {
            case 0: // 进程名
                result = a.name < b.name;
                break;
            case 1: // 类型（系统/应用）
                result = a.isSystemProcess < b.isSystemProcess;
                break;
            case 2: // 公司
                result = a.companyName < b.companyName;
                break;
            case 3: // 内存
                result = a.totalMemory > b.totalMemory;
                break;
            case 4: // 安全等级
                result = a.safety < b.safety;
                break;
            default:
                result = a.name < b.name;
                break;
        }
        return m_processSortAsc ? result : !result;
    };
    std::sort(m_processGroups.begin(), m_processGroups.end(), cmp);
}

void StartupPanel::OnProcessColumnClick(wxListEvent& event) {
    const auto& colors = ThemeManager::Instance().GetColors();
    int col = event.GetColumn();
    if (m_processSortColumn == col) {
        m_processSortAsc = !m_processSortAsc;
    } else {
        m_processSortColumn = col;
        m_processSortAsc = true;
    }
    SortProcessGroups();

    // 重建列表
    m_processList->DeleteAllItems();
    for (size_t i = 0; i < m_processGroups.size(); ++i) {
        const auto& group = m_processGroups[i];

        long idx = m_processList->InsertItem(static_cast<long>(i), group.name);

        wxString typeStr = group.isSystemProcess ? L"系统进程" : L"应用进程";
        m_processList->SetItem(idx, 1, typeStr);

        wxString company = group.companyName.empty() ? L"未知" : group.companyName;
        m_processList->SetItem(idx, 2, company);

        double memMB = static_cast<double>(group.totalMemory) / (1024.0 * 1024.0);
        wxString memStr;
        if (group.instanceCount > 1) {
            memStr = wxString::Format(L"%.1f MB (%d)", memMB, group.instanceCount);
        } else {
            memStr = wxString::Format(L"%.1f MB", memMB);
        }
        m_processList->SetItem(idx, 3, memStr);

        wxString safetyText;
        if (group.safety == IceClean::Core::Analyzer::ProcessSafety::Safe) {
            safetyText = L"可终止";
        } else if (group.safety == IceClean::Core::Analyzer::ProcessSafety::Caution) {
            safetyText = L"谨慎";
        } else {
            safetyText = L"系统关键";
        }
        m_processList->SetItem(idx, 4, safetyText);

        if (group.safety != IceClean::Core::Analyzer::ProcessSafety::Critical) {
            m_processList->SetItem(idx, 5, L"终止");
        }
    }

    // 设置行颜色
    for (long i = 0; i < m_processList->GetItemCount(); ++i) {
        const auto& group = m_processGroups[i];
        if (group.safety == IceClean::Core::Analyzer::ProcessSafety::Critical) {
            m_processList->SetItemTextColour(i, colors.danger);
        } else if (group.safety == IceClean::Core::Analyzer::ProcessSafety::Caution) {
            m_processList->SetItemTextColour(i, colors.warning);
        }
    }
}

void StartupPanel::OnRefreshProcess(wxCommandEvent& /*event*/) {
    LoadProcessList();
}

void StartupPanel::OnKillProcess(wxListEvent& event) {
    int groupIndex = event.GetIndex();
    if (groupIndex < 0 || groupIndex >= static_cast<int>(m_processGroups.size())) return;

    const auto& group = m_processGroups[groupIndex];

    // 系统关键进程不可终止
    if (group.safety == IceClean::Core::Analyzer::ProcessSafety::Critical) {
        wxMessageBox(L"系统关键进程不可终止。", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    // 确认对话框
    wxString desc;
    if (group.instanceCount > 1) {
        desc = wxString::Format(L"确定要终止所有 %d 个 %s 进程吗？\n\n",
            group.instanceCount, group.name.c_str());
    } else {
        desc = wxString::Format(L"确定要终止进程 %s 吗？\n\n", group.name.c_str());
    }
    if (group.safety == IceClean::Core::Analyzer::ProcessSafety::Caution) {
        desc += L"该进程标记为\"谨慎\"，终止后可能影响相关软件功能。\n";
    }
    desc += L"如果普通方式无法终止，将自动升级为强制终止。";

    ConfirmDialog dlg(this, L"终止进程", desc,
                      ConfirmDialog::DangerLevel::Caution, L"终止", L"取消");

    if (dlg.ShowModal() != wxID_OK) return;

    // 尝试终止该组所有进程
    bool killed = IceClean::Utils::Win32Util::KillProcessByName(group.name);

    if (killed) {
        if (group.instanceCount > 1) {
            wxMessageBox(wxString::Format(L"所有 %s 进程已成功终止。", group.name.c_str()),
                         L"成功", wxOK | wxICON_INFORMATION);
        } else {
            wxMessageBox(wxString::Format(L"进程 %s 已成功终止。", group.name.c_str()),
                         L"成功", wxOK | wxICON_INFORMATION);
        }
    } else {
        wxMessageBox(wxString::Format(L"无法终止进程 %s，可能需要管理员权限。", group.name.c_str()),
                     L"终止失败", wxOK | wxICON_WARNING);
    }

    // 刷新列表
    LoadProcessList();
}

} // namespace IceClean::Gui
