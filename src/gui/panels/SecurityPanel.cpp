#include "SecurityPanel.h"
#include "core/safety/PopupBlocker.h"
#include "core/safety/BrowserProtector.h"
#include "core/safety/FileWatcher.h"
#include "core/safety/StartupProtector.h"
#include "core/safety/MalwareDetector.h"
#include "gui/controls/ThemeManager.h"
#include "utils/FormatUtil.h"

#include <thread>
#include <wx/dirdlg.h>

namespace IceClean::Gui {

// ── Event table ──

wxBEGIN_EVENT_TABLE(SecurityPanel, wxPanel)
    EVT_BUTTON(ID_REFRESH_POPUP, SecurityPanel::OnRefreshPopup)
    EVT_BUTTON(ID_BLOCK, SecurityPanel::OnBlock)
    EVT_BUTTON(ID_UNBLOCK, SecurityPanel::OnUnblock)
    EVT_BUTTON(ID_BLOCK_ALL, SecurityPanel::OnBlockAll)
    EVT_CHECKBOX(ID_ENABLE_POPUP, SecurityPanel::OnEnablePopupToggle)
    EVT_LIST_ITEM_SELECTED(ID_POPUP_LIST, SecurityPanel::OnPopupItemSelected)
    EVT_LIST_ITEM_ACTIVATED(ID_POPUP_LIST, SecurityPanel::OnPopupItemActivated)

    EVT_BUTTON(ID_SCAN_BROWSER, SecurityPanel::OnScanBrowser)
    EVT_BUTTON(ID_LOCK_HOME, SecurityPanel::OnLockHomePage)
    EVT_BUTTON(ID_RESTORE_HOME, SecurityPanel::OnRestoreHomePage)
    EVT_CHECKBOX(ID_LOCK_HOME_CHECK, SecurityPanel::OnLockHomeCheckToggle)
    EVT_CHECKBOX(ID_LOCK_SEARCH_CHECK, SecurityPanel::OnLockSearchCheckToggle)

    EVT_BUTTON(ID_START_WATCH, SecurityPanel::OnStartWatch)
    EVT_BUTTON(ID_STOP_WATCH, SecurityPanel::OnStopWatch)
    EVT_BUTTON(ID_ADD_WATCH_PATH, SecurityPanel::OnAddWatchPath)
    EVT_BUTTON(ID_REMOVE_WATCH_PATH, SecurityPanel::OnRemoveWatchPath)

    EVT_BUTTON(ID_BUILD_BASELINE, SecurityPanel::OnBuildBaseline)
    EVT_BUTTON(ID_DETECT_STARTUP_CHANGE, SecurityPanel::OnDetectStartupChange)
    EVT_CHECKBOX(ID_LOCK_STARTUP, SecurityPanel::OnLockStartupToggle)

    EVT_BUTTON(ID_FULL_SCAN, SecurityPanel::OnFullScan)
wxEND_EVENT_TABLE()

// ── Constructor ──

SecurityPanel::SecurityPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.background);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(16);

    // 标题
    auto* titleText = new wxStaticText(this, wxID_ANY, L"安全防护");
    titleText->SetFont(wxFont(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    titleText->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleText, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // 标签页
    m_notebook = new wxNotebook(this, wxID_ANY);
    m_notebook->SetBackgroundColour(colors.surface);

    CreatePopupPage();
    CreateBrowserPage();
    CreateFileWatchPage();
    CreateStartupProtectPage();
    CreateMalwareScanPage();

    m_notebook->AddPage(m_popupPage, L"弹窗拦截", true);
    m_notebook->AddPage(m_browserPage, L"浏览器保护", false);
    m_notebook->AddPage(m_fileWatchPage, L"文件监控", false);
    m_notebook->AddPage(m_startupProtectPage, L"启动保护", false);
    m_notebook->AddPage(m_malwareScanPage, L"恶意软件检测", false);

    mainSizer->Add(m_notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, 16);
    mainSizer->AddSpacer(16);

    SetSizer(mainSizer);

    // 延迟加载数据
    CallAfter([this]() { RefreshData(); });
}

SecurityPanel::~SecurityPanel() {
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
}

// ═══════════════════════════════════════════
// 弹窗拦截页面
// ═══════════════════════════════════════════

void SecurityPanel::CreatePopupPage() {
    m_popupPage = new wxPanel(m_notebook);
    m_popupPage->SetBackgroundColour(ThemeManager::Instance().GetColors().background);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 统计卡片
    auto* statsSizer = new wxBoxSizer(wxHORIZONTAL);

    // 今日拦截
    {
        auto* panel = new wxPanel(m_popupPage, wxID_ANY);
        panel->SetBackgroundColour(ThemeManager::Instance().GetColors().surface);
        panel->SetMinSize(wxSize(200, 75));

        auto* s = new wxBoxSizer(wxVERTICAL);
        s->AddSpacer(8);
        auto* label = new wxStaticText(panel, wxID_ANY, L"今日拦截");
        label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        label->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
        s->Add(label, 0, wxLEFT, 12);

        m_todayBlockedLabel = new wxStaticText(panel, wxID_ANY, L"0");
        m_todayBlockedLabel->SetFont(wxFont(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        m_todayBlockedLabel->SetForegroundColour(ThemeManager::Instance().GetColors().accent);
        s->Add(m_todayBlockedLabel, 0, wxLEFT, 12);
        panel->SetSizer(s);

        statsSizer->Add(panel, 1, wxEXPAND | wxALL, 3);
    }

    // 累计拦截
    {
        auto* panel = new wxPanel(m_popupPage, wxID_ANY);
        panel->SetBackgroundColour(ThemeManager::Instance().GetColors().surface);
        panel->SetMinSize(wxSize(200, 75));

        auto* s = new wxBoxSizer(wxVERTICAL);
        s->AddSpacer(8);
        auto* label = new wxStaticText(panel, wxID_ANY, L"累计拦截");
        label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        label->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
        s->Add(label, 0, wxLEFT, 12);

        m_totalBlockedLabel = new wxStaticText(panel, wxID_ANY, L"0");
        m_totalBlockedLabel->SetFont(wxFont(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        m_totalBlockedLabel->SetForegroundColour(ThemeManager::Instance().GetColors().success);
        s->Add(m_totalBlockedLabel, 0, wxLEFT, 12);
        panel->SetSizer(s);

        statsSizer->Add(panel, 1, wxEXPAND | wxALL, 3);
    }

    // 拦截规则
    {
        auto* panel = new wxPanel(m_popupPage, wxID_ANY);
        panel->SetBackgroundColour(ThemeManager::Instance().GetColors().surface);
        panel->SetMinSize(wxSize(200, 75));

        auto* s = new wxBoxSizer(wxVERTICAL);
        s->AddSpacer(8);
        auto* label = new wxStaticText(panel, wxID_ANY, L"拦截规则");
        label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        label->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
        s->Add(label, 0, wxLEFT, 12);

        m_rulesCountLabel = new wxStaticText(panel, wxID_ANY, L"0");
        m_rulesCountLabel->SetFont(wxFont(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        m_rulesCountLabel->SetForegroundColour(ThemeManager::Instance().GetColors().danger);
        s->Add(m_rulesCountLabel, 0, wxLEFT, 12);
        panel->SetSizer(s);

        statsSizer->Add(panel, 1, wxEXPAND | wxALL, 3);
    }

    sizer->Add(statsSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(8);

    // 启用开关
    m_enablePopupCheckBox = new wxCheckBox(m_popupPage, ID_ENABLE_POPUP, L"启用弹窗拦截");
    m_enablePopupCheckBox->SetValue(true);
    sizer->Add(m_enablePopupCheckBox, 0, wxLEFT | wxRIGHT, 8);
    sizer->AddSpacer(8);

    // 列表
    m_popupListCtrl = new wxListCtrl(m_popupPage, ID_POPUP_LIST, wxDefaultPosition, wxDefaultSize,
                                      wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_popupListCtrl->AppendColumn(L"名称", wxLIST_FORMAT_LEFT, 160);
    m_popupListCtrl->AppendColumn(L"类型", wxLIST_FORMAT_LEFT, 90);
    m_popupListCtrl->AppendColumn(L"描述", wxLIST_FORMAT_LEFT, 260);
    m_popupListCtrl->AppendColumn(L"状态", wxLIST_FORMAT_CENTER, 70);
    // SetAlternateRowColour requires wxLC_VIRTUAL; set alternating colors manually after populating
    sizer->Add(m_popupListCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(8);

    // 按钮
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    m_refreshPopupButton = new wxButton(m_popupPage, ID_REFRESH_POPUP, L"刷新扫描");
    m_refreshPopupButton->SetBackgroundColour(ThemeManager::Instance().GetColors().accent);
    m_refreshPopupButton->SetForegroundColour(*wxWHITE);
    btnSizer->Add(m_refreshPopupButton, 0, wxALL, 3);

    m_blockButton = new wxButton(m_popupPage, ID_BLOCK, L"拦截选中");
    m_blockButton->SetBackgroundColour(ThemeManager::Instance().GetColors().danger);
    m_blockButton->SetForegroundColour(*wxWHITE);
    m_blockButton->Enable(false);
    btnSizer->Add(m_blockButton, 0, wxALL, 3);

    m_unblockButton = new wxButton(m_popupPage, ID_UNBLOCK, L"解除拦截");
    m_unblockButton->SetBackgroundColour(ThemeManager::Instance().GetColors().success);
    m_unblockButton->SetForegroundColour(*wxWHITE);
    m_unblockButton->Enable(false);
    btnSizer->Add(m_unblockButton, 0, wxALL, 3);

    m_blockAllButton = new wxButton(m_popupPage, ID_BLOCK_ALL, L"一键拦截全部广告弹窗");
    m_blockAllButton->SetBackgroundColour(ThemeManager::Instance().GetColors().warning);
    m_blockAllButton->SetForegroundColour(*wxWHITE);
    btnSizer->Add(m_blockAllButton, 0, wxALL, 3);

    m_popupStatusLabel = new wxStaticText(m_popupPage, wxID_ANY, L"就绪");
    m_popupStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
    btnSizer->Add(m_popupStatusLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

    sizer->Add(btnSizer, 0, wxLEFT | wxRIGHT, 4);

    m_popupPage->SetSizer(sizer);
}

// ═══════════════════════════════════════════
// 浏览器保护页面
// ═══════════════════════════════════════════

void SecurityPanel::CreateBrowserPage() {
    m_browserPage = new wxPanel(m_notebook);
    m_browserPage->SetBackgroundColour(ThemeManager::Instance().GetColors().background);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 保护开关
    auto* checkSizer = new wxBoxSizer(wxHORIZONTAL);

    m_lockHomePageCheckBox = new wxCheckBox(m_browserPage, ID_LOCK_HOME_CHECK, L"锁定主页防篡改");
    m_lockHomePageCheckBox->SetValue(false);
    checkSizer->Add(m_lockHomePageCheckBox, 0, wxALL, 4);

    m_lockSearchEngineCheckBox = new wxCheckBox(m_browserPage, ID_LOCK_SEARCH_CHECK, L"锁定搜索引擎");
    m_lockSearchEngineCheckBox->SetValue(false);
    checkSizer->Add(m_lockSearchEngineCheckBox, 0, wxALL, 4);

    sizer->Add(checkSizer, 0, wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(8);

    // 主页设置
    auto* homeSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* homeLabel = new wxStaticText(m_browserPage, wxID_ANY, L"锁定主页:");
    homeLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textPrimary);
    homeSizer->Add(homeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    m_homePageTextCtrl = new wxTextCtrl(m_browserPage, ID_HOME_PAGE_TEXT, L"about:blank",
                                         wxDefaultPosition, wxSize(300, -1));
    homeSizer->Add(m_homePageTextCtrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_lockHomePageButton = new wxButton(m_browserPage, ID_LOCK_HOME, L"应用锁定");
    m_lockHomePageButton->SetBackgroundColour(ThemeManager::Instance().GetColors().accent);
    m_lockHomePageButton->SetForegroundColour(*wxWHITE);
    homeSizer->Add(m_lockHomePageButton, 0, wxALL, 2);

    sizer->Add(homeSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(8);

    // 浏览器列表
    m_browserListCtrl = new wxListCtrl(m_browserPage, ID_BROWSER_LIST, wxDefaultPosition, wxDefaultSize,
                                        wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_browserListCtrl->AppendColumn(L"浏览器", wxLIST_FORMAT_LEFT, 130);
    m_browserListCtrl->AppendColumn(L"当前主页", wxLIST_FORMAT_LEFT, 200);
    m_browserListCtrl->AppendColumn(L"搜索引擎", wxLIST_FORMAT_LEFT, 100);
    m_browserListCtrl->AppendColumn(L"主页状态", wxLIST_FORMAT_CENTER, 80);
    m_browserListCtrl->AppendColumn(L"搜索状态", wxLIST_FORMAT_CENTER, 80);
    // SetAlternateRowColour requires wxLC_VIRTUAL; set alternating colors manually after populating
    sizer->Add(m_browserListCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(8);

    // 按钮
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    m_scanBrowserButton = new wxButton(m_browserPage, ID_SCAN_BROWSER, L"扫描浏览器");
    m_scanBrowserButton->SetBackgroundColour(ThemeManager::Instance().GetColors().accent);
    m_scanBrowserButton->SetForegroundColour(*wxWHITE);
    btnSizer->Add(m_scanBrowserButton, 0, wxALL, 3);

    m_restoreHomePageButton = new wxButton(m_browserPage, ID_RESTORE_HOME, L"恢复默认主页");
    m_restoreHomePageButton->SetBackgroundColour(ThemeManager::Instance().GetColors().success);
    m_restoreHomePageButton->SetForegroundColour(*wxWHITE);
    btnSizer->Add(m_restoreHomePageButton, 0, wxALL, 3);

    m_browserStatusLabel = new wxStaticText(m_browserPage, wxID_ANY, L"就绪");
    m_browserStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
    btnSizer->Add(m_browserStatusLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

    sizer->Add(btnSizer, 0, wxLEFT | wxRIGHT, 4);

    m_browserPage->SetSizer(sizer);
}

// ═══════════════════════════════════════════
// 刷新数据
// ═══════════════════════════════════════════

void SecurityPanel::RefreshData() {
    m_popupStatusLabel->SetLabelText(L"正在扫描...");
    m_popupStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().accent);

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_workerThread = std::thread([this]() {
        IceClean::Core::Safety::PopupBlocker blocker;
        auto items = blocker.ScanPopupSources();
        auto stats = blocker.GetStats();

        IceClean::Core::Safety::BrowserProtector protector;
        auto browserItems = protector.ScanBrowsers();

        CallAfter([this, items = std::move(items), stats = std::move(stats),
                          browserItems = std::move(browserItems)]() mutable {
            if (IsBeingDeleted()) return;
            m_popupItems = std::move(items);
            m_popupStats = std::move(stats);
            m_browserItems = std::move(browserItems);
            LoadPopupItems();
            UpdatePopupStats();
            LoadBrowserItems();

            m_popupStatusLabel->SetLabelText(
                wxString::Format(L"扫描完成，发现 %d 个弹窗源", static_cast<int>(m_popupItems.size())));
            m_popupStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().success);

            m_browserStatusLabel->SetLabelText(
                wxString::Format(L"已扫描 %d 个浏览器", static_cast<int>(m_browserItems.size())));
        });
    });
}

// ── 加载弹窗项到列表 ──

void SecurityPanel::LoadPopupItems() {
    m_popupListCtrl->DeleteAllItems();

    for (int i = 0; i < static_cast<int>(m_popupItems.size()); ++i) {
        const auto& item = m_popupItems[i];
        long idx = m_popupListCtrl->InsertItem(i, item.name);
        m_popupListCtrl->SetItem(idx, 1, IceClean::Core::Safety::PopupBlocker::GetPopupTypeName(item.type));
        m_popupListCtrl->SetItem(idx, 2, item.description);
        m_popupListCtrl->SetItem(idx, 3, item.isBlocked ? L"已拦截" : (item.isSystem ? L"系统项" : L"未拦截"));

        wxColour itemColor = item.isBlocked ? ThemeManager::Instance().GetColors().success : (item.isSystem ? ThemeManager::Instance().GetColors().textSecondary : ThemeManager::Instance().GetColors().danger);
        m_popupListCtrl->SetItemTextColour(idx, itemColor);
    }
}

void SecurityPanel::UpdatePopupStats() {
    m_totalBlockedLabel->SetLabelText(wxString::Format(L"%d", m_popupStats.totalBlocked));
    m_todayBlockedLabel->SetLabelText(wxString::Format(L"%d", m_popupStats.todayBlocked));
    m_rulesCountLabel->SetLabelText(wxString::Format(L"%d", m_popupStats.rulesCount));
    m_enablePopupCheckBox->SetValue(m_popupStats.isEnabled);
}

// ── 加载浏览器项到列表 ──

void SecurityPanel::LoadBrowserItems() {
    m_browserListCtrl->DeleteAllItems();

    for (int i = 0; i < static_cast<int>(m_browserItems.size()); ++i) {
        const auto& item = m_browserItems[i];
        long idx = m_browserListCtrl->InsertItem(i, item.browserName);
        m_browserListCtrl->SetItem(idx, 1, item.homePage);
        m_browserListCtrl->SetItem(idx, 2, item.searchEngine);
        m_browserListCtrl->SetItem(idx, 3, item.isHomePageHijacked ? L"⚠ 已劫持" : L"正常");
        m_browserListCtrl->SetItem(idx, 4, item.isSearchEngineHijacked ? L"⚠ 已劫持" : L"正常");

        if (item.isHomePageHijacked || item.isSearchEngineHijacked) {
            m_browserListCtrl->SetItemTextColour(idx, ThemeManager::Instance().GetColors().danger);
        }
    }
}

// ═══════════════════════════════════════════
// 弹窗拦截事件
// ═══════════════════════════════════════════

void SecurityPanel::OnRefreshPopup(wxCommandEvent& event) {
    m_popupStatusLabel->SetLabelText(L"正在扫描...");
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_workerThread = std::thread([this]() {
        IceClean::Core::Safety::PopupBlocker blocker;
        auto items = blocker.ScanPopupSources();
        auto stats = blocker.GetStats();

        CallAfter([this, items = std::move(items), stats = std::move(stats)]() mutable {
            if (IsBeingDeleted()) return;
            m_popupItems = std::move(items);
            m_popupStats = std::move(stats);
            LoadPopupItems();
            UpdatePopupStats();
            m_popupStatusLabel->SetLabelText(L"扫描完成");
        });
    });
}

void SecurityPanel::OnBlock(wxCommandEvent& event) {
    int idx = GetSelectedPopupIndex();
    if (idx < 0 || idx >= static_cast<int>(m_popupItems.size())) return;

    auto& item = m_popupItems[idx];
    if (item.isSystem) {
        wxMessageBox(L"系统关键项不建议拦截。", L"警告", wxOK | wxICON_WARNING, this);
        return;
    }

    // 复制项数据到线程中，避免后台线程访问 m_popupItems
    auto itemCopy = item;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_workerThread = std::thread([this, itemCopy]() mutable {
        IceClean::Core::Safety::PopupBlocker blocker;
        bool success = blocker.BlockPopup(itemCopy);
        if (success) {
            blocker.IncrementBlockCount(itemCopy.name);
        }

        CallAfter([this, success, itemCopy]() {
            if (IsBeingDeleted()) return;
            if (success) {
                // 更新列表中对应项的状态
                for (auto& it : m_popupItems) {
                    if (it.name == itemCopy.name && it.sourcePath == itemCopy.sourcePath) {
                        it.isBlocked = true;
                        break;
                    }
                }
                LoadPopupItems();
                UpdatePopupStats();
                m_popupStatusLabel->SetLabelText(wxString::Format(L"已拦截: %s", itemCopy.name.c_str()));
            } else {
                wxMessageBox(L"拦截失败，可能需要管理员权限。", L"错误", wxOK | wxICON_ERROR, this);
            }
        });
    });
}

void SecurityPanel::OnUnblock(wxCommandEvent& event) {
    int idx = GetSelectedPopupIndex();
    if (idx < 0 || idx >= static_cast<int>(m_popupItems.size())) return;

    auto& item = m_popupItems[idx];
    IceClean::Core::Safety::PopupBlocker blocker;
    if (blocker.UnblockPopup(item)) {
        item.isBlocked = false;
        LoadPopupItems();
        m_popupStatusLabel->SetLabelText(wxString::Format(L"已解除: %s", item.name.c_str()));
    } else {
        wxMessageBox(L"解除拦截失败。", L"错误", wxOK | wxICON_ERROR, this);
    }
}

void SecurityPanel::OnBlockAll(wxCommandEvent& event) {
    // 收集需要拦截的项
    std::vector<IceClean::Models::PopupBlockerItem> toBlock;
    for (const auto& item : m_popupItems) {
        if (!item.isSystem && !item.isBlocked && item.type == Models::PopupType::AdwarePopup) {
            toBlock.push_back(item);
        }
    }

    if (toBlock.empty()) {
        m_popupStatusLabel->SetLabelText(L"没有需要拦截的广告弹窗");
        return;
    }

    m_popupStatusLabel->SetLabelText(L"正在拦截...");
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_workerThread = std::thread([this, toBlock = std::move(toBlock)]() mutable {
        IceClean::Core::Safety::PopupBlocker blocker;
        int blockedCount = 0;

        for (auto& item : toBlock) {
            if (blocker.BlockPopup(item)) {
                item.isBlocked = true;
                blocker.IncrementBlockCount(item.name);
                blockedCount++;
            }
        }

        CallAfter([this, toBlock = std::move(toBlock), blockedCount]() {
            if (IsBeingDeleted()) return;
            // 标记所有已拦截的项
            for (const auto& blocked : toBlock) {
                for (auto& item : m_popupItems) {
                    if (item.name == blocked.name && item.sourcePath == blocked.sourcePath) {
                        item.isBlocked = blocked.isBlocked;
                        break;
                    }
                }
            }
            LoadPopupItems();
            UpdatePopupStats();
            if (blockedCount > 0) {
                m_popupStatusLabel->SetLabelText(wxString::Format(L"已拦截 %d 个广告弹窗", blockedCount));
            } else {
                m_popupStatusLabel->SetLabelText(L"没有需要拦截的广告弹窗");
            }
        });
    });
}

void SecurityPanel::OnEnablePopupToggle(wxCommandEvent& event) {
    IceClean::Core::Safety::PopupBlocker::SetBlockerEnabled(m_enablePopupCheckBox->GetValue());
    m_popupStats.isEnabled = m_enablePopupCheckBox->GetValue();
    m_popupStatusLabel->SetLabelText(m_popupStats.isEnabled ? L"弹窗拦截已启用" : L"弹窗拦截已禁用");
}

void SecurityPanel::OnPopupItemSelected(wxListEvent& event) {
    int idx = GetSelectedPopupIndex();
    if (idx < 0 || idx >= static_cast<int>(m_popupItems.size())) {
        m_blockButton->Enable(false);
        m_unblockButton->Enable(false);
        return;
    }
    const auto& item = m_popupItems[idx];
    m_blockButton->Enable(!item.isBlocked && !item.isSystem);
    m_unblockButton->Enable(item.isBlocked);
}

void SecurityPanel::OnPopupItemActivated(wxListEvent& event) {
    int idx = event.GetIndex();
    if (idx < 0 || idx >= static_cast<int>(m_popupItems.size())) return;

    const auto& item = m_popupItems[idx];
    wxString info = wxString::Format(
        L"名称: %s\n类型: %s\n描述: %s\n来源: %s\n进程: %s\n状态: %s",
        item.name.c_str(),
        IceClean::Core::Safety::PopupBlocker::GetPopupTypeName(item.type).c_str(),
        item.description.c_str(), item.sourcePath.c_str(), item.processName.c_str(),
        item.isBlocked ? L"已拦截" : L"未拦截");
    wxMessageBox(info, L"弹窗详情", wxOK | wxICON_INFORMATION, this);
}

// ═══════════════════════════════════════════
// 浏览器保护事件
// ═══════════════════════════════════════════

void SecurityPanel::OnScanBrowser(wxCommandEvent& event) {
    m_browserStatusLabel->SetLabelText(L"正在扫描...");
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_workerThread = std::thread([this]() {
        IceClean::Core::Safety::BrowserProtector protector;
        auto items = protector.ScanBrowsers();

        CallAfter([this, items = std::move(items)]() mutable {
            if (IsBeingDeleted()) return;
            m_browserItems = std::move(items);
            LoadBrowserItems();
            m_browserStatusLabel->SetLabelText(
                wxString::Format(L"已扫描 %d 个浏览器", static_cast<int>(m_browserItems.size())));
        });
    });
}

void SecurityPanel::OnLockHomePage(wxCommandEvent& event) {
    int idx = GetSelectedBrowserIndex();
    if (idx < 0 || idx >= static_cast<int>(m_browserItems.size())) {
        wxMessageBox(L"请先选择要锁定的浏览器。", L"提示", wxOK | wxICON_INFORMATION, this);
        return;
    }

    auto homePage = m_homePageTextCtrl->GetValue().ToStdWstring();
    IceClean::Core::Safety::BrowserProtector protector;
    if (protector.LockHomePage(m_browserItems[idx], homePage)) {
        m_browserStatusLabel->SetLabelText(wxString::Format(L"已锁定 %s 主页", m_browserItems[idx].browserName.c_str()));
        OnScanBrowser(event);  // 刷新
    } else {
        wxMessageBox(L"锁定主页失败，请确保浏览器已关闭。", L"错误", wxOK | wxICON_ERROR, this);
    }
}

void SecurityPanel::OnRestoreHomePage(wxCommandEvent& event) {
    int idx = GetSelectedBrowserIndex();
    if (idx < 0 || idx >= static_cast<int>(m_browserItems.size())) {
        wxMessageBox(L"请先选择要恢复的浏览器。", L"提示", wxOK | wxICON_INFORMATION, this);
        return;
    }

    IceClean::Core::Safety::BrowserProtector protector;
    if (protector.RestoreDefaultHomePage(m_browserItems[idx])) {
        m_browserStatusLabel->SetLabelText(wxString::Format(L"已恢复 %s 默认主页", m_browserItems[idx].browserName.c_str()));
        OnScanBrowser(event);  // 刷新
    } else {
        wxMessageBox(L"恢复默认主页失败，请确保浏览器已关闭。", L"错误", wxOK | wxICON_ERROR, this);
    }
}

void SecurityPanel::OnLockHomeCheckToggle(wxCommandEvent& event) {
    auto config = IceClean::Core::Safety::BrowserProtector::GetConfig();
    config.enableHomePageLock = m_lockHomePageCheckBox->GetValue();
    config.lockedHomePage = m_homePageTextCtrl->GetValue().ToStdWstring();
    IceClean::Core::Safety::BrowserProtector::SaveConfig(config);
}

void SecurityPanel::OnLockSearchCheckToggle(wxCommandEvent& event) {
    auto config = IceClean::Core::Safety::BrowserProtector::GetConfig();
    config.enableSearchEngineLock = m_lockSearchEngineCheckBox->GetValue();
    IceClean::Core::Safety::BrowserProtector::SaveConfig(config);
}

// ── 辅助方法 ──

int SecurityPanel::GetSelectedPopupIndex() const {
    long idx = m_popupListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    return static_cast<int>(idx);
}

int SecurityPanel::GetSelectedBrowserIndex() const {
    long idx = m_browserListCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    return static_cast<int>(idx);
}

// ═══════════════════════════════════════════
// 文件监控页面
// ═══════════════════════════════════════════

void SecurityPanel::CreateFileWatchPage() {
    m_fileWatchPage = new wxPanel(m_notebook);
    m_fileWatchPage->SetBackgroundColour(ThemeManager::Instance().GetColors().background);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 统计
    auto* statsSizer = new wxBoxSizer(wxHORIZONTAL);

    {
        auto* panel = new wxPanel(m_fileWatchPage, wxID_ANY);
        panel->SetBackgroundColour(ThemeManager::Instance().GetColors().surface);
        panel->SetMinSize(wxSize(200, 70));
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->AddSpacer(8);
        auto* label = new wxStaticText(panel, wxID_ANY, L"今日变更");
        label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        label->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
        s->Add(label, 0, wxLEFT, 12);
        m_todayChangesLabel = new wxStaticText(panel, wxID_ANY, L"0");
        m_todayChangesLabel->SetFont(wxFont(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        m_todayChangesLabel->SetForegroundColour(ThemeManager::Instance().GetColors().accent);
        s->Add(m_todayChangesLabel, 0, wxLEFT, 12);
        panel->SetSizer(s);
        statsSizer->Add(panel, 1, wxEXPAND | wxALL, 3);
    }

    {
        auto* panel = new wxPanel(m_fileWatchPage, wxID_ANY);
        panel->SetBackgroundColour(ThemeManager::Instance().GetColors().surface);
        panel->SetMinSize(wxSize(200, 70));
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->AddSpacer(8);
        auto* label = new wxStaticText(panel, wxID_ANY, L"累计变更");
        label->SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        label->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
        s->Add(label, 0, wxLEFT, 12);
        m_totalChangesLabel = new wxStaticText(panel, wxID_ANY, L"0");
        m_totalChangesLabel->SetFont(wxFont(18, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        m_totalChangesLabel->SetForegroundColour(ThemeManager::Instance().GetColors().success);
        s->Add(m_totalChangesLabel, 0, wxLEFT, 12);
        panel->SetSizer(s);
        statsSizer->Add(panel, 1, wxEXPAND | wxALL, 3);
    }

    sizer->Add(statsSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(8);

    // 监控路径列表 + 按钮
    auto* pathSizer = new wxBoxSizer(wxHORIZONTAL);

    m_watchPathListBox = new wxListBox(m_fileWatchPage, ID_WATCH_PATH_LIST, wxDefaultPosition, wxSize(350, 100));
    pathSizer->Add(m_watchPathListBox, 1, wxEXPAND | wxALL, 3);

    auto* pathBtnSizer = new wxBoxSizer(wxVERTICAL);
    m_addWatchPathButton = new wxButton(m_fileWatchPage, ID_ADD_WATCH_PATH, L"添加路径");
    m_addWatchPathButton->SetBackgroundColour(ThemeManager::Instance().GetColors().accent);
    m_addWatchPathButton->SetForegroundColour(*wxWHITE);
    pathBtnSizer->Add(m_addWatchPathButton, 0, wxALL, 3);

    m_removeWatchPathButton = new wxButton(m_fileWatchPage, ID_REMOVE_WATCH_PATH, L"移除路径");
    m_removeWatchPathButton->SetBackgroundColour(ThemeManager::Instance().GetColors().danger);
    m_removeWatchPathButton->SetForegroundColour(*wxWHITE);
    pathBtnSizer->Add(m_removeWatchPathButton, 0, wxALL, 3);

    pathSizer->Add(pathBtnSizer, 0, wxEXPAND);
    sizer->Add(pathSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(8);

    // 开始/停止按钮
    auto* controlSizer = new wxBoxSizer(wxHORIZONTAL);

    m_startWatchButton = new wxButton(m_fileWatchPage, ID_START_WATCH, L"开始监控");
    m_startWatchButton->SetBackgroundColour(ThemeManager::Instance().GetColors().success);
    m_startWatchButton->SetForegroundColour(*wxWHITE);
    controlSizer->Add(m_startWatchButton, 0, wxALL, 3);

    m_stopWatchButton = new wxButton(m_fileWatchPage, ID_STOP_WATCH, L"停止监控");
    m_stopWatchButton->SetBackgroundColour(ThemeManager::Instance().GetColors().danger);
    m_stopWatchButton->SetForegroundColour(*wxWHITE);
    m_stopWatchButton->Enable(false);
    controlSizer->Add(m_stopWatchButton, 0, wxALL, 3);

    m_fileWatchStatusLabel = new wxStaticText(m_fileWatchPage, wxID_ANY, L"就绪");
    m_fileWatchStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
    controlSizer->Add(m_fileWatchStatusLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

    sizer->Add(controlSizer, 0, wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(8);

    // 变更记录列表
    m_fileWatchListCtrl = new wxListCtrl(m_fileWatchPage, ID_FILE_WATCH_LIST, wxDefaultPosition, wxDefaultSize,
                                          wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_fileWatchListCtrl->AppendColumn(L"时间", wxLIST_FORMAT_LEFT, 80);
    m_fileWatchListCtrl->AppendColumn(L"类型", wxLIST_FORMAT_LEFT, 60);
    m_fileWatchListCtrl->AppendColumn(L"路径", wxLIST_FORMAT_LEFT, 400);
    // SetAlternateRowColour requires wxLC_VIRTUAL; set alternating colors manually after populating
    sizer->Add(m_fileWatchListCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);

    m_fileWatchPage->SetSizer(sizer);

    // 默认添加 C:\ 和用户目录
    m_watchConfigs.push_back({L"C:\\Windows\\Temp", true, true, true, true, true, true});
    m_watchConfigs.push_back({L"C:\\Windows\\System32\\drivers\\etc", false, true, true, true, false, true});
    LoadWatchConfigs();
}

void SecurityPanel::LoadWatchConfigs() {
    m_watchPathListBox->Clear();
    for (const auto& config : m_watchConfigs) {
        m_watchPathListBox->Append(config.watchPath);
    }
}

void SecurityPanel::UpdateFileWatchStats() {
    if (m_fileWatcher) {
        auto stats = m_fileWatcher->GetStats();
        m_totalChangesLabel->SetLabelText(wxString::Format(L"%d", stats.totalChanges));
        m_todayChangesLabel->SetLabelText(wxString::Format(L"%d", stats.todayChanges));
    }
}

void SecurityPanel::OnStartWatch(wxCommandEvent& event) {
    if (m_watchConfigs.empty()) {
        wxMessageBox(L"请先添加监控路径。", L"提示", wxOK | wxICON_INFORMATION, this);
        return;
    }

    m_fileWatcher = std::make_unique<IceClean::Core::Safety::FileWatcher>();

    // 设置回调 - 通过 wxThreadEvent 发送到主线程
    m_fileWatcher->SetChangeCallback([this](const IceClean::Models::FileChangeRecord& record) {
        wxThreadEvent* evt = new wxThreadEvent(wxEVT_THREAD);
        evt->SetPayload(record);
        wxQueueEvent(this, evt);
    });

    // 绑定线程事件
    Bind(wxEVT_THREAD, &SecurityPanel::OnFileChangeReceived, this);

    if (m_fileWatcher->Start(m_watchConfigs)) {
        m_startWatchButton->Enable(false);
        m_stopWatchButton->Enable(true);
        m_fileWatchStatusLabel->SetLabelText(L"正在监控...");
        m_fileWatchStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().success);
    }
}

void SecurityPanel::OnStopWatch(wxCommandEvent& event) {
    if (m_fileWatcher) {
        m_fileWatcher->Stop();
        m_fileWatcher.reset();
    }

    m_startWatchButton->Enable(true);
    m_stopWatchButton->Enable(false);
    m_fileWatchStatusLabel->SetLabelText(L"监控已停止");
    m_fileWatchStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
}

void SecurityPanel::OnAddWatchPath(wxCommandEvent& event) {
    wxDirDialog dlg(this, L"选择要监控的目录", L"",
                     wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() == wxID_OK) {
        auto path = dlg.GetPath().ToStdWstring();
        // 检查是否已存在
        for (const auto& config : m_watchConfigs) {
            if (config.watchPath == path) {
                wxMessageBox(L"该路径已在监控列表中。", L"提示", wxOK | wxICON_INFORMATION, this);
                return;
            }
        }
        m_watchConfigs.push_back({path, true, true, true, true, true, true});
        LoadWatchConfigs();
    }
}

void SecurityPanel::OnRemoveWatchPath(wxCommandEvent& event) {
    int sel = m_watchPathListBox->GetSelection();
    if (sel >= 0 && sel < static_cast<int>(m_watchConfigs.size())) {
        m_watchConfigs.erase(m_watchConfigs.begin() + sel);
        LoadWatchConfigs();
    }
}

void SecurityPanel::OnFileChangeReceived(wxThreadEvent& event) {
    auto record = event.GetPayload<IceClean::Models::FileChangeRecord>();

    // 添加到列表顶部
    long idx = m_fileWatchListCtrl->InsertItem(0, L"now");
    m_fileWatchListCtrl->SetItem(idx, 1, IceClean::Core::Safety::FileWatcher::GetChangeTypeName(record.changeType));
    m_fileWatchListCtrl->SetItem(idx, 2, record.filePath);

    // 根据变更类型设置颜色
    wxColour rowColor;
    switch (record.changeType) {
    case Models::FileChangeType::Added:     rowColor = ThemeManager::Instance().GetColors().success; break;
    case Models::FileChangeType::Removed:   rowColor = ThemeManager::Instance().GetColors().danger; break;
    case Models::FileChangeType::Modified:  rowColor = ThemeManager::Instance().GetColors().warning; break;
    default:                                 rowColor = ThemeManager::Instance().GetColors().textPrimary; break;
    }
    m_fileWatchListCtrl->SetItemTextColour(idx, rowColor);

    // 限制显示条目数
    while (m_fileWatchListCtrl->GetItemCount() > 500) {
        m_fileWatchListCtrl->DeleteItem(m_fileWatchListCtrl->GetItemCount() - 1);
    }

    UpdateFileWatchStats();
}

// ═══════════════════════════════════════════
// 启动保护页面
// ═══════════════════════════════════════════

void SecurityPanel::CreateStartupProtectPage() {
    m_startupProtectPage = new wxPanel(m_notebook);
    m_startupProtectPage->SetBackgroundColour(ThemeManager::Instance().GetColors().background);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 锁定开关
    m_lockStartupCheckBox = new wxCheckBox(m_startupProtectPage, ID_LOCK_STARTUP,
                                             L"锁定启动项（阻止新增启动项）");
    m_lockStartupCheckBox->SetValue(IceClean::Core::Safety::StartupProtector::IsStartupLocked());
    sizer->Add(m_lockStartupCheckBox, 0, wxLEFT | wxRIGHT, 8);
    sizer->AddSpacer(8);

    // 按钮
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    m_buildBaselineButton = new wxButton(m_startupProtectPage, ID_BUILD_BASELINE, L"建立基线");
    m_buildBaselineButton->SetBackgroundColour(ThemeManager::Instance().GetColors().accent);
    m_buildBaselineButton->SetForegroundColour(*wxWHITE);
    btnSizer->Add(m_buildBaselineButton, 0, wxALL, 3);

    m_detectStartupChangeButton = new wxButton(m_startupProtectPage, ID_DETECT_STARTUP_CHANGE, L"检测变更");
    m_detectStartupChangeButton->SetBackgroundColour(ThemeManager::Instance().GetColors().danger);
    m_detectStartupChangeButton->SetForegroundColour(*wxWHITE);
    btnSizer->Add(m_detectStartupChangeButton, 0, wxALL, 3);

    m_startupProtectStatusLabel = new wxStaticText(m_startupProtectPage, wxID_ANY, L"就绪");
    m_startupProtectStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
    btnSizer->Add(m_startupProtectStatusLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

    sizer->Add(btnSizer, 0, wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(8);

    // 变更列表
    m_startupChangeListCtrl = new wxListCtrl(m_startupProtectPage, ID_STARTUP_CHANGE_LIST,
        wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_startupChangeListCtrl->AppendColumn(L"名称", wxLIST_FORMAT_LEFT, 150);
    m_startupChangeListCtrl->AppendColumn(L"变更类型", wxLIST_FORMAT_LEFT, 80);
    m_startupChangeListCtrl->AppendColumn(L"路径", wxLIST_FORMAT_LEFT, 350);
    // SetAlternateRowColour requires wxLC_VIRTUAL; set alternating colors manually after populating
    sizer->Add(m_startupChangeListCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);

    m_startupProtectPage->SetSizer(sizer);
}

void SecurityPanel::OnBuildBaseline(wxCommandEvent& event) {
    m_startupProtectStatusLabel->SetLabelText(L"正在建立基线...");
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_workerThread = std::thread([this]() {
        IceClean::Core::Safety::StartupProtector protector;
        protector.BuildBaseline();

        CallAfter([this]() {
            if (IsBeingDeleted()) return;
            m_startupProtectStatusLabel->SetLabelText(L"基线已建立");
            m_startupProtectStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().success);
        });
    });
}

void SecurityPanel::OnDetectStartupChange(wxCommandEvent& event) {
    m_startupProtectStatusLabel->SetLabelText(L"正在检测变更...");
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_workerThread = std::thread([this]() {
        IceClean::Core::Safety::StartupProtector protector;
        auto changes = protector.DetectChanges();

        CallAfter([this, changes = std::move(changes)]() mutable {
            if (IsBeingDeleted()) return;
            m_startupChangeListCtrl->DeleteAllItems();

            for (int i = 0; i < static_cast<int>(changes.size()); ++i) {
                const auto& change = changes[i];
                long idx = m_startupChangeListCtrl->InsertItem(i, change.itemName);
                m_startupChangeListCtrl->SetItem(idx, 1,
                    IceClean::Core::Safety::StartupProtector::GetChangeTypeName(change.changeType));
                m_startupChangeListCtrl->SetItem(idx, 2, change.itemPath);

                wxColour color = (change.changeType == IceClean::Core::Safety::StartupChangeRecord::ChangeType::Added)
                    ? ThemeManager::Instance().GetColors().danger : ThemeManager::Instance().GetColors().textPrimary;
                m_startupChangeListCtrl->SetItemTextColour(idx, color);
            }

            m_startupProtectStatusLabel->SetLabelText(
                wxString::Format(L"检测完成，发现 %d 个变更", static_cast<int>(changes.size())));
            m_startupProtectStatusLabel->SetForegroundColour(
                changes.empty() ? ThemeManager::Instance().GetColors().success : ThemeManager::Instance().GetColors().danger);
        });
    });
}

void SecurityPanel::OnLockStartupToggle(wxCommandEvent& event) {
    IceClean::Core::Safety::StartupProtector::SetStartupLocked(m_lockStartupCheckBox->GetValue());
    m_startupProtectStatusLabel->SetLabelText(
        m_lockStartupCheckBox->GetValue() ? L"启动项已锁定" : L"启动项已解锁");
}

// ═══════════════════════════════════════════
// 恶意软件检测页面
// ═══════════════════════════════════════════

void SecurityPanel::CreateMalwareScanPage() {
    m_malwareScanPage = new wxPanel(m_notebook);
    m_malwareScanPage->SetBackgroundColour(ThemeManager::Instance().GetColors().background);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 扫描按钮
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    m_fullScanButton = new wxButton(m_malwareScanPage, ID_FULL_SCAN, L"全面扫描");
    m_fullScanButton->SetBackgroundColour(ThemeManager::Instance().GetColors().accent);
    m_fullScanButton->SetForegroundColour(*wxWHITE);
    btnSizer->Add(m_fullScanButton, 0, wxALL, 3);

    m_malwareScanStatusLabel = new wxStaticText(m_malwareScanPage, wxID_ANY, L"就绪");
    m_malwareScanStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
    btnSizer->Add(m_malwareScanStatusLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

    sizer->Add(btnSizer, 0, wxLEFT | wxRIGHT, 4);
    sizer->AddSpacer(4);

    // 进度条
    m_scanProgressGauge = new wxGauge(m_malwareScanPage, wxID_ANY, 100);
    m_scanProgressGauge->SetValue(0);
    sizer->Add(m_scanProgressGauge, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
    sizer->AddSpacer(8);

    // 结果列表
    m_malwareListCtrl = new wxListCtrl(m_malwareScanPage, ID_MALWARE_LIST,
        wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_malwareListCtrl->AppendColumn(L"检测项", wxLIST_FORMAT_LEFT, 180);
    m_malwareListCtrl->AppendColumn(L"风险等级", wxLIST_FORMAT_CENTER, 80);
    m_malwareListCtrl->AppendColumn(L"描述", wxLIST_FORMAT_LEFT, 220);
    m_malwareListCtrl->AppendColumn(L"建议", wxLIST_FORMAT_LEFT, 150);
    // SetAlternateRowColour requires wxLC_VIRTUAL; set alternating colors manually after populating
    sizer->Add(m_malwareListCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT, 4);

    m_malwareScanPage->SetSizer(sizer);
}

void SecurityPanel::OnFullScan(wxCommandEvent& event) {
    m_fullScanButton->Enable(false);
    m_malwareScanStatusLabel->SetLabelText(L"正在扫描...");
    m_scanProgressGauge->SetValue(10);

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_workerThread = std::thread([this]() {
        IceClean::Core::Safety::MalwareDetector detector;
        auto results = detector.PerformFullScan();

        CallAfter([this, results = std::move(results)]() mutable {
            if (IsBeingDeleted()) return;
            m_malwareListCtrl->DeleteAllItems();

            for (int i = 0; i < static_cast<int>(results.size()); ++i) {
                const auto& r = results[i];
                long idx = m_malwareListCtrl->InsertItem(i, r.name);
                m_malwareListCtrl->SetItem(idx, 1,
                    IceClean::Core::Safety::MalwareDetector::GetSeverityName(r.severity));
                m_malwareListCtrl->SetItem(idx, 2, r.description);
                m_malwareListCtrl->SetItem(idx, 3, r.suggestedAction);

                // 颜色
                wxColour color;
                switch (r.severity) {
                case IceClean::Core::Safety::MalwareDetectionResult::Severity::Critical:
                    color = ThemeManager::Instance().GetColors().danger; break;
                case IceClean::Core::Safety::MalwareDetectionResult::Severity::High:
                    color = ThemeManager::Instance().GetColors().warning; break;
                case IceClean::Core::Safety::MalwareDetectionResult::Severity::Medium:
                    color = ThemeManager::Instance().GetColors().warning; break;
                default:
                    color = ThemeManager::Instance().GetColors().textPrimary; break;
                }
                m_malwareListCtrl->SetItemTextColour(idx, color);
            }

            m_scanProgressGauge->SetValue(100);
            m_fullScanButton->Enable(true);

            if (results.empty()) {
                m_malwareScanStatusLabel->SetLabelText(L"扫描完成，未发现安全威胁");
                m_malwareScanStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().success);
            } else {
                m_malwareScanStatusLabel->SetLabelText(
                    wxString::Format(L"扫描完成，发现 %d 个潜在威胁", static_cast<int>(results.size())));
                m_malwareScanStatusLabel->SetForegroundColour(ThemeManager::Instance().GetColors().danger);
            }
        });
    });
}

} // namespace IceClean::Gui
