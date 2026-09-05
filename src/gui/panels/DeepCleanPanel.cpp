#include "DeepCleanPanel.h"
#include "gui/controls/ThemeManager.h"
#include "gui/controls/SafetyBadge.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/Events.h"
#include "core/cleaner/RegistryCleaner.h"
#include "core/cleaner/PrivacyCleaner.h"
#include "core/cleaner/DismCleaner.h"
#include "core/cleaner/HibernationCleaner.h"
#include "core/cleaner/FileCleaner.h"
#include "core/safety/RestorePointManager.h"
#include "core/safety/OperationLogger.h"
#include "utils/FileUtil.h"
#include "utils/FormatUtil.h"
#include "utils/JsonUtil.h"
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <shlobj.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(DeepCleanPanel, wxPanel)
wxEND_EVENT_TABLE()

DeepCleanPanel::DeepCleanPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();

    // 主题切换时刷新按钮颜色
    ThemeManager::Instance().RegisterChangeCallback([this](const ThemeColors&) {
        CallAfter([this]() {
            RefreshButtonColors();
        });
    });
}

void DeepCleanPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(8);

    // 一键概览区（360 式：大圆环 + 概览 + 主按钮）
    CreateQuickOverview(mainSizer);
    mainSizer->AddSpacer(4);

    // 高级设置（默认折叠）
    CreateAdvancedPanel(mainSizer);

    // 标签页（高级模式：分类精细操作）
    m_notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxNB_TOP | wxBORDER_NONE);
    m_notebook->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    m_notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this](wxBookCtrlEvent& event) {
        // 注册表清理/软件专清页有独立按钮，隐藏底部"开始清理"按钮
        int page = m_notebook->GetSelection();
        if (page == 1 || page == 3) {  // 注册表清理 或 软件专清
            m_cleanButton->Hide();
        } else {
            m_cleanButton->Show();
            // 更新按钮文本
            if (page == 0) {
                m_cleanButton->SetLabel(L"开始系统清理");
            } else {
                m_cleanButton->SetLabel(L"开始隐私清理");
            }
        }
        Layout();
        event.Skip();
    });

    // 系统清理标签页
    auto* systemTab = new wxPanel(m_notebook);
    systemTab->SetBackgroundColour(colors.background);
    CreateSystemCleanTab(systemTab);
    m_notebook->AddPage(systemTab, L"系统清理");

    // 注册表清理标签页
    auto* registryTab = new wxPanel(m_notebook);
    registryTab->SetBackgroundColour(colors.background);
    CreateRegistryCleanTab(registryTab);
    m_notebook->AddPage(registryTab, L"注册表清理");

    // 隐私清理标签页
    auto* privacyTab = new wxPanel(m_notebook);
    privacyTab->SetBackgroundColour(colors.background);
    CreatePrivacyCleanTab(privacyTab);
    m_notebook->AddPage(privacyTab, L"隐私清理");

    // 软件专清标签页
    auto* softwareTab = new wxPanel(m_notebook);
    softwareTab->SetBackgroundColour(colors.background);
    CreateSoftwareCacheTab(softwareTab);
    m_notebook->AddPage(softwareTab, L"软件专清");

    mainSizer->Add(m_notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // 底部按钮
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    bottomSizer->AddStretchSpacer();

    m_cleanButton = new wxButton(this, wxID_ANY, L"开始系统清理", wxDefaultPosition, wxSize(160, 40));
    m_cleanButton->SetName("btn_primary_clean");
    m_cleanButton->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
    m_cleanButton->SetBackgroundColour(colors.accent);
    m_cleanButton->SetForegroundColour(*wxWHITE);
    m_cleanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnCleanButton, this);
    ThemeManager::Instance().ApplyButtonHover(m_cleanButton, colors.accent, wxColour());
    TrackButtonColor(m_cleanButton, colors.accent, wxColour());
    bottomSizer->Add(m_cleanButton, 0, wxRIGHT, 20);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxBOTTOM, 12);

    // 子复选框勾选变化（事件冒泡到面板）→ 刷新 tab 数量角标
    Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
        UpdateTabBadges();
        event.Skip();
    });

    UpdateTabBadges();
    SetSizer(mainSizer);
}

// ── 一键概览区（360 式） ──

void DeepCleanPanel::RefreshButtonColors() {
    const auto& colors = ThemeManager::Instance().GetColors();
    for (auto& pair : m_buttonColors) {
        if (!pair.btn) continue;
        // 重新设置背景色
        pair.btn->SetBackgroundColour(pair.normalBg);
        // 重新计算悬停色并绑定（lambda 替换：wxWidgets 会自动去重/替换同类型处理器）
        const wxColour normal = pair.normalBg;
        const wxColour hover = pair.hoverBg.IsOk() ? pair.hoverBg
            : (0.299 * normal.Red() + 0.587 * normal.Green() + 0.114 * normal.Blue() > 128.0
                ? wxColour(std::max(0, normal.Red() - 12), std::max(0, normal.Green() - 12), std::max(0, normal.Blue() - 12))
                : wxColour(std::min(255, normal.Red() + 15), std::min(255, normal.Green() + 15), std::min(255, normal.Blue() + 15)));
        auto enterCb = [pair, hover](wxMouseEvent&) {
            if (!pair.btn->IsEnabled()) return;
            pair.btn->SetBackgroundColour(hover);
            pair.btn->Refresh();
        };
        auto leaveCb = [pair, normal](wxMouseEvent&) {
            if (!pair.btn->IsEnabled()) return;
            pair.btn->SetBackgroundColour(normal);
            pair.btn->Refresh();
        };
        pair.btn->Bind(wxEVT_ENTER_WINDOW, enterCb);
        pair.btn->Bind(wxEVT_LEAVE_WINDOW, leaveCb);
    }
}

void DeepCleanPanel::CreateQuickOverview(wxSizer* mainSizer) {
    const auto& colors = ThemeManager::Instance().GetColors();
    
    // 紧凑单行头部：健康分圆环 | 预计释放+进度+抽样信息 | 操作按钮
    auto* mainVerticalSizer = new wxBoxSizer(wxVERTICAL);

    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);

    // 小圆环（点击即扫描）
    m_quickRing = new IceClean::Gui::CircularProgress(this, wxID_ANY,
                                                      wxDefaultPosition, wxSize(76, 76));
    m_quickRing->SetProgressColor(colors.accent);
    m_quickRing->SetValue(0);
    m_quickRing->SetLabel(L"--");
    m_quickRing->SetSubLabel(L"健康分");
    m_quickRing->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) {
        wxCommandEvent cmd(wxEVT_BUTTON, m_quickScanButton->GetId());
        OnQuickScan(cmd);
    });
    headerSizer->Add(m_quickRing, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

    // 中部：核心数字 + 说明 + 进度条 + 扫描抽样
    auto* midSizer = new wxBoxSizer(wxVERTICAL);

    m_quickSizeLabel = new wxStaticText(this, wxID_ANY, L"预计可释放 --");
    m_quickSizeLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                     false, L"微软雅黑"));
    m_quickSizeLabel->SetForegroundColour(colors.textPrimary);
    midSizer->Add(m_quickSizeLabel, 0, wxBOTTOM, 2);

    m_quickCountLabel = new wxStaticText(this, wxID_ANY, L"点击\"一键扫描\"检测可清理项，安全项将自动勾选");
    m_quickCountLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
    m_quickCountLabel->SetForegroundColour(colors.textSecondary);
    midSizer->Add(m_quickCountLabel, 0, wxBOTTOM, 6);

    // 实时进度条（横向铺满中部）
    m_scanProgressBar = new IceClean::Gui::ScanProgressBar(this, wxID_ANY);
    m_scanProgressBar->SetValue(0);
    m_scanProgressBar->SetStatusText(L"就绪");
    m_scanProgressBar->SetMinSize(wxSize(200, 14));
    midSizer->Add(m_scanProgressBar, 0, wxEXPAND | wxRIGHT, 8);

    // 扫描抽样信息条
    m_scanInfoPanel = new IceClean::Gui::ScanInfoPanel(this, wxID_ANY,
                                                       wxDefaultPosition, wxSize(-1, 40));
    m_scanInfoPanel->SetState(ScanInfoPanelState::Normal);
    m_scanInfoPanel->SetMinSize(wxSize(220, 40));
    midSizer->Add(m_scanInfoPanel, 0, wxEXPAND | wxTOP, 6);

    headerSizer->Add(midSizer, 1, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 16);

    // 右侧：操作按钮
    auto* buttonSizer = new wxBoxSizer(wxVERTICAL);

    m_quickScanButton = new wxButton(this, wxID_ANY, L"一键扫描",
                                     wxDefaultPosition, wxSize(120, 34));
    m_quickScanButton->SetName("btn_primary_quick_scan");
    m_quickScanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                      false, L"微软雅黑"));
    m_quickScanButton->SetBackgroundColour(colors.accent);
    m_quickScanButton->SetForegroundColour(*wxWHITE);
    m_quickScanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnQuickScan, this);
    ThemeManager::Instance().ApplyButtonHover(m_quickScanButton, colors.accent, wxColour());
    TrackButtonColor(m_quickScanButton, colors.accent, wxColour());
    buttonSizer->Add(m_quickScanButton, 0, wxBOTTOM, 8);

    m_quickCleanButton = new wxButton(this, wxID_ANY, L"一键清理",
                                      wxDefaultPosition, wxSize(120, 34));
    m_quickCleanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                       false, L"微软雅黑"));
    m_quickCleanButton->SetBackgroundColour(colors.accent);
    m_quickCleanButton->SetForegroundColour(*wxWHITE);
    m_quickCleanButton->Enable(false);
    m_quickCleanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnQuickClean, this);
    ThemeManager::Instance().ApplyButtonHover(m_quickCleanButton, colors.accent, wxColour());
    TrackButtonColor(m_quickCleanButton, colors.accent, wxColour());
    buttonSizer->Add(m_quickCleanButton, 0, wxBOTTOM, 8);

    // 次要操作并排，压缩头部高度
    auto* minorRow = new wxBoxSizer(wxHORIZONTAL);
    m_pauseButton = new wxButton(this, wxID_ANY, L"暂停",
                                 wxDefaultPosition, wxSize(57, 26));
    m_pauseButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                  false, L"微软雅黑"));
    m_pauseButton->SetName("btn_pause");
    m_pauseButton->SetBackgroundColour(colors.surface);
    m_pauseButton->SetForegroundColour(colors.textPrimary);
    m_pauseButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnPauseButton, this);
    ThemeManager::Instance().ApplyButtonHover(m_pauseButton, colors.surface, wxColour());
    TrackButtonColor(m_pauseButton, colors.surface, wxColour());
    minorRow->Add(m_pauseButton, 0, wxRIGHT, 6);

    m_stopScanButton = new wxButton(this, wxID_ANY, L"停止",
                                    wxDefaultPosition, wxSize(57, 26));
    m_stopScanButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
    m_stopScanButton->SetBackgroundColour(colors.surface);
    m_stopScanButton->SetForegroundColour(colors.danger);
    m_stopScanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnStopScanButton, this);
    m_stopScanButton->Enable(false);
    ThemeManager::Instance().ApplyButtonHover(m_stopScanButton, colors.surface, wxColour());
    TrackButtonColor(m_stopScanButton, colors.surface, wxColour());
    minorRow->Add(m_stopScanButton, 0, wxRIGHT, 6);

    m_advancedToggleButton = new wxButton(this, wxID_ANY, L"高级 ▾",
                                          wxDefaultPosition, wxSize(57, 26));
    m_advancedToggleButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_advancedToggleButton->SetBackgroundColour(colors.surface);
    m_advancedToggleButton->SetForegroundColour(colors.textPrimary);
    m_advancedToggleButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnAdvancedToggle, this);
    ThemeManager::Instance().ApplyButtonHover(m_advancedToggleButton, colors.surface, wxColour());
    TrackButtonColor(m_advancedToggleButton, colors.surface, wxColour());
    minorRow->Add(m_advancedToggleButton, 0);

    buttonSizer->Add(minorRow, 0, wxALIGN_CENTER_HORIZONTAL);

    headerSizer->Add(buttonSizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    mainVerticalSizer->Add(headerSizer, 0, wxEXPAND | wxTOP, 8);

    // 高风险提示
    m_quickDangerLabel = new wxStaticText(this, wxID_ANY, L"");
    m_quickDangerLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                       false, L"微软雅黑"));
    m_quickDangerLabel->SetForegroundColour(colors.warning);
    mainVerticalSizer->Add(m_quickDangerLabel, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 4);

    // 暂停遮罩层（全屏覆盖）
    m_pauseOverlay = new IceClean::Gui::PauseOverlay(this, wxID_ANY,
                                                     wxDefaultPosition, wxDefaultSize);
    m_pauseOverlay->Hide(); // 初始隐藏

    mainSizer->Add(mainVerticalSizer, 0, wxEXPAND | wxTOP, 4);
}

void DeepCleanPanel::CreateAdvancedPanel(wxSizer* mainSizer) {
    const auto& colors = ThemeManager::Instance().GetColors();

    m_advancedPanel = new wxPanel(this);
    m_advancedPanel->SetBackgroundColour(colors.background);
    auto* panelSizer = new wxBoxSizer(wxVERTICAL);
    panelSizer->AddSpacer(4);

    // 顶部工具行：风险筛选 + 记忆上次选择
    auto* toolSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* filterLabel = new wxStaticText(m_advancedPanel, wxID_ANY, L"风险筛选:");
    filterLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    filterLabel->SetForegroundColour(colors.textSecondary);
    toolSizer->Add(filterLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    auto* filter = new wxComboBox(m_advancedPanel, wxID_ANY, L"全部",
                                  wxDefaultPosition, wxSize(110, -1),
                                  wxArrayString(), wxCB_READONLY);
    filter->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                           false, L"微软雅黑"));
    filter->Append(L"全部");
    filter->Append(L"仅安全");
    filter->Append(L"仅谨慎");
    filter->Append(L"仅危险");
    filter->SetSelection(0);
    filter->Bind(wxEVT_COMBOBOX, [this](wxCommandEvent& evt) {
        auto* combo = dynamic_cast<wxComboBox*>(evt.GetEventObject());
        if (!combo) return;
        int sel = combo->GetSelection();
        for (auto& item : m_quickItems) {
            if (sel == 0) {
                item.check->Show();
            } else if (sel == 1) {
                item.check->Show(item.safety == IceClean::Models::SafetyRating::Safe);
            } else if (sel == 2) {
                item.check->Show(item.safety == IceClean::Models::SafetyRating::Caution);
            } else {
                item.check->Show(item.safety == IceClean::Models::SafetyRating::Dangerous);
            }
        }
        m_advancedPanel->Layout();
    });
    toolSizer->Add(filter, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

    auto* memoryCheck = new wxCheckBox(m_advancedPanel, wxID_ANY, L"记忆上次选择");
    memoryCheck->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    memoryCheck->SetValue(m_quickMemoryEnabled);
    memoryCheck->Bind(wxEVT_CHECKBOX, [this, memoryCheck](wxCommandEvent&) {
        m_quickMemoryEnabled = memoryCheck->GetValue();
        ApplyQuickPreferences(true);
    });
    toolSizer->Add(memoryCheck, 0, wxALIGN_CENTER_VERTICAL);

    toolSizer->AddStretchSpacer();
    panelSizer->Add(toolSizer, 0, wxLEFT | wxRIGHT, 12);

    // 可滚动卡片区
    auto* scrollWin = new wxScrolledWindow(m_advancedPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                           wxVSCROLL | wxBORDER_NONE);
    scrollWin->SetBackgroundColour(colors.background);
    scrollWin->SetScrollRate(0, 10);

    auto* listSizer = new wxBoxSizer(wxVERTICAL);

    auto addSection = [&](const wxString& title) {
        auto* header = new wxStaticText(scrollWin, wxID_ANY, title);
        header->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
        header->SetForegroundColour(colors.textSecondary);
        listSizer->Add(header, 0, wxLEFT | wxRIGHT | wxTOP, 14);
    };

    auto addQuickItem = [&](const wxString& id, const wxString& name,
                            IceClean::Models::SafetyRating safety,
                            bool defaultChecked, const wxString& detail) {
        QuickCleanItem item;
        item.id = id;
        item.name = name;
        item.safety = safety;
        item.detail = detail;

        auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        item.check = new wxCheckBox(scrollWin, wxID_ANY, name);
        item.check->SetValue(defaultChecked);
        item.check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
        item.check->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
            ApplyQuickPreferences(true);
            UpdateQuickOverview();
        });
        rowSizer->Add(item.check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(scrollWin);
        badge->SetSafetyRating(safety);
        rowSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        rowSizer->AddStretchSpacer();

        item.sizeLabel = new wxStaticText(scrollWin, wxID_ANY, L"--");
        item.sizeLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                       false, L"微软雅黑"));
        item.sizeLabel->SetForegroundColour(colors.accent);
        rowSizer->Add(item.sizeLabel, 0, wxALIGN_CENTER_VERTICAL);
        listSizer->Add(rowSizer, 0, wxLEFT | wxRIGHT | wxTOP, 12);

        auto* desc = new wxStaticText(scrollWin, wxID_ANY, detail);
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        listSizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_quickItems.push_back(item);
    };

    addSection(L"自动清理（安全项）");
    addQuickItem(L"softwareCache", L"软件缓存",
                 IceClean::Models::SafetyRating::Safe, true,
                 L"微信/QQ/浏览器/开发者工具等缓存，清理后软件可能需重新加载数据。");
    addQuickItem(L"privacy", L"隐私记录",
                 IceClean::Models::SafetyRating::Safe, true,
                 L"浏览器痕迹、最近文档、剪贴板/缩略图缓存等（不含保存的密码）。");

    addSection(L"需确认项（谨慎/危险）");
    addQuickItem(L"winSxS", L"WinSxS 组件清理",
                 IceClean::Models::SafetyRating::Caution, false,
                 L"清理被取代的系统组件，可释放数 GB，需管理员权限（DISM）。");
    addQuickItem(L"compactOS", L"CompactOS 压缩",
                 IceClean::Models::SafetyRating::Caution, false,
                 L"压缩系统文件，可能略微增加 CPU 占用。");
    addQuickItem(L"oldWindows", L"删除旧 Windows 安装",
                 IceClean::Models::SafetyRating::Caution, false,
                 L"删除 Windows.old 等文件，删除后无法回退到旧版本系统。");
    addQuickItem(L"hibernation", L"关闭休眠功能",
                 IceClean::Models::SafetyRating::Caution, false,
                 L"删除 hiberfil.sys，关闭后无法使用休眠功能。");
    addQuickItem(L"registry", L"无效注册表项",
                 IceClean::Models::SafetyRating::Caution, false,
                 L"扫描发现的无效卸载信息/启动项等，清理前自动备份注册表（.reg）。");
    addQuickItem(L"passwords", L"浏览器保存的密码",
                 IceClean::Models::SafetyRating::Dangerous, false,
                 L"删除所有已保存的网站密码，不可恢复！");

    listSizer->AddStretchSpacer();
    scrollWin->SetSizer(listSizer);
    scrollWin->FitInside();

    panelSizer->Add(scrollWin, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
    panelSizer->AddSpacer(4);

    m_advancedPanel->SetSizer(panelSizer);
    m_advancedPanel->Hide();

    mainSizer->Add(m_advancedPanel, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);

    // 高级设置项已创建完毕，应用记忆的偏好
    ApplyQuickPreferences(false);
}

void DeepCleanPanel::OnAdvancedToggle(wxCommandEvent& event) {
    m_advancedVisible = !m_advancedVisible;
    m_advancedPanel->Show(m_advancedVisible);
    m_advancedToggleButton->SetLabel(m_advancedVisible ? L"高级 ▴" : L"高级 ▾");
    Layout();
}

void DeepCleanPanel::WaitIfPaused() {
    while (m_pauseRequested.load() && !m_scanCancelled.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void DeepCleanPanel::OnStopScanButton(wxCommandEvent& event) {
    m_scanCancelled.store(true);
    m_pauseRequested.store(false);   // 解除暂停等待，让线程尽快退出
    if (m_stopScanButton) {
        m_stopScanButton->Enable(false);
    }
    if (m_scanInfoPanel) {
        m_scanInfoPanel->SetState(ScanInfoPanelState::Processing);
        m_scanInfoPanel->SetStatusText(L"正在停止...");
        m_scanInfoPanel->SetProcessingItem(L"");
    }
    if (m_pauseButton) {
        m_pauseButton->SetLabel(L"暂停");
        m_isPaused = false;
    }
    if (m_pauseOverlay) {
        m_pauseOverlay->ShowOverlay(false);
    }
}

void DeepCleanPanel::UpdateQuickOverview(bool updateRing) {
    if (!m_quickRing || !m_quickSizeLabel) return;

    // 统计可用可清理项与已勾选的需确认项
    int availCount = 0;
    int riskCount = 0;
    for (const auto& item : m_quickItems) {
        if (!item.sizeLabel) continue;
        const wxString& label = item.sizeLabel->GetLabelText();
        if (label.empty() || label == L"--" || label == L"无" || label == L"扫描中…") continue;
        availCount++;
        if (item.safety != IceClean::Models::SafetyRating::Safe && item.check->GetValue()) {
            riskCount++;
        }
    }
    m_quickResCount = availCount;
    m_quickDangerCount = riskCount;

    // 健康分只在全部阶段完成后定格一次；
    // 扫描过程中圆环保持转圈，避免用半截数据提前显示 100 分
    if (updateRing) {
        int score = ComputeHealthScore();
        m_quickRing->SetValue(score);
        m_quickRing->SetLabel(wxString::Format(L"%d", score));
        m_quickRing->SetSubLabel(L"健康分");
        m_quickRing->SetIndeterminate(false);
    } else {
        m_quickRing->SetIndeterminate(true);
    }

    wxString sizeText = m_quickEstSize > 0
        ? (wxString(L"预计可释放 ") + Utils::FormatUtil::FormatFileSize(m_quickEstSize))
        : wxString(L"预计可释放 --");
    m_quickSizeLabel->SetLabelText(sizeText);

    if (availCount > 0) {
        m_quickCountLabel->SetLabelText(wxString::Format(L"发现 %d 类可清理项，安全项已自动勾选", availCount));
    } else {
        m_quickCountLabel->SetLabelText(L"点击\"一键扫描\"检测可清理项，安全项将自动勾选");
    }

    if (m_quickDangerCount > 0) {
        m_quickDangerLabel->SetLabelText(
            wxString::Format(L"其中 %d 项为需确认操作", m_quickDangerCount));
        m_quickDangerLabel->Show();
    } else {
        m_quickDangerLabel->SetLabelText(L"");
        m_quickDangerLabel->Hide();
    }
}

void DeepCleanPanel::UpdateTabBadges() {
    if (!m_notebook) return;

    int systemChecked = 0;
    for (const auto& item : m_systemItems) {
        if (item.checkbox->GetValue()) systemChecked++;
    }
    int privacyChecked = 0;
    for (const auto& item : m_privacyItems) {
        if (item.checkbox->GetValue()) privacyChecked++;
    }
    int softFound = 0;
    for (const auto& item : m_softwareCacheItems) {
        if (item.cacheSize > 0) softFound++;
    }

    m_notebook->SetPageText(0, wxString::Format(L"系统清理 (%d)", systemChecked));
    m_notebook->SetPageText(1, wxString::Format(L"注册表清理 (%d)",
                                                static_cast<int>(m_registryItems.size())));
    m_notebook->SetPageText(2, wxString::Format(L"隐私清理 (%d)", privacyChecked));
    m_notebook->SetPageText(3, wxString::Format(L"软件专清 (%d)", softFound));
}

int DeepCleanPanel::ComputeHealthScore() const {
    // 参考值：按预计可释放量与高危项数估算，仅提示性
    long long estMB = static_cast<long long>(m_quickEstSize / (1024 * 1024));
    int score = 100 - static_cast<int>(estMB / 60) - m_quickDangerCount * 3;
    if (score < 10) score = 10;
    if (score > 100) score = 100;
    return score;
}

DeepCleanPanel::QuickCleanItem* DeepCleanPanel::FindQuickItem(const wxString& id) {
    for (auto& item : m_quickItems) {
        if (item.id == id) return &item;
    }
    return nullptr;
}

void DeepCleanPanel::OnQuickScan(wxCommandEvent& event) {
    m_quickScanButton->Enable(false);
    m_quickCleanButton->Enable(false);
    if (m_quickRing) {
        m_quickRing->SetIndeterminate(true);
        m_quickRing->SetLabel(L"...");
        m_quickRing->SetSubLabel(L"扫描中");
    }
    if (m_quickCountLabel) {
        m_quickCountLabel->SetLabelText(L"正在扫描各类缓存...");
    }
    if (m_scanProgressBar) {
        m_scanProgressBar->SetValue(0);
        m_scanProgressBar->SetStatusText(L"初始化...");
    }
    if (m_scanInfoPanel) {
        m_scanInfoPanel->SetState(ScanInfoPanelState::Processing);
        m_scanInfoPanel->SetStatusText(L"正在扫描…");
        m_scanInfoPanel->SetProcessingItem(L"并行扫描：系统缓存 / 注册表 / 软件缓存");
    }

    m_pauseRequested.store(false);
    m_isPaused = false;
    m_scanCancelled.store(false);
    if (m_stopScanButton) {
        m_stopScanButton->Enable(true);
    }
    m_quickEstSize = 0;

    // 统一扫描中文案：所有卡片先置"扫描中…"，清除上一轮残留的混合状态
    for (auto& qi : m_quickItems) {
        if (qi.sizeLabel) qi.sizeLabel->SetLabelText(L"扫描中…");
    }
    if (m_quickCountLabel) {
        m_quickCountLabel->SetLabelText(L"正在并行扫描：系统缓存 / 注册表 / 软件缓存");
    }

    // 三路独立扫描并行执行，共享完成计数器，全部结束后收尾
    const auto remaining = std::make_shared<std::atomic<int>>(3);

    // 阶段A：系统项容量探测（快）
    std::thread([this, remaining]() {
        WaitIfPaused();

        uint64_t hiberSize = IceClean::Utils::FileUtil::GetFileSize(L"C:\\hiberfil.sys");
        uint64_t oldWinSize = 0;
        const wchar_t* oldDirs[] = { L"C:\\Windows.old", L"C:\\$Windows.~BT", L"C:\\$Windows.~WS" };
        for (const wchar_t* dir : oldDirs) {
            if (IceClean::Utils::FileUtil::Exists(dir)) {
                oldWinSize += IceClean::Utils::FileUtil::GetFolderSize(dir);
            }
            WaitIfPaused();
        }

        CallAfter([this, remaining, hiberSize, oldWinSize] {
            if (IsBeingDeleted()) return;

            // 渐进式回填系统卡片（无内容的项同时禁用勾选，避免可选无可清）
            QuickCleanItem* item = nullptr;
            if ((item = FindQuickItem(L"winSxS"))) {
                item->check->Enable(true);
                item->sizeLabel->SetLabelText(L"支持清理");   // 可回收量需 DISM 分析，M2 接入
            }
            if ((item = FindQuickItem(L"compactOS"))) {
                item->check->Enable(true);
                item->sizeLabel->SetLabelText(L"支持清理");
            }
            if ((item = FindQuickItem(L"oldWindows"))) {
                bool has = oldWinSize > 0;
                item->sizeLabel->SetLabelText(
                    has ? wxString(Utils::FormatUtil::FormatFileSize(oldWinSize)) : wxString(L"无"));
                item->check->Enable(has);
                if (!has) item->check->SetValue(false);
            }
            if ((item = FindQuickItem(L"hibernation"))) {
                bool has = hiberSize > 0;
                item->sizeLabel->SetLabelText(
                    has ? wxString(Utils::FormatUtil::FormatFileSize(hiberSize)) : wxString(L"已关闭"));
                item->check->Enable(has);
                if (!has) item->check->SetValue(false);
            }
            m_quickEstSize += hiberSize + oldWinSize;
            UpdateQuickOverview(false);
            if (m_scanProgressBar) {
                // 权重映射：系统 20% / 注册表 20% / 软件缓存 60%
                m_scanProgressBar->SetValue(20);
                m_scanProgressBar->SetStatusText(L"系统缓存 ✓");
            }
            if (remaining->fetch_sub(1) == 1) FinishQuickScan();
        });
    }).detach();

    // 阶段B：无效注册表项（快，结果先到先显示）
    std::thread([this, remaining]() {
        WaitIfPaused();
        auto regItems = IceClean::Core::Cleaner::RegistryCleaner().ScanInvalidItems();

        CallAfter([this, remaining, regItems = std::move(regItems)]() mutable {
            if (IsBeingDeleted()) return;

            // 回填注册表数据（注册表 tab 共用）
            m_registryItems = std::move(regItems);
            m_registryChecked.assign(m_registryItems.size(), false);
            RefreshRegistryList();
            m_registrySelectAllCheck->Enable(!m_registryItems.empty());
            m_registryCleanButton->Enable(!m_registryItems.empty());

            QuickCleanItem* item = nullptr;
            if ((item = FindQuickItem(L"registry"))) {
                item->sizeLabel->SetLabelText(
                    !m_registryItems.empty()
                        ? wxString::Format(L"%d 项", static_cast<int>(m_registryItems.size()))
                        : wxString(L"无"));
            }
            UpdateQuickOverview(false);
            if (m_scanProgressBar) {
                m_scanProgressBar->SetValue(40);
                m_scanProgressBar->SetStatusText(L"注册表 ✓");
            }
            UpdateTabBadges();
            if (remaining->fetch_sub(1) == 1) FinishQuickScan();
        });
    }).detach();

    // 阶段C：软件缓存（最慢，通常决定总耗时）
    std::thread([this, remaining]() {
        WaitIfPaused();

        // 隐私缓存体量探测（浏览器/系统常见缓存目录，供"隐私清理"卡片显示实际大小）
        uint64_t privacyBytes = 0;
        const wchar_t* privacyDirs[] = {
            L"%LOCALAPPDATA%\\Google\\Chrome\\User Data\\Default\\Cache",
            L"%LOCALAPPDATA%\\Google\\Chrome\\User Data\\Default\\Code Cache",
            L"%LOCALAPPDATA%\\Microsoft\\Edge\\User Data\\Default\\Cache",
            L"%LOCALAPPDATA%\\Microsoft\\Edge\\User Data\\Default\\Code Cache",
            L"%APPDATA%\\Mozilla\\Firefox\\Profiles",
            L"%LOCALAPPDATA%\\Microsoft\\Windows\\INetCache",
        };
        for (const wchar_t* d : privacyDirs) {
            std::wstring p = IceClean::Utils::Win32Util::ExpandEnvVars(d);
            if (IceClean::Utils::FileUtil::Exists(p)) {
                privacyBytes += IceClean::Utils::FileUtil::GetFolderSize(p);
            }
            WaitIfPaused();
        }

        WaitIfPaused();
        // 传入取消标志 + 实时回调：软件缓存占整体约 60% 权重，
        // 按已扫描文件数做饱和曲线填充 40→95%，停止时扫描器按文件粒度尽快中止
        auto lastTick = std::make_shared<std::chrono::steady_clock::time_point>(
            std::chrono::steady_clock::now());
        auto softCb = [this, lastTick](int filesScanned, const std::wstring&) {
            auto now = std::chrono::steady_clock::now();
            if (now - *lastTick < std::chrono::milliseconds(150)) return;
            *lastTick = now;
            int pct = 40 + static_cast<int>(
                55.0 * (1.0 - std::exp(-static_cast<double>(filesScanned) / 3000.0)));
            CallAfter([this, pct] {
                if (IsBeingDeleted() || !m_scanProgressBar) return;
                if (pct > static_cast<int>(m_scanProgressBar->GetValue())) {
                    m_scanProgressBar->SetValue(pct);
                }
            });
        };
        auto softResult = m_softwareCacheScanner.Scan(&m_scanCancelled, softCb);

        // 预展开各缓存根路径，避免对每个扫描条目重复解析环境变量
        std::vector<std::wstring> expandedRoots;
        expandedRoots.reserve(m_softwareCacheItems.size());
        for (const auto& cacheItem : m_softwareCacheItems) {
            expandedRoots.push_back(IceClean::Utils::Win32Util::ExpandEnvVars(cacheItem.cachePath));
        }

        std::vector<uint64_t> cacheSizes(m_softwareCacheItems.size(), 0);
        for (const auto& scanItem : softResult.items) {
            for (size_t i = 0; i < expandedRoots.size(); ++i) {
                if (scanItem.path.find(expandedRoots[i]) == 0) {
                    cacheSizes[i] += scanItem.size;
                }
            }
        }

        CallAfter([this, remaining, cacheSizes = std::move(cacheSizes), privacyBytes]() mutable {
            if (IsBeingDeleted()) return;

            // 回填软件缓存数据（软件专清 tab 共用）
            uint64_t softTotal = 0;
            int softFound = 0;
            for (size_t i = 0; i < m_softwareCacheItems.size(); ++i) {
                m_softwareCacheItems[i].cacheSize = cacheSizes[i];
                if (cacheSizes[i] > 0) {
                    m_softwareCacheItems[i].sizeLabel->SetLabelText(
                        wxString(Utils::FormatUtil::FormatFileSize(cacheSizes[i])));
                    m_softwareCacheItems[i].checkbox->SetValue(true);
                    m_softwareCacheItems[i].checkbox->Enable(true);
                    softTotal += cacheSizes[i];
                    softFound++;
                } else {
                    m_softwareCacheItems[i].sizeLabel->SetLabelText(L"0 B");
                    m_softwareCacheItems[i].checkbox->SetValue(false);
                    m_softwareCacheItems[i].checkbox->Enable(false);
                }
            }
            m_softwareSelectAllCheck->Enable(softFound > 0);

            QuickCleanItem* item = nullptr;
            if ((item = FindQuickItem(L"softwareCache"))) {
                item->sizeLabel->SetLabelText(
                    softTotal > 0 ? wxString(Utils::FormatUtil::FormatFileSize(softTotal)) : wxString(L"无"));
                item->check->Enable(softFound > 0);
                // 默认集语义：发现缓存即自动勾选
                item->check->SetValue(softFound > 0);
            }
            if ((item = FindQuickItem(L"privacy"))) {
                // 显示探测到的缓存体量；记录类隐私（历史/跳转列表等）无论如何可清
                item->sizeLabel->SetLabelText(
                    privacyBytes > 0
                        ? wxString(Utils::FormatUtil::FormatFileSize(privacyBytes))
                        : wxString(L"仅记录类"));
                item->check->Enable(true);
            }
            m_quickEstSize += softTotal;
            UpdateQuickOverview(false);
            if (m_scanProgressBar) {
                m_scanProgressBar->SetValue(100);
                m_scanProgressBar->SetStatusText(L"软件缓存 ✓");
            }
            UpdateTabBadges();
            if (remaining->fetch_sub(1) == 1) FinishQuickScan();
        });
    }).detach();
}

void DeepCleanPanel::FinishQuickScan() {
    const bool cancelled = m_scanCancelled.load();
    if (m_scanProgressBar) {
        m_scanProgressBar->SetValue(100);
        m_scanProgressBar->SetStatusText(cancelled ? L"已停止" : L"扫描完成");
    }
    if (m_scanInfoPanel) {
        m_scanInfoPanel->SetState(ScanInfoPanelState::Normal);
        m_scanInfoPanel->SetStatusText(cancelled ? L"扫描已停止" : L"扫描完成");
        m_scanInfoPanel->SetProcessingItem(L"");
    }
    UpdateQuickOverview();
    m_quickScanButton->Enable(true);
    m_quickCleanButton->Enable(true);
    if (m_stopScanButton) {
        m_stopScanButton->Enable(false);
    }

    // 默认集语义：扫描完成后自动勾选安全项，高危项留给用户在高级面板显式加选
    if (QuickCleanItem* it = FindQuickItem(L"registry")) {
        it->check->SetValue(!m_registryItems.empty());
    }
    if (QuickCleanItem* it = FindQuickItem(L"privacy")) {
        it->check->SetValue(true);   // 记录类隐私（历史/跳转列表等）默认包含
    }

    UpdateTabBadges();
}

void DeepCleanPanel::OnPauseButton(wxCommandEvent& event) {
    m_isPaused = !m_isPaused;
    m_pauseRequested.store(m_isPaused);

    if (m_isPaused) {
        // 暂停状态
        if (m_pauseOverlay) {
            m_pauseOverlay->SetPosition(wxPoint(0, 0));
            m_pauseOverlay->SetSize(GetClientSize());
            m_pauseOverlay->ShowOverlay(true);
        }
        if (m_scanInfoPanel) {
            m_scanInfoPanel->SetState(ScanInfoPanelState::Paused);
            m_scanInfoPanel->SetStatusText(L"扫描已暂停，点击继续按钮继续清理");
        }
        if (m_pauseButton) {
            m_pauseButton->SetLabel(L"继续");
        }
    } else {
        // 恢复状态
        if (m_pauseOverlay) {
            m_pauseOverlay->ShowOverlay(false);
        }
        if (m_scanInfoPanel) {
            m_scanInfoPanel->SetState(ScanInfoPanelState::Processing);
        }
        if (m_pauseButton) {
            m_pauseButton->SetLabel(L"暂停");
        }
    }
}

void DeepCleanPanel::OnQuickClean(wxCommandEvent& event) {
    auto isChecked = [this](const wxString& id) -> bool {
        QuickCleanItem* item = FindQuickItem(id);
        return item && item->check->GetValue();
    };

    bool cleanSoftware = isChecked(L"softwareCache");
    bool cleanPrivacy = isChecked(L"privacy");

    // 收集需确认的高影响项
    struct ConfirmEntry { wxString id; wxString name; };
    std::vector<ConfirmEntry> confirmEntries;
    const std::pair<const wchar_t*, const wchar_t*> confirmable[] = {
        { L"winSxS", L"WinSxS 组件清理" },
        { L"compactOS", L"CompactOS 压缩" },
        { L"oldWindows", L"删除旧 Windows 安装" },
        { L"hibernation", L"关闭休眠功能" },
        { L"registry", L"无效注册表项" },
        { L"passwords", L"浏览器保存的密码" },
    };
    for (const auto& pair : confirmable) {
        if (isChecked(pair.first)) {
            confirmEntries.push_back({ pair.first, pair.second });
        }
    }

    if (!cleanSoftware && !cleanPrivacy && confirmEntries.empty()) {
        wxMessageBox(L"当前没有可清理的内容。\n\n"
                     L"安全项（软件缓存 / 隐私 / 注册表）会在扫描后自动勾选；\n"
                     L"高危项（WinSxS、旧系统、休眠等）可在\"高级\"面板中加选。",
                     L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    if (!confirmEntries.empty()) {
        wxString confirmText;
        for (const auto& entry : confirmEntries) {
            confirmText += L"• " + entry.name + L"\n";
        }
        ConfirmDialog dlg(this, L"确认清理高影响项目",
            L"以下项目影响较大，确认继续？\n\n" + confirmText +
            L"\n清理前将自动创建系统还原点，注册表项会单独备份。",
            ConfirmDialog::DangerLevel::Caution, L"确认清理", L"取消");
        if (dlg.ShowModal() != wxID_OK) {
            return;
        }
    }

    m_quickScanButton->Enable(false);
    m_quickCleanButton->Enable(false);
    if (m_quickRing) {
        m_quickRing->SetIndeterminate(true);
        m_quickRing->SetLabel(L"...");
        m_quickRing->SetSubLabel(L"清理中");
    }
    if (m_quickCountLabel) {
        m_quickCountLabel->SetLabelText(L"正在创建还原点并清理...");
    }
    if (m_scanProgressBar) {
        m_scanProgressBar->SetValue(0);
        m_scanProgressBar->SetStatusText(L"初始化...");
    }
    if (m_scanInfoPanel) {
        m_scanInfoPanel->SetState(ScanInfoPanelState::Processing);
        m_scanInfoPanel->SetProcessingItem(L"正在创建系统还原点...");
    }

    m_pauseRequested.store(false);
    m_isPaused = false;

    // 快照注册表项，避免后台线程与 UI 线程并发读写
    const std::vector<IceClean::Core::Cleaner::RegistryInvalidItem> registrySnapshot = m_registryItems;

    std::thread([this, cleanSoftware, cleanPrivacy,
                 registrySnapshot,
                 confirmEntries = std::move(confirmEntries)]() mutable {
        // 创建系统还原点
        CallAfter([this] {
            if (IsBeingDeleted()) return;
            m_scanProgressBar->SetValue(10);
            m_scanProgressBar->SetStatusText(L"创建还原点...");
            m_scanInfoPanel->SetProcessingItem(L"正在创建系统还原点...");
        });

        WaitIfPaused();
        IceClean::Core::Safety::RestorePointManager::CreateRestorePoint(L"IceClean 一键清理前自动还原点");

        uint64_t totalFreed = 0;
        int successCount = 0;
        int failCount = 0;

        const auto addResult = [&](const IceClean::Models::CleanResult& result) {
            totalFreed += result.totalCleanedSize;
            if (result.success) successCount++;
            else failCount++;
        };

        // 1. 软件缓存
        if (cleanSoftware) {
            CallAfter([this] {
                if (IsBeingDeleted()) return;
                m_scanProgressBar->SetValue(30);
                m_scanProgressBar->SetStatusText(L"清理软件缓存...");
                m_scanInfoPanel->SetProcessingItem(L"正在清理软件缓存...");
            });

            for (const auto& cacheItem : m_softwareCacheItems) {
                if (cacheItem.cacheSize <= 0) continue;
                const std::wstring path = IceClean::Utils::Win32Util::ExpandEnvVars(cacheItem.cachePath);
                if (!IceClean::Utils::FileUtil::Exists(path)) continue;
                WIN32_FIND_DATAW findData;
                std::wstring searchPath = path + L"\\*";
                HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        std::wstring itemName(findData.cFileName);
                        if (itemName == L"." || itemName == L"..") continue;
                        std::wstring itemPath = path + L"\\" + itemName;
                        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            IceClean::Utils::FileUtil::DeleteFolder(itemPath);
                        } else {
                            IceClean::Utils::FileUtil::DeleteFilePermanently(itemPath);
                        }
                    } while (FindNextFileW(hFind, &findData));
                    FindClose(hFind);
                }
                successCount++;
            }
            WaitIfPaused();
        }

        if (cleanPrivacy) {
            CallAfter([this] {
                if (IsBeingDeleted()) return;
                m_scanProgressBar->SetValue(60);
                m_scanProgressBar->SetStatusText(L"清理隐私数据...");
                m_scanInfoPanel->SetProcessingItem(L"正在清理隐私数据...");
            });

            std::vector<IceClean::Core::Cleaner::PrivacyType> types = {
                IceClean::Core::Cleaner::PrivacyType::Cookies,
                IceClean::Core::Cleaner::PrivacyType::History,
                IceClean::Core::Cleaner::PrivacyType::FormData,
                IceClean::Core::Cleaner::PrivacyType::Cache,
                IceClean::Core::Cleaner::PrivacyType::Session,
                IceClean::Core::Cleaner::PrivacyType::RecentDocs,
                IceClean::Core::Cleaner::PrivacyType::RunHistory,
                IceClean::Core::Cleaner::PrivacyType::SearchHistory,
                IceClean::Core::Cleaner::PrivacyType::ClipboardHistory,
                IceClean::Core::Cleaner::PrivacyType::JumpList,
                IceClean::Core::Cleaner::PrivacyType::ThumbnailCache,
                IceClean::Core::Cleaner::PrivacyType::OfficeRecent,
                IceClean::Core::Cleaner::PrivacyType::ArchiveHistory,
                IceClean::Core::Cleaner::PrivacyType::DownloadHistory,
            };
            WaitIfPaused();
            IceClean::Core::Cleaner::PrivacyCleaner privacyCleaner;
            addResult(privacyCleaner.CleanPrivacy(types));
        }

        // 3. 确认的高影响项
        CallAfter([this] {
            if (IsBeingDeleted()) return;
            m_scanProgressBar->SetValue(90);
            m_scanProgressBar->SetStatusText(L"清理高危项...");
            m_scanInfoPanel->SetProcessingItem(L"正在清理高危项...");
        });

        for (const auto& entry : confirmEntries) {
            WaitIfPaused();
            if (entry.id == L"winSxS") {
                CallAfter([this] {
                    if (IsBeingDeleted()) return;
                    m_scanInfoPanel->SetProcessingItem(L"正在清理 WinSxS...");
                });
                IceClean::Core::Cleaner::DismCleaner dism;
                addResult(dism.Clean({ L"WinSxS" }));
            }
            else if (entry.id == L"compactOS") {
                CallAfter([this] {
                    if (IsBeingDeleted()) return;
                    m_scanInfoPanel->SetProcessingItem(L"正在清理 CompactOS...");
                });
                IceClean::Core::Cleaner::DismCleaner dism;
                addResult(dism.Clean({ L"CompactOS" }));
            }
            else if (entry.id == L"oldWindows") {
                CallAfter([this] {
                    if (IsBeingDeleted()) return;
                    m_scanInfoPanel->SetProcessingItem(L"正在清理旧系统文件...");
                });
                std::vector<std::wstring> paths = {
                    L"C:\\Windows.old", L"C:\\$Windows.~BT", L"C:\\$Windows.~WS"
                };
                IceClean::Core::Cleaner::FileCleaner fileCleaner;
                addResult(fileCleaner.Clean(paths));
            }
            else if (entry.id == L"hibernation") {
                CallAfter([this] {
                    if (IsBeingDeleted()) return;
                    m_scanInfoPanel->SetProcessingItem(L"正在关闭休眠功能...");
                });
                IceClean::Core::Cleaner::HibernationCleaner hib;
                addResult(hib.Clean({}));
            }
            else if (entry.id == L"registry") {
                CallAfter([this] {
                    if (IsBeingDeleted()) return;
                    m_scanInfoPanel->SetProcessingItem(L"正在清理注册表...");
                });
                IceClean::Core::Cleaner::RegistryCleaner regCleaner;
                addResult(regCleaner.Clean(registrySnapshot, GenerateRegistryBackupPath()));
            }
            else if (entry.id == L"passwords") {
                CallAfter([this] {
                    if (IsBeingDeleted()) return;
                    m_scanInfoPanel->SetProcessingItem(L"正在清理密码...");
                });
                IceClean::Core::Cleaner::PrivacyCleaner privacyCleaner;
                addResult(privacyCleaner.CleanPrivacy({
                    IceClean::Core::Cleaner::PrivacyType::Passwords
                }));
            }
        }

        // 记录操作日志
        IceClean::Models::OperationRecord record;
        record.type = IceClean::Models::OperationType::Clean;
        record.description = L"一键深度清理";
        record.size = totalFreed;
        record.timestamp = std::chrono::system_clock::now();
        record.success = failCount == 0;
        IceClean::Core::Safety::OperationLogger::LogOperation(record);

        CallAfter([this, totalFreed, successCount, failCount]() {
            if (IsBeingDeleted()) return;

            if (m_scanProgressBar) {
                m_scanProgressBar->SetValue(100);
                m_scanProgressBar->SetStatusText(L"清理完成");
            }
            if (m_scanInfoPanel) {
                m_scanInfoPanel->SetState(ScanInfoPanelState::Normal);
                m_scanInfoPanel->SetProcessingItem(L"清理完成");
            }

            m_quickScanButton->Enable(true);
            m_quickCleanButton->Enable(true);
            if (m_quickRing) {
                m_quickRing->SetIndeterminate(false);
                m_quickRing->SetLabel(L"完成");
                m_quickRing->SetSubLabel(L"清理完成");
            }
            wxString msg = wxString::Format(L"清理完成：成功 %d，失败 %d，释放 ",
                                            successCount, failCount);
            msg += Utils::FormatUtil::FormatFileSize(totalFreed);
            if (m_quickCountLabel) {
                m_quickCountLabel->SetLabelText(msg);
            }

            // 清理后自动重扫，刷新概览与各分类结果
            wxCommandEvent scanEvt(wxEVT_BUTTON, m_quickScanButton->GetId());
            m_quickScanButton->GetEventHandler()->AddPendingEvent(scanEvt);
        });
    }).detach();
}

std::wstring DeepCleanPanel::GetQuickPreferencesPath() const {
    std::wstring configPath = IceClean::Utils::JsonUtil::GetConfigPath();
    size_t lastSlash = configPath.rfind(L'\\');
    if (lastSlash != std::wstring::npos) {
        return configPath.substr(0, lastSlash) + L"\\quick_clean_prefs.json";
    }
    return configPath;
}

void DeepCleanPanel::ApplyQuickPreferences(bool save) {
    const std::wstring path = GetQuickPreferencesPath();

    if (save) {
        nlohmann::json json;
        json["memoEnabled"] = m_quickMemoryEnabled;
        nlohmann::json checks = nlohmann::json::object();
        for (const auto& item : m_quickItems) {
            checks[item.id.ToStdString()] = item.check->GetValue();
        }
        json["checks"] = std::move(checks);
        IceClean::Utils::JsonUtil::SaveJson(path, json);
        return;
    }

    auto json = IceClean::Utils::JsonUtil::LoadJson(path);
    if (!json.is_object()) return;

    m_quickMemoryEnabled = json.value("memoEnabled", m_quickMemoryEnabled);
    if (json.contains("checks") && json["checks"].is_object()) {
        const auto& checks = json["checks"];
        for (auto& item : m_quickItems) {
            const std::string key = item.id.ToStdString();
            if (checks.contains(key)) {
                item.check->SetValue(checks.value(key, false));
            }
        }
    }
}

std::wstring DeepCleanPanel::GenerateRegistryBackupPath() {
    std::wstring backupDir;
    wchar_t exePath[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
        std::filesystem::path p(exePath);
        std::filesystem::path dir = p.parent_path() / L"data" / L"registry_backup";
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        backupDir = dir.wstring();
    }

    std::wstring backupPath;
    if (!backupDir.empty()) {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        struct tm tmBuf {};
        localtime_s(&tmBuf, &timeT);
        wchar_t timeStr[32] = {};
        wcsftime(timeStr, 32, L"%Y%m%d_%H%M%S", &tmBuf);
        backupPath = backupDir + L"\\reg_backup_" + timeStr + L".reg";

        // 清理旧备份（保留最近5次）
        WIN32_FIND_DATAW findData;
        std::wstring searchPattern = backupDir + L"\\reg_backup_*.reg";
        std::vector<std::wstring> backupFiles;
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    backupFiles.push_back(backupDir + L"\\" + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        std::sort(backupFiles.begin(), backupFiles.end());
        while (backupFiles.size() > 5) {
            DeleteFileW(backupFiles.front().c_str());
            backupFiles.erase(backupFiles.begin());
        }
    }
    return backupPath;
}

void DeepCleanPanel::RefreshRegistryList() {
    m_registryList->DeleteAllItems();
    for (int i = 0; i < static_cast<int>(m_registryItems.size()); ++i) {
        const auto& item = m_registryItems[i];
        long idx = m_registryList->InsertItem(i, L" ", 0);  // 0=未勾选图片
        m_registryList->SetItem(idx, 1, GetTypeString(item.type));
        m_registryList->SetItem(idx, 2, item.keyPath);
        m_registryList->SetItem(idx, 3, item.description);
    }
    UpdateTabBadges();
}

void DeepCleanPanel::CreateSystemCleanTab(wxWindow* parent) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 全选
    auto* selectAllSizer = new wxBoxSizer(wxHORIZONTAL);
    selectAllSizer->AddStretchSpacer();
    m_systemSelectAllCheck = new wxCheckBox(parent, wxID_ANY, L"全选");
    m_systemSelectAllCheck->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_systemSelectAllCheck->Bind(wxEVT_CHECKBOX, &DeepCleanPanel::OnSystemSelectAll, this);
    selectAllSizer->Add(m_systemSelectAllCheck, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(selectAllSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // WinSxS组件清理
    {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* check = new wxCheckBox(parent, wxID_ANY, L"WinSxS组件清理");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(parent);
        badge->SetSafetyRating(IceClean::Models::SafetyRating::Caution);
        itemSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);

        sizer->Add(itemSizer, 0, wxLEFT | wxRIGHT, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理WinSxS文件夹中的被取代组件，可释放2-10GB空间。需要管理员权限。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_systemItems.push_back({check, L"winSxS",
            L"清理WinSxS文件夹中的被取代组件", false});
    }

    // CompactOS压缩
    {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* check = new wxCheckBox(parent, wxID_ANY, L"CompactOS压缩");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(parent);
        badge->SetSafetyRating(IceClean::Models::SafetyRating::Safe);
        itemSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);

        sizer->Add(itemSizer, 0, wxLEFT | wxRIGHT, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"压缩Windows系统文件，可释放2-5GB空间。不影响系统运行，但可能略微增加CPU使用率。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_systemItems.push_back({check, L"compactOS",
            L"压缩Windows系统文件", false});
    }

    // 旧Windows安装
    {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* check = new wxCheckBox(parent, wxID_ANY, L"删除旧Windows安装");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(parent);
        badge->SetSafetyRating(IceClean::Models::SafetyRating::Caution);
        itemSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);

        sizer->Add(itemSizer, 0, wxLEFT | wxRIGHT, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"删除Windows.old、$Windows.~BT、$Windows.~WS等旧安装文件。"
            L"删除后将无法回退到旧版本Windows，可释放10-50GB空间。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_systemItems.push_back({check, L"oldWindows",
            L"删除旧Windows安装文件", true});
    }

    // 休眠文件清理
    {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* check = new wxCheckBox(parent, wxID_ANY, L"关闭休眠功能");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(parent);
        badge->SetSafetyRating(IceClean::Models::SafetyRating::Caution);
        itemSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);

        sizer->Add(itemSizer, 0, wxLEFT | wxRIGHT, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"删除hiberfil.sys休眠文件，可释放4-32GB空间。关闭后无法使用休眠功能，快速启动也会受影响。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_systemItems.push_back({check, L"hibernation",
            L"关闭休眠功能并删除休眠文件", true});
    }

    // 单项变更时反向同步全选状态
    for (const auto& item : m_systemItems) {
        item.checkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
            UpdateSelectAllChecked(m_systemItems, m_systemSelectAllCheck);
        });
    }

    sizer->AddStretchSpacer();
    parent->SetSizer(sizer);
}

void DeepCleanPanel::CreateRegistryCleanTab(wxWindow* parent) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 顶部按钮区域
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);

    m_registryScanButton = new wxButton(parent, wxID_ANY, L"扫描注册表",
                                         wxDefaultPosition, wxSize(120, 36));
    m_registryScanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
    m_registryScanButton->SetBackgroundColour(colors.surface);
    m_registryScanButton->SetForegroundColour(colors.textPrimary);
    m_registryScanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnRegistryScan, this);
    ThemeManager::Instance().ApplyButtonHover(m_registryScanButton, colors.surface, wxColour());
    TrackButtonColor(m_registryScanButton, colors.surface, wxColour());
    topSizer->Add(m_registryScanButton, 0, wxRIGHT, 12);

    m_registryCleanButton = new wxButton(parent, wxID_ANY, L"清理选中项",
                                          wxDefaultPosition, wxSize(120, 36));
    m_registryCleanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_registryCleanButton->SetBackgroundColour(colors.surface);
    m_registryCleanButton->SetForegroundColour(colors.textPrimary);
    m_registryCleanButton->Enable(false);
    m_registryCleanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnRegistryClean, this);
    ThemeManager::Instance().ApplyButtonHover(m_registryCleanButton, colors.surface, wxColour());
    TrackButtonColor(m_registryCleanButton, colors.surface, wxColour());
    topSizer->Add(m_registryCleanButton, 0, wxRIGHT, 12);

    m_registrySelectAllCheck = new wxCheckBox(parent, wxID_ANY, L"全选");
    m_registrySelectAllCheck->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                              false, L"微软雅黑"));
    m_registrySelectAllCheck->Enable(false);
    m_registrySelectAllCheck->Bind(wxEVT_CHECKBOX, &DeepCleanPanel::OnRegistrySelectAll, this);
    topSizer->Add(m_registrySelectAllCheck, 0, wxALIGN_CENTER_VERTICAL);

    topSizer->AddStretchSpacer();

    m_registryStatusLabel = new wxStaticText(parent, wxID_ANY,
                                             L"点击\"扫描注册表\"检测无效注册表项");
    m_registryStatusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                           false, L"微软雅黑"));
    m_registryStatusLabel->SetForegroundColour(colors.textSecondary);
    topSizer->Add(m_registryStatusLabel, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(topSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // 列表控件
    m_registryList = new wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxLC_REPORT | wxBORDER_SIMPLE);
    m_registryList->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));

    // 创建复选框图片列表（0=未勾选, 1=已勾选）
    auto* imgList = new wxImageList(16, 16, true, 2);
    wxBitmap uncheckedBmp(16, 16);
    wxBitmap checkedBmp(16, 16);
    {
        wxMemoryDC dc;
        // 未勾选
        dc.SelectObject(uncheckedBmp);
        dc.SetBackground(colors.background);
        dc.Clear();
        dc.SetPen(wxPen(colors.border, 1));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(2, 2, 12, 12);
        dc.SelectObject(wxNullBitmap);
        // 已勾选
        dc.SelectObject(checkedBmp);
        dc.SetBackground(colors.background);
        dc.Clear();
        dc.SetPen(wxPen(colors.accent, 1));
        dc.SetBrush(wxBrush(colors.accent));
        dc.DrawRectangle(2, 2, 12, 12);
        dc.SetPen(wxPen(*wxWHITE, 2));
        dc.DrawLine(4, 8, 7, 11);
        dc.DrawLine(7, 11, 12, 4);
        dc.SelectObject(wxNullBitmap);
    }
    imgList->Add(uncheckedBmp);
    imgList->Add(checkedBmp);
    m_registryList->AssignImageList(imgList, wxIMAGE_LIST_SMALL);

    // 添加列（第一列为复选框列）
    m_registryList->AppendColumn(L" ", wxLIST_FORMAT_CENTER, 30);
    m_registryList->AppendColumn(L"类型", wxLIST_FORMAT_LEFT, 110);
    m_registryList->AppendColumn(L"注册表路径", wxLIST_FORMAT_LEFT, 280);
    m_registryList->AppendColumn(L"描述", wxLIST_FORMAT_LEFT, 180);

    // 点击行时切换复选框（使用左键按下事件，单击即可触发）
    m_registryList->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& mouseEvent) {
        int flags = wxLIST_HITTEST_ONITEM;
        long idx = m_registryList->HitTest(mouseEvent.GetPosition(), flags);
        if (idx >= 0 && idx < static_cast<long>(m_registryChecked.size())) {
            // 切换复选框状态
            m_registryChecked[idx] = !m_registryChecked[idx];
            m_registryList->SetItemImage(idx, m_registryChecked[idx] ? 1 : 0);

            // 更新全选复选框状态
            bool allChecked = true;
            for (size_t i = 0; i < m_registryChecked.size(); ++i) {
                if (!m_registryChecked[i]) { allChecked = false; break; }
            }
            m_registrySelectAllCheck->SetValue(allChecked);
        }
        mouseEvent.Skip();
    });

    sizer->Add(m_registryList, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(8);

    parent->SetSizer(sizer);
}

void DeepCleanPanel::CreatePrivacyCleanTab(wxWindow* parent) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 全选
    auto* selectAllSizer = new wxBoxSizer(wxHORIZONTAL);
    selectAllSizer->AddStretchSpacer();
    m_privacySelectAllCheck = new wxCheckBox(parent, wxID_ANY, L"全选");
    m_privacySelectAllCheck->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                            false, L"微软雅黑"));
    m_privacySelectAllCheck->Bind(wxEVT_CHECKBOX, &DeepCleanPanel::OnPrivacySelectAll, this);
    selectAllSizer->Add(m_privacySelectAllCheck, 0, wxALIGN_CENTER_VERTICAL);
    sizer->Add(selectAllSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // 浏览器Cookies
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器Cookies");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Chrome/Edge/Firefox/Brave/Vivaldi/Opera的Cookies数据。清理后需要重新登录网站。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"cookies",
            L"清理浏览器Cookies数据"});
    }

    // 浏览器历史记录
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器历史记录");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理浏览器浏览历史记录，包括下载历史。清理后无法恢复访问记录。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"history",
            L"清理浏览器浏览历史记录"});
    }

    // 表单自动填充数据
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"表单自动填充数据");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理浏览器保存的表单自动填充数据，包括用户名、地址等。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"formData",
            L"清理浏览器表单自动填充数据"});
    }

    // 浏览器缓存
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器缓存");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理浏览器缓存文件，可释放磁盘空间。清理后网页首次加载可能稍慢。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"browserCache",
            L"清理浏览器缓存文件"});
    }

    // 浏览器会话数据
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器会话数据");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理浏览器保存的标签页和会话恢复数据。清理后浏览器无法恢复上次打开的标签页。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"session",
            L"清理浏览器会话数据"});
    }

    // 浏览器保存的密码
    {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器保存的密码");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(parent);
        badge->SetSafetyRating(IceClean::Models::SafetyRating::Dangerous);
        itemSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);

        sizer->Add(itemSizer, 0, wxLEFT | wxRIGHT, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"⚠ 清理后所有保存的网站密码将被删除，需要重新输入所有密码。此操作不可恢复！");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.danger);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"passwords",
            L"清理浏览器保存的密码", true});
    }

    // 最近文档记录
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"最近文档记录");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Windows最近打开的文档、文件夹快捷方式和跳转列表。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"recentDocs",
            L"清理最近文档记录"});
    }

    // 剪贴板历史
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"剪贴板历史");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Windows剪贴板历史记录（Win+V）。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"clipboardHistory",
            L"清理剪贴板历史记录"});
    }

    // Office最近文件
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"Office最近文件记录");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Microsoft Office(Word/Excel/PowerPoint)的最近打开文件记录。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"officeRecent",
            L"清理Office最近文件记录"});
    }

    // 压缩软件历史
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"压缩软件历史记录");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理WinRAR/7-Zip等压缩软件的历史记录和临时解压文件。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"archiveHistory",
            L"清理压缩软件历史记录"});
    }

    // 下载器历史
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"下载器历史记录");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理迅雷/IDM等下载工具的下载历史记录。不会删除已下载的文件。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"downloadHistory",
            L"清理下载器历史记录"});
    }

    // 缩略图缓存
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"缩略图缓存");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Windows资源管理器的缩略图缓存。清理后首次打开文件夹时缩略图会重新生成。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"thumbnailCache",
            L"清理缩略图缓存"});
    }

    // 单项变更时反向同步全选状态
    for (const auto& item : m_privacyItems) {
        item.checkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
            UpdateSelectAllChecked(m_privacyItems, m_privacySelectAllCheck);
        });
    }

    sizer->AddStretchSpacer();
    parent->SetSizer(sizer);
}

void DeepCleanPanel::OnCleanButton(wxCommandEvent& event) {
    int currentPage = m_notebook->GetSelection();
    bool isSystem = (currentPage == 1);
    bool isPrivacy = (currentPage == 3);
    if (!isSystem && !isPrivacy) return;

    // 收集当前tab的勾选项
    std::vector<wxString> selectedIds;
    wxString tabName;
    bool hasDangerous = false;
    wxString dangerousItems;

    if (isSystem) {
        tabName = L"系统清理";
        for (const auto& item : m_systemItems) {
            if (item.checkbox->GetValue()) {
                selectedIds.push_back(item.id);
                if (item.isDangerous) {
                    hasDangerous = true;
                    dangerousItems += L"• " + item.checkbox->GetLabel() + L"\n";
                }
            }
        }
    } else {
        tabName = L"隐私清理";
        for (const auto& item : m_privacyItems) {
            if (item.checkbox->GetValue()) {
                selectedIds.push_back(item.id);
                if (item.isDangerous) {
                    hasDangerous = true;
                    dangerousItems += L"• " + item.checkbox->GetLabel() + L"\n";
                }
            }
        }
    }

    if (selectedIds.empty()) {
        wxMessageBox(wxString::Format(L"请在\"%s\"标签页中至少勾选一项。", tabName.wx_str()),
                     L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    if (hasDangerous) {
        ConfirmDialog dlg(this, L"确认操作",
            L"以下操作可能影响系统功能，确认继续？\n\n" + dangerousItems +
            L"\n操作前将自动创建系统还原点。",
            ConfirmDialog::DangerLevel::Caution, L"确认清理", L"取消");

        if (dlg.ShowModal() != wxID_OK) {
            return;
        }
    }

    // 发送深度清理事件，携带选中项ID
    wxThreadEvent cleanEvt(wxEVT_CLEAN_PROGRESS);
    cleanEvt.SetInt(1);  // 1=深度清理
    cleanEvt.SetPayload(selectedIds);
    wxPostEvent(GetParent(), cleanEvt);
}

std::vector<wxString> DeepCleanPanel::GetSelectedIds() const {
    std::vector<wxString> ids;
    for (const auto& item : m_systemItems) {
        if (item.checkbox->GetValue()) {
            ids.push_back(item.id);
        }
    }
    for (const auto& item : m_privacyItems) {
        if (item.checkbox->GetValue()) {
            ids.push_back(item.id);
        }
    }
    return ids;
}

void DeepCleanPanel::OnRegistryScan(wxCommandEvent& event) {
    m_registryScanButton->Enable(false);
    m_registryStatusLabel->SetLabelText(L"正在扫描...");

    // 在后台线程执行扫描
    std::thread([this]() {
        IceClean::Core::Cleaner::RegistryCleaner cleaner;
        auto items = cleaner.ScanInvalidItems();

        // 回到主线程更新UI
        CallAfter([this, items = std::move(items)]() mutable {
            m_registryItems = std::move(items);
            m_registryChecked.assign(m_registryItems.size(), false);
            RefreshRegistryList();

            m_registryStatusLabel->SetLabelText(
                wxString::Format(L"共发现 %d 个无效注册表项", static_cast<int>(m_registryItems.size())));
            m_registryScanButton->Enable(true);
            m_registryCleanButton->Enable(!m_registryItems.empty());
            m_registrySelectAllCheck->Enable(!m_registryItems.empty());
        });
    }).detach();
}

void DeepCleanPanel::OnRegistryClean(wxCommandEvent& event) {
    // 收集勾选的项
    std::vector<IceClean::Core::Cleaner::RegistryInvalidItem> selectedItems;
    for (int i = 0; i < static_cast<int>(m_registryItems.size()); ++i) {
        if (i < static_cast<int>(m_registryChecked.size()) && m_registryChecked[i]) {
            selectedItems.push_back(m_registryItems[i]);
        }
    }

    if (selectedItems.empty()) {
        wxMessageBox(L"请至少勾选一项要清理的注册表项。", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    ConfirmDialog dlg(this, L"确认注册表清理",
        wxString::Format(L"即将清理 %d 个无效注册表项。\n\n清理前将自动备份注册表，确认继续？",
                          static_cast<int>(selectedItems.size())),
        ConfirmDialog::DangerLevel::Caution, L"确认清理", L"取消");

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    m_registryCleanButton->Enable(false);
    m_registryStatusLabel->SetLabelText(L"正在备份注册表...");

    // 生成自动备份路径
    // 生成自动备份路径（含旧备份清理）
    std::wstring backupPath = GenerateRegistryBackupPath();

    m_registryStatusLabel->SetLabelText(L"正在清理...");

    // 在后台线程执行清理
    std::thread([this, selectedItems = std::move(selectedItems), backupPath]() mutable {
        IceClean::Core::Cleaner::RegistryCleaner cleaner;
        auto result = cleaner.Clean(selectedItems, backupPath, nullptr);

        CallAfter([this, result = std::move(result)]() mutable {
            m_registryStatusLabel->SetLabelText(
                wxString::Format(L"清理完成，成功 %d 项，失败 %d 项", result.cleanedFileCount, result.failedFileCount));
            m_registryCleanButton->Enable(true);

            // 触发重新扫描
            wxCommandEvent scanEvt(wxEVT_BUTTON, m_registryScanButton->GetId());
            m_registryScanButton->GetEventHandler()->AddPendingEvent(scanEvt);
        });
    }).detach();
}

void DeepCleanPanel::OnSystemSelectAll(wxCommandEvent& event) {
    bool select = m_systemSelectAllCheck->GetValue();
    for (auto& item : m_systemItems) {
        item.checkbox->SetValue(select);
    }
    UpdateTabBadges();
}

void DeepCleanPanel::OnPrivacySelectAll(wxCommandEvent& event) {
    bool select = m_privacySelectAllCheck->GetValue();
    for (auto& item : m_privacyItems) {
        item.checkbox->SetValue(select);
    }
    UpdateTabBadges();
}

void DeepCleanPanel::OnRegistrySelectAll(wxCommandEvent& event) {
    bool select = m_registrySelectAllCheck->GetValue();
    for (int i = 0; i < m_registryList->GetItemCount(); ++i) {
        m_registryChecked[i] = select;
        m_registryList->SetItemImage(i, select ? 1 : 0);
    }
}

void DeepCleanPanel::OnSoftwareSelectAll(wxCommandEvent& event) {
    bool select = m_softwareSelectAllCheck->GetValue();
    for (auto& item : m_softwareCacheItems) {
        if (item.checkbox->IsEnabled()) {
            item.checkbox->SetValue(select);
        }
    }
    UpdateTabBadges();
}

wxString DeepCleanPanel::GetTypeString(IceClean::Core::Cleaner::RegistryInvalidItem::Type type) const {
    using Type = IceClean::Core::Cleaner::RegistryInvalidItem::Type;
    switch (type) {
        case Type::InvalidUninstall:  return L"无效卸载信息";
        case Type::InvalidStartup:    return L"无效启动项";
        case Type::InvalidFileAssoc:  return L"无效文件关联";
        case Type::InvalidSharedDLL:  return L"无效共享DLL";
        case Type::InvalidFont:       return L"无效字体引用";
        case Type::InvalidHelpFile:   return L"无效帮助文件";
        case Type::InvalidAppPath:    return L"无效应用路径";
        case Type::InvalidCOM:        return L"无效COM组件";
        case Type::InvalidMUI:        return L"无效MUI缓存";
        case Type::InvalidEnvVar:     return L"无效环境变量";
        case Type::InvalidTrayNotify: return L"无效托盘缓存";
        case Type::InvalidSound:      return L"无效声音关联";
        default:                      return L"其他";
    }
}

void DeepCleanPanel::CreateSoftwareCacheTab(wxWindow* parent) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 顶部按钮区域
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);

    m_softwareScanButton = new wxButton(parent, wxID_ANY, L"扫描缓存",
                                         wxDefaultPosition, wxSize(120, 36));
    m_softwareScanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
    m_softwareScanButton->SetBackgroundColour(colors.surface);
    m_softwareScanButton->SetForegroundColour(colors.textPrimary);
    m_softwareScanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnSoftwareScan, this);
    ThemeManager::Instance().ApplyButtonHover(m_softwareScanButton, colors.surface, wxColour());
    TrackButtonColor(m_softwareScanButton, colors.surface, wxColour());
    topSizer->Add(m_softwareScanButton, 0, wxRIGHT, 12);

    m_softwareCleanButton = new wxButton(parent, wxID_ANY, L"清理选中",
                                          wxDefaultPosition, wxSize(120, 36));
    m_softwareCleanButton->SetName("btn_primary_software_clean");
    m_softwareCleanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_softwareCleanButton->SetBackgroundColour(colors.accent);
    m_softwareCleanButton->SetForegroundColour(*wxWHITE);
    m_softwareCleanButton->Enable(false);
    m_softwareCleanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnSoftwareClean, this);
    ThemeManager::Instance().ApplyButtonHover(m_softwareCleanButton, colors.accent, wxColour());
    TrackButtonColor(m_softwareCleanButton, colors.accent, wxColour());
    topSizer->Add(m_softwareCleanButton, 0, wxRIGHT, 12);

    m_softwareSelectAllCheck = new wxCheckBox(parent, wxID_ANY, L"全选");
    m_softwareSelectAllCheck->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                             false, L"微软雅黑"));
    m_softwareSelectAllCheck->Bind(wxEVT_CHECKBOX, &DeepCleanPanel::OnSoftwareSelectAll, this);
    topSizer->Add(m_softwareSelectAllCheck, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    topSizer->AddStretchSpacer();

    m_softwareStatusLabel = new wxStaticText(parent, wxID_ANY, L"点击\"扫描缓存\"检测常用软件缓存");
    m_softwareStatusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_softwareStatusLabel->SetForegroundColour(colors.textDisabled);
    topSizer->Add(m_softwareStatusLabel, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(topSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // 软件列表区域（使用wxScrolledWindow）
    auto* scrollWin = new wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                            wxVSCROLL | wxBORDER_NONE);
    scrollWin->SetBackgroundColour(colors.background);
    scrollWin->SetScrollRate(0, 10);

    auto* listSizer = new wxBoxSizer(wxVERTICAL);

    // 定义软件缓存项
    struct SoftwareDef {
        const wchar_t* name;
        const wchar_t* path;
        const wchar_t* desc;
    };

    // 分组头
    auto addSectionHeader = [&](const wxString& title) {
        auto* headerLabel = new wxStaticText(scrollWin, wxID_ANY, title);
        headerLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                    false, L"微软雅黑"));
        headerLabel->SetForegroundColour(colors.textSecondary);
        listSizer->Add(headerLabel, 0, wxLEFT | wxRIGHT | wxTOP, 16);
    };

    const SoftwareDef softwareDefs[] = {
        { L"微信缓存", L"%USERPROFILE%\\Documents\\WeChat Files", L"微信聊天图片、视频、文件缓存" },
        { L"QQ缓存", L"%USERPROFILE%\\Documents\\Tencent Files", L"QQ聊天图片、视频缓存" },
        { L"迅雷缓存", L"%USERPROFILE%\\AppData\\Local\\Thunder Network", L"迅雷下载缓存和临时文件" },
        { L"爱奇艺缓存", L"%USERPROFILE%\\AppData\\Local\\Qiyi", L"爱奇艺视频缓存" },
        { L"腾讯视频缓存", L"%USERPROFILE%\\AppData\\Local\\Tencent\\QLive", L"腾讯视频缓存" },
        { L"优酷缓存", L"%USERPROFILE%\\AppData\\Local\\Youku", L"优酷视频缓存" },
        { L"哔哩哔哩缓存", L"%USERPROFILE%\\AppData\\Local\\bilibili", L"哔哩哔哩视频缓存" },
        { L"WPS缓存", L"%USERPROFILE%\\AppData\\Local\\Kingsoft", L"WPS云端缓存和临时文件" },
        { L"钉钉缓存", L"%USERPROFILE%\\AppData\\Local\\DingTalk", L"钉钉缓存文件" },
        { L"有道云笔记缓存", L"%USERPROFILE%\\AppData\\Local\\Youdao", L"有道云笔记和词典缓存" },
    };

    for (const auto& def : softwareDefs) {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* check = new wxCheckBox(scrollWin, wxID_ANY, def.name);
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* sizeLabel = new wxStaticText(scrollWin, wxID_ANY, L"");
        sizeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
        sizeLabel->SetForegroundColour(colors.accent);
        sizeLabel->SetMinSize(wxSize(80, -1));
        itemSizer->Add(sizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        listSizer->Add(itemSizer, 0, wxLEFT | wxRIGHT | wxTOP, 12);

        auto* desc = new wxStaticText(scrollWin, wxID_ANY, def.desc);
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        listSizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_softwareCacheItems.push_back({check, def.name, def.path, sizeLabel, 0});
    }

    // 开发者工具缓存
    addSectionHeader(L"开发者工具缓存");

    const SoftwareDef devDefs[] = {
        { L"npm缓存", L"%USERPROFILE%\\.npm\\_cacache", L"npm 7+ 包下载缓存(CACache)" },
        { L"npm旧版缓存", L"%APPDATA%\\npm-cache", L"npm 6 及以下版本缓存目录" },
        { L"Yarn缓存", L"%USERPROFILE%\\.yarn\\cache", L"Yarn 1.x 包缓存" },
        { L"Yarn Berry缓存", L"%USERPROFILE%\\.yarn\\berry", L"Yarn 2+ Berry 全局缓存" },
        { L"pnpm缓存", L"%APPDATA%\\pnpm-store", L"pnpm 内容寻址缓存存储" },
        { L"pip缓存", L"%LOCALAPPDATA%\\pip\\Cache", L"Python pip 包下载缓存" },
        { L"uv缓存", L"%LOCALAPPDATA%\\uv\\cache", L"Python uv 包管理器缓存" },
        { L"Poetry缓存", L"%USERPROFILE%\\.cache\\pypoetry\\cache", L"Python Poetry 包缓存" },
        { L"NuGet缓存", L"%LOCALAPPDATA%\\NuGet\\Cache", L"C#/.NET NuGet 包缓存" },
        { L"Cargo缓存", L"%USERPROFILE%\\.cargo\\registry", L"Rust Cargo 注册表缓存" },
        { L"Go模块缓存", L"%USERPROFILE%\\go\\pkg\\mod\\cache", L"Go 模块下载缓存" },
        { L"Gradle缓存", L"%USERPROFILE%\\.gradle\\caches", L"Gradle 构建缓存(Java/Android)" },
        { L"Maven仓库", L"%USERPROFILE%\\.m2\\repository", L"Java Maven 本地仓库缓存" },
        { L"vcpkg下载缓存", L"%LOCALAPPDATA%\\vcpkg\\downloads", L"vcpkg 源码包下载缓存" },
    };

    for (const auto& def : devDefs) {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* check = new wxCheckBox(scrollWin, wxID_ANY, def.name);
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* sizeLabel = new wxStaticText(scrollWin, wxID_ANY, L"");
        sizeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
        sizeLabel->SetForegroundColour(colors.accent);
        sizeLabel->SetMinSize(wxSize(80, -1));
        itemSizer->Add(sizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        listSizer->Add(itemSizer, 0, wxLEFT | wxRIGHT | wxTOP, 12);

        auto* desc = new wxStaticText(scrollWin, wxID_ANY, def.desc);
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        listSizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_softwareCacheItems.push_back({check, def.name, def.path, sizeLabel, 0});
    }

    // 单项变更时反向同步全选状态（仅统计可用项）
    for (const auto& item : m_softwareCacheItems) {
        item.checkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
            UpdateSelectAllChecked(m_softwareCacheItems, m_softwareSelectAllCheck, true);
        });
    }

    listSizer->AddStretchSpacer();
    scrollWin->SetSizer(listSizer);
    scrollWin->FitInside();

    sizer->Add(scrollWin, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(8);

    parent->SetSizer(sizer);
}

void DeepCleanPanel::OnSoftwareScan(wxCommandEvent& event) {
    m_softwareScanButton->Enable(false);
    m_softwareStatusLabel->SetLabelText(L"正在扫描...");

    // 重置大小标签
    for (auto& item : m_softwareCacheItems) {
        item.cacheSize = 0;
        item.sizeLabel->SetLabelText(L"");
    }

    std::thread([this]() {
        auto result = m_softwareCacheScanner.Scan();

        CallAfter([this, result = std::move(result)]() mutable {
            if (IsBeingDeleted()) return;

            m_softwareScanResult = std::move(result);

            // 按路径前缀统计每个软件的缓存大小
            for (const auto& scanItem : m_softwareScanResult.items) {
                const auto& path = scanItem.path;
                for (auto& cacheItem : m_softwareCacheItems) {
                    auto expandedPath = Utils::Win32Util::ExpandEnvVars(cacheItem.cachePath);
                    // 检查扫描到的文件是否属于该软件的缓存路径
                    if (path.find(expandedPath) == 0) {
                        cacheItem.cacheSize += scanItem.size;
                    }
                }
            }

            // 更新UI
            uint64_t totalSize = 0;
            int foundCount = 0;
            for (auto& item : m_softwareCacheItems) {
                if (item.cacheSize > 0) {
                    item.sizeLabel->SetLabelText(
                        wxString::Format(L"%s", Utils::FormatUtil::FormatFileSize(item.cacheSize)));
                    item.checkbox->SetValue(true);
                    totalSize += item.cacheSize;
                    foundCount++;
                } else {
                    item.sizeLabel->SetLabelText(L"0 B");
                    item.checkbox->SetValue(false);
                    item.checkbox->Enable(false);
                }
            }

            m_softwareStatusLabel->SetLabelText(
                wxString::Format(L"共发现 %d 个软件缓存，总计 %s",
                    foundCount, Utils::FormatUtil::FormatFileSize(totalSize)));
            m_softwareScanButton->Enable(true);
            m_softwareCleanButton->Enable(foundCount > 0);
            m_softwareSelectAllCheck->Enable(foundCount > 0);

            // 刷新全选状态
            UpdateSelectAllChecked(m_softwareCacheItems, m_softwareSelectAllCheck, true);
        });
    }).detach();
}

void DeepCleanPanel::OnSoftwareClean(wxCommandEvent& event) {
    // 收集选中的缓存项
    std::vector<std::wstring> selectedPaths;
    uint64_t totalSize = 0;
    for (const auto& item : m_softwareCacheItems) {
        if (item.checkbox->IsEnabled() && item.checkbox->GetValue()) {
            selectedPaths.push_back(Utils::Win32Util::ExpandEnvVars(item.cachePath));
            totalSize += item.cacheSize;
        }
    }

    if (selectedPaths.empty()) {
        wxMessageBox(L"请至少勾选一项要清理的软件缓存。", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    ConfirmDialog dlg(this, L"确认软件缓存清理",
        wxString::Format(L"即将清理 %d 个软件的缓存文件，预计释放 %s。\n\n"
                          L"清理后部分软件可能需要重新加载数据，确认继续？",
                          static_cast<int>(selectedPaths.size()),
                          Utils::FormatUtil::FormatFileSize(totalSize)),
        ConfirmDialog::DangerLevel::Caution, L"确认清理", L"取消");

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    m_softwareCleanButton->Enable(false);
    m_softwareStatusLabel->SetLabelText(L"正在清理...");

    std::thread([this, selectedPaths = std::move(selectedPaths)]() mutable {
        int cleanedCount = 0;
        uint64_t cleanedSize = 0;

        for (const auto& path : selectedPaths) {
            if (IceClean::Utils::FileUtil::Exists(path)) {
                // 递归删除目录内容（保留目录本身）
                WIN32_FIND_DATAW findData;
                std::wstring searchPath = path + L"\\*";
                HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        std::wstring itemName(findData.cFileName);
                        if (itemName == L"." || itemName == L"..") continue;

                        std::wstring itemPath = path + L"\\" + itemName;
                        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            // 删除子目录
                            IceClean::Utils::FileUtil::DeleteFolder(itemPath);
                        } else {
                            // 删除文件
                            IceClean::Utils::FileUtil::DeleteFilePermanently(itemPath);
                        }
                    } while (FindNextFileW(hFind, &findData));
                    FindClose(hFind);
                }
                cleanedCount++;
            }
        }

        CallAfter([this, cleanedCount, cleanedSize]() {
            if (IsBeingDeleted()) return;

            m_softwareStatusLabel->SetLabelText(
                wxString::Format(L"清理完成，已处理 %d 个软件缓存", cleanedCount));
            m_softwareCleanButton->Enable(false);

            // 重新扫描以更新大小
            wxCommandEvent scanEvt(wxEVT_BUTTON, m_softwareScanButton->GetId());
            m_softwareScanButton->GetEventHandler()->AddPendingEvent(scanEvt);
        });
    }).detach();
}

} // namespace IceClean::Gui
