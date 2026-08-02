#include "DashboardPanel.h"
#include "gui/Events.h"
#include "gui/controls/NavSidebar.h"
#include "gui/controls/ThemeManager.h"
#include "core/safety/UsageStats.h"
#include "utils/FormatUtil.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <shlobj.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(DashboardPanel, wxPanel)
wxEND_EVENT_TABLE()

DashboardPanel::DashboardPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();

    // C盘空间监控定时器（每60秒检查一次）
    m_diskTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &DashboardPanel::OnDiskTimer, this, m_diskTimer->GetId());
    m_diskTimer->Start(60000);
}

void DashboardPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(20);

    // ── C盘健康状态区域 ──
    auto* healthCard = new CardPanel(this, wxID_ANY, L"C盘健康状态");
    auto* healthSizer = healthCard->GetContentSizer();

    // 圆形进度 + 磁盘信息
    auto* healthContentSizer = new wxBoxSizer(wxHORIZONTAL);

    // 圆形进度条
    m_progressCtrl = new CircularProgress(healthCard, wxID_ANY, wxDefaultPosition, wxSize(160, 160));
    m_progressCtrl->SetValue(0);
    m_progressCtrl->SetLabel(L"0%");
    m_progressCtrl->SetSubLabel(L"已使用");
    healthContentSizer->Add(m_progressCtrl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 24);

    // 磁盘信息文字
    auto* infoSizer = new wxBoxSizer(wxVERTICAL);
    infoSizer->AddSpacer(10);

    auto* titleLabel = new wxStaticText(healthCard, wxID_ANY, L"本地磁盘 (C:)");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    infoSizer->Add(titleLabel, 0, wxBOTTOM, 8);

    m_diskInfoLabel = new wxStaticText(healthCard, wxID_ANY, L"0 GB / 0 GB");
    m_diskInfoLabel->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
    m_diskInfoLabel->SetForegroundColour(colors.textSecondary);
    infoSizer->Add(m_diskInfoLabel, 0, wxBOTTOM, 8);

    // 健康评分
    auto* scoreSizer = new wxBoxSizer(wxHORIZONTAL);
    m_healthScoreLabel = new wxStaticText(healthCard, wxID_ANY, L"健康评分: --");
    m_healthScoreLabel->SetFont(wxFont(13, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                       false, L"微软雅黑"));
    m_healthScoreLabel->SetForegroundColour(colors.success);
    scoreSizer->Add(m_healthScoreLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_healthDescLabel = new wxStaticText(healthCard, wxID_ANY, L"扫描后获取评分");
    m_healthDescLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
    m_healthDescLabel->SetForegroundColour(colors.textDisabled);
    scoreSizer->Add(m_healthDescLabel, 0, wxALIGN_CENTER_VERTICAL);
    infoSizer->Add(scoreSizer, 0, wxBOTTOM, 4);

    // 累计清理统计
    m_cumulativeLabel = new wxStaticText(healthCard, wxID_ANY, L"累计清理: 0 B");
    m_cumulativeLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
    m_cumulativeLabel->SetForegroundColour(colors.accent);
    infoSizer->Add(m_cumulativeLabel, 0, wxBOTTOM, 4);

    // C盘空间预警
    m_spaceWarningLabel = new wxStaticText(healthCard, wxID_ANY, L"");
    m_spaceWarningLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                        false, L"微软雅黑"));
    m_spaceWarningLabel->Hide();
    infoSizer->Add(m_spaceWarningLabel, 0, wxBOTTOM, 4);

    // 加载累计统计
    LoadCumulativeStats();

    healthContentSizer->Add(infoSizer, 1, wxALIGN_CENTER_VERTICAL);
    healthSizer->Add(healthContentSizer, 0, wxEXPAND);

    mainSizer->Add(healthCard, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(16);

    // ── 一键扫描/清理/停止按钮 ──
    auto* scanSizer = new wxBoxSizer(wxHORIZONTAL);
    scanSizer->AddStretchSpacer();

    m_scanButton = new wxButton(this, wxID_ANY, L"一键扫描", wxDefaultPosition, wxSize(200, 48));
    m_scanButton->SetName("btn_primary_scan");
    m_scanButton->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    m_scanButton->SetBackgroundColour(colors.accent);
    m_scanButton->SetForegroundColour(*wxWHITE);
    m_scanButton->SetCursor(wxCURSOR_HAND);
    m_scanButton->SetToolTip(L"扫描C盘中的垃圾文件、缓存和临时文件");
    m_scanButton->Bind(wxEVT_BUTTON, &DashboardPanel::OnScanButton, this);
    scanSizer->Add(m_scanButton, 0);

    m_cleanButton = new wxButton(this, wxID_ANY, L"一键清理", wxDefaultPosition, wxSize(200, 48));
    m_cleanButton->SetName("btn_success_clean");
    m_cleanButton->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                   false, L"微软雅黑"));
    m_cleanButton->SetBackgroundColour(colors.success);
    m_cleanButton->SetForegroundColour(*wxWHITE);
    m_cleanButton->SetCursor(wxCURSOR_HAND);
    m_cleanButton->SetToolTip(L"清理选中的垃圾文件，释放磁盘空间");
    m_cleanButton->Bind(wxEVT_BUTTON, &DashboardPanel::OnCleanButton, this);
    m_cleanButton->Hide();
    scanSizer->Add(m_cleanButton, 0);

    m_stopButton = new wxButton(this, wxID_ANY, L"停止扫描", wxDefaultPosition, wxSize(120, 48));
    m_stopButton->SetName("btn_danger_stop");
    m_stopButton->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
    m_stopButton->SetBackgroundColour(colors.danger);
    m_stopButton->SetForegroundColour(*wxWHITE);
    m_stopButton->SetCursor(wxCURSOR_HAND);
    m_stopButton->SetToolTip(L"停止当前扫描（再次点击强制停止）");
    m_stopButton->Bind(wxEVT_BUTTON, &DashboardPanel::OnStopButton, this);
    m_stopButton->Hide();
    scanSizer->Add(m_stopButton, 0, wxLEFT, 10);

    scanSizer->AddStretchSpacer();
    mainSizer->Add(scanSizer, 0, wxEXPAND);
    mainSizer->AddSpacer(16);

    // ── 扫描结果区域（初始隐藏）──
    CreateResultSection(mainSizer);

    // ── 快捷入口卡片 ──
    CreateQuickAccessCards(mainSizer);
    mainSizer->AddSpacer(20);

    SetSizer(mainSizer);
}

void DashboardPanel::CreateQuickAccessCards(wxSizer* parentSizer) {
    const auto& colors = ThemeManager::Instance().GetColors();
    m_quickAccessSizer = new wxBoxSizer(wxVERTICAL);

    auto* gridSizer = new wxGridSizer(3, 2, 10, 10);

    // 临时文件卡片
    m_cardTemp = new CardPanel(this, wxID_ANY, L"临时文件");
    m_cardTemp->SetClickable(true);
    m_cardTemp->SetMinSize(wxSize(200, 80));
    m_cardTemp->SetToolTip(L"清理系统临时文件夹和用户临时文件");
    auto* tempLabel = new wxStaticText(m_cardTemp, wxID_ANY, L"清理系统临时文件\n释放磁盘空间");
    tempLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    tempLabel->SetForegroundColour(colors.textSecondary);
    m_cardTemp->GetContentSizer()->Add(tempLabel, 1, wxEXPAND);
    m_cardTemp->Bind(wxEVT_BUTTON, &DashboardPanel::OnQuickAccessCard, this);
    gridSizer->Add(m_cardTemp, 1, wxEXPAND);

    // 更新缓存卡片
    m_cardUpdate = new CardPanel(this, wxID_ANY, L"更新缓存");
    m_cardUpdate->SetClickable(true);
    m_cardUpdate->SetMinSize(wxSize(200, 80));
    m_cardUpdate->SetToolTip(L"清理Windows Update下载缓存和旧版本备份");
    auto* updateLabel = new wxStaticText(m_cardUpdate, wxID_ANY, L"清理Windows更新缓存\n释放大量空间");
    updateLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    updateLabel->SetForegroundColour(colors.textSecondary);
    m_cardUpdate->GetContentSizer()->Add(updateLabel, 1, wxEXPAND);
    m_cardUpdate->Bind(wxEVT_BUTTON, &DashboardPanel::OnQuickAccessCard, this);
    gridSizer->Add(m_cardUpdate, 1, wxEXPAND);

    // 浏览器缓存卡片
    m_cardBrowser = new CardPanel(this, wxID_ANY, L"浏览器缓存");
    m_cardBrowser->SetClickable(true);
    m_cardBrowser->SetMinSize(wxSize(200, 80));
    m_cardBrowser->SetToolTip(L"清理Chrome/Edge/Firefox等浏览器缓存和Cookie");
    auto* browserLabel = new wxStaticText(m_cardBrowser, wxID_ANY, L"清理浏览器缓存\n保护隐私安全");
    browserLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    browserLabel->SetForegroundColour(colors.textSecondary);
    m_cardBrowser->GetContentSizer()->Add(browserLabel, 1, wxEXPAND);
    m_cardBrowser->Bind(wxEVT_BUTTON, &DashboardPanel::OnQuickAccessCard, this);
    gridSizer->Add(m_cardBrowser, 1, wxEXPAND);

    // 休眠文件卡片
    m_cardHibernation = new CardPanel(this, wxID_ANY, L"休眠文件");
    m_cardHibernation->SetClickable(true);
    m_cardHibernation->SetMinSize(wxSize(200, 80));
    m_cardHibernation->SetToolTip(L"关闭休眠功能并删除hiberfil.sys，释放数GB空间");
    auto* hibernationLabel = new wxStaticText(m_cardHibernation, wxID_ANY, L"关闭休眠功能\n释放数GB空间");
    hibernationLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
    hibernationLabel->SetForegroundColour(colors.textSecondary);
    m_cardHibernation->GetContentSizer()->Add(hibernationLabel, 1, wxEXPAND);
    m_cardHibernation->Bind(wxEVT_BUTTON, &DashboardPanel::OnQuickAccessCard, this);
    gridSizer->Add(m_cardHibernation, 1, wxEXPAND);

    // 启动加速卡片
    m_cardStartup = new CardPanel(this, wxID_ANY, L"启动加速");
    m_cardStartup->SetClickable(true);
    m_cardStartup->SetMinSize(wxSize(200, 80));
    m_cardStartup->SetToolTip(L"管理开机启动项和服务，加速系统启动");
    auto* startupLabel = new wxStaticText(m_cardStartup, wxID_ANY, L"管理启动项和服务\n加速系统启动");
    startupLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                  false, L"微软雅黑"));
    startupLabel->SetForegroundColour(colors.textSecondary);
    m_cardStartup->GetContentSizer()->Add(startupLabel, 1, wxEXPAND);
    m_cardStartup->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxCommandEvent navEvt(wxEVT_NAV_SELECTION_CHANGED, GetId());
        navEvt.SetInt(3);  // 加速优化面板索引
        wxPostEvent(GetParent(), navEvt);
    });
    gridSizer->Add(m_cardStartup, 1, wxEXPAND);

    // 软件管理卡片
    m_cardSoftware = new CardPanel(this, wxID_ANY, L"软件管理");
    m_cardSoftware->SetClickable(true);
    m_cardSoftware->SetMinSize(wxSize(200, 80));
    m_cardSoftware->SetToolTip(L"卸载不需要的软件，释放磁盘空间");
    auto* softwareLabel = new wxStaticText(m_cardSoftware, wxID_ANY, L"卸载不需要的软件\n释放磁盘空间");
    softwareLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    softwareLabel->SetForegroundColour(colors.textSecondary);
    m_cardSoftware->GetContentSizer()->Add(softwareLabel, 1, wxEXPAND);
    m_cardSoftware->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        wxCommandEvent navEvt(wxEVT_NAV_SELECTION_CHANGED, GetId());
        navEvt.SetInt(4);  // 软件管理面板索引
        wxPostEvent(GetParent(), navEvt);
    });
    gridSizer->Add(m_cardSoftware, 1, wxEXPAND);

    // 将网格放入带边距的容器
    auto* cardContainer = new wxBoxSizer(wxHORIZONTAL);
    cardContainer->Add(gridSizer, 1, wxEXPAND);
    m_quickAccessSizer->Add(cardContainer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    parentSizer->Add(m_quickAccessSizer, 0, wxEXPAND);
}

void DashboardPanel::UpdateDiskInfo(uint64_t usedBytes, uint64_t totalBytes) {
    const auto& colors = ThemeManager::Instance().GetColors();
    m_usedBytes = usedBytes;
    m_totalBytes = totalBytes;

    int percent = 0;
    if (totalBytes > 0) {
        percent = static_cast<int>((usedBytes * 100) / totalBytes);
    }

    m_progressCtrl->SetValue(percent);
    m_progressCtrl->SetLabel(wxString::Format(L"%d%%", percent));

    using namespace IceClean::Utils;
    wxString info = wxString::Format(L"%s / %s",
        FormatUtil::FormatFileSize(usedBytes).c_str(),
        FormatUtil::FormatFileSize(totalBytes).c_str());
    m_diskInfoLabel->SetLabel(info);

    // 根据使用率调整颜色
    if (percent > 90) {
        m_progressCtrl->SetProgressColor(colors.danger); // 红色
    } else if (percent > 75) {
        m_progressCtrl->SetProgressColor(colors.warning); // 橙色
    } else {
        m_progressCtrl->SetProgressColor(colors.accent); // 蓝色
    }

    // C盘空间预警
    uint64_t freeBytes = totalBytes - usedBytes;
    double freePercent = (totalBytes > 0) ? (static_cast<double>(freeBytes) / totalBytes * 100.0) : 100.0;
    if (freePercent < 10.0) {
        m_spaceWarningLabel->SetLabel(L"C盘空间不足！");
        m_spaceWarningLabel->SetForegroundColour(colors.danger);
        m_spaceWarningLabel->Show();
    } else if (freePercent < 20.0) {
        m_spaceWarningLabel->SetLabel(L"C盘空间偏低");
        m_spaceWarningLabel->SetForegroundColour(colors.warning);
        m_spaceWarningLabel->Show();
    } else {
        m_spaceWarningLabel->Hide();
    }
}

void DashboardPanel::SetScanning(bool scanning) {
    const auto& colors = ThemeManager::Instance().GetColors();
    if (scanning) {
        m_scanButton->Hide();
        m_stopButton->Show();
        m_stopButton->Enable();
        m_stopButton->SetLabel(L"停止扫描");
        m_stopForceMode = false;
        // 切换 CircularProgress 为扫描进度模式（不确定旋转动画）
        m_progressCtrl->SetValue(0);
        m_progressCtrl->SetLabel(L"");
        m_progressCtrl->SetSubLabel(L"扫描中");
        m_progressCtrl->SetProgressColor(colors.accent);
        m_progressCtrl->SetIndeterminate(true);
        m_diskInfoLabel->SetLabel(L"准备扫描...");
        Layout();
    } else {
        m_scanButton->Show();
        m_stopButton->Hide();
        m_stopForceMode = false;
        m_progressCtrl->SetIndeterminate(false);
        Layout();
    }
}

void DashboardPanel::UpdateScanProgress(int completedScanners, int totalScanners,
                                         const std::wstring& currentScanner) {
    if (totalScanners <= 0) return;

    int percent = (completedScanners * 100) / totalScanners;

    // 第一次进度到来时关闭不确定模式
    if (m_progressCtrl->IsIndeterminate() && completedScanners > 0) {
        m_progressCtrl->SetIndeterminate(false);
    }

    m_progressCtrl->SetValue(percent);
    m_progressCtrl->SetLabel(wxString::Format(L"%d%%", percent));
    m_progressCtrl->SetSubLabel(L"扫描中");

    // 显示当前正在扫描的项目
    wxString scannerName(currentScanner);
    if (!scannerName.IsEmpty()) {
        m_diskInfoLabel->SetLabel(L"正在扫描: " + scannerName);
    }
}

void DashboardPanel::UpdateScanProgress(const IceClean::Core::Scanner::ScanProgressInfo& info) {
    if (info.totalScanners <= 0) return;

    int percent = (info.completedScanners * 100) / info.totalScanners;
    m_progressCtrl->SetValue(percent);
    m_progressCtrl->SetLabel(wxString::Format(L"%d%%", percent));
    m_progressCtrl->SetSubLabel(L"扫描中");

    // 显示当前正在扫描的项目和文件名
    wxString scannerName(info.currentScanner);
    if (!scannerName.IsEmpty()) {
        wxString fileDetail;
        if (!info.currentFile.empty()) {
            // 截取文件名部分（取最后两级路径）
            wxString filePath(info.currentFile);
            int pos1 = filePath.Find(L'\\', true);  // 从后找
            if (pos1 != wxNOT_FOUND) {
                wxString rest = filePath.Left(pos1);
                int pos2 = rest.Find(L'\\', true);
                if (pos2 != wxNOT_FOUND) {
                    fileDetail = filePath.Mid(pos2 + 1);
                } else {
                    fileDetail = filePath;
                }
            } else {
                fileDetail = filePath;
            }
        }

        if (!fileDetail.IsEmpty()) {
            m_diskInfoLabel->SetLabel(wxString::Format(L"正在扫描: %s → %s",
                scannerName.wx_str(), fileDetail.wx_str()));
        } else if (info.filesScanned > 0) {
            m_diskInfoLabel->SetLabel(wxString::Format(L"正在扫描: %s (%d 个文件)",
                scannerName.wx_str(), info.filesScanned));
        } else {
            m_diskInfoLabel->SetLabel(L"正在扫描: " + scannerName);
        }
    }
}

void DashboardPanel::RestoreDiskInfo() {
    const auto& colors = ThemeManager::Instance().GetColors();
    // 隐藏扫描结果，恢复快捷卡片和最近操作
    HideScanResults();

    // 恢复 CircularProgress 为磁盘使用率模式
    int percent = 0;
    if (m_totalBytes > 0) {
        percent = static_cast<int>((m_usedBytes * 100) / m_totalBytes);
    }
    m_progressCtrl->SetValue(percent);
    m_progressCtrl->SetLabel(wxString::Format(L"%d%%", percent));
    m_progressCtrl->SetSubLabel(L"已使用");

    // 恢复颜色
    if (percent > 90) {
        m_progressCtrl->SetProgressColor(colors.danger);
    } else if (percent > 75) {
        m_progressCtrl->SetProgressColor(colors.warning);
    } else {
        m_progressCtrl->SetProgressColor(colors.accent);
    }

    // 恢复磁盘信息文字
    using namespace IceClean::Utils;
    wxString info = wxString::Format(L"%s / %s",
        FormatUtil::FormatFileSize(m_usedBytes).c_str(),
        FormatUtil::FormatFileSize(m_totalBytes).c_str());
    m_diskInfoLabel->SetLabel(info);
}

void DashboardPanel::OnScanButton(wxCommandEvent& event) {
    SetScanning(true);
    // 发送扫描请求事件，由MainWindow处理
    wxThreadEvent scanEvt(wxEVT_SCAN_REQUEST);
    scanEvt.SetInt(0);
    wxPostEvent(GetParent(), scanEvt);
}

void DashboardPanel::OnStopButton(wxCommandEvent& event) {
    if (m_stopForceMode) {
        // 二次点击：强制停止
        m_diskInfoLabel->SetLabel(L"强制停止扫描...");
        wxThreadEvent stopEvt(wxEVT_SCAN_STOP);
        stopEvt.SetInt(1);  // 1=强制停止
        wxPostEvent(GetParent(), stopEvt);
    } else {
        // 首次点击：请求停止
        m_stopForceMode = true;
        m_stopButton->SetLabel(L"强制停止");
        m_diskInfoLabel->SetLabel(L"正在停止扫描... (再次点击强制停止)");
        wxThreadEvent stopEvt(wxEVT_SCAN_STOP);
        stopEvt.SetInt(0);  // 0=正常停止
        wxPostEvent(GetParent(), stopEvt);
    }
}

void DashboardPanel::OnQuickAccessCard(wxCommandEvent& event) {
    // 所有快捷卡片都触发一键扫描
    OnScanButton(event);
}

void DashboardPanel::SetLastScanResult(const IceClean::Models::ScanResult& result) {
    m_lastScanResult = result;
    UpdateHealthScore();
}

void DashboardPanel::UpdateHealthScore() {
    const auto& colors = ThemeManager::Instance().GetColors();
    int score = 100;  // 满分100

    // 1. 磁盘使用率（权重30%）
    double usagePercent = 0.0;
    if (m_totalBytes > 0) {
        usagePercent = static_cast<double>(m_usedBytes) / m_totalBytes * 100.0;
    }
    if (usagePercent > 95) {
        score -= 30;
    } else if (usagePercent > 90) {
        score -= 22;
    } else if (usagePercent > 80) {
        score -= 15;
    } else if (usagePercent > 70) {
        score -= 8;
    }

    // 2. 垃圾文件量（权重25%）
    uint64_t junkSize = 0;
    for (const auto& cat : m_lastScanResult.categories) {
        junkSize += cat.totalSize;
    }
    double junkGB = static_cast<double>(junkSize) / (1024.0 * 1024.0 * 1024.0);
    if (junkGB > 20) {
        score -= 25;
    } else if (junkGB > 10) {
        score -= 18;
    } else if (junkGB > 5) {
        score -= 12;
    } else if (junkGB > 2) {
        score -= 6;
    } else if (junkGB > 0.5) {
        score -= 3;
    }

    // 3. 休眠文件（权重15%）
    bool hasHibernation = false;
    for (const auto& cat : m_lastScanResult.categories) {
        if (cat.name.find(L"休眠") != std::wstring::npos && cat.totalSize > 0) {
            hasHibernation = true;
            break;
        }
    }
    if (hasHibernation) {
        score -= 10;
    }

    // 4. 浏览器缓存（权重15%）
    bool hasBrowserCache = false;
    for (const auto& cat : m_lastScanResult.categories) {
        if (cat.name.find(L"浏览器") != std::wstring::npos && cat.totalSize > 100 * 1024 * 1024) {
            hasBrowserCache = true;
            break;
        }
    }
    if (hasBrowserCache) {
        score -= 8;
    }

    // 5. 开发工具缓存（权重15%）
    bool hasDevCache = false;
    for (const auto& cat : m_lastScanResult.categories) {
        if (cat.name.find(L"开发") != std::wstring::npos && cat.totalSize > 0) {
            hasDevCache = true;
            break;
        }
    }
    if (hasDevCache) {
        score -= 5;
    }

    // 限制范围
    score = std::max(0, std::min(100, score));

    // 更新UI
    m_healthScoreLabel->SetLabel(wxString::Format(L"健康评分: %d", score));

    // 评分颜色和描述
    if (score >= 80) {
        m_healthScoreLabel->SetForegroundColour(colors.success);
        m_healthDescLabel->SetLabel(L"系统状态良好");
        m_healthDescLabel->SetForegroundColour(colors.success);
    } else if (score >= 60) {
        m_healthScoreLabel->SetForegroundColour(colors.warning);
        m_healthDescLabel->SetLabel(L"建议清理垃圾文件");
        m_healthDescLabel->SetForegroundColour(colors.warning);
    } else {
        m_healthScoreLabel->SetForegroundColour(colors.danger);
        m_healthDescLabel->SetLabel(L"磁盘空间严重不足，请立即清理");
        m_healthDescLabel->SetForegroundColour(colors.danger);
    }
}

// ── 扫描结果区域创建 ──

void DashboardPanel::CreateResultSection(wxSizer* parentSizer) {
    const auto& colors = ThemeManager::Instance().GetColors();
    m_resultSection = new wxPanel(this, wxID_ANY);
    m_resultSection->SetBackgroundColour(colors.surface);

    auto* sectionSizer = new wxBoxSizer(wxVERTICAL);

    // 摘要标签
    m_resultSummaryLabel = new wxStaticText(m_resultSection, wxID_ANY, L"");
    m_resultSummaryLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                         false, L"微软雅黑"));
    m_resultSummaryLabel->SetForegroundColour(colors.textPrimary);
    sectionSizer->Add(m_resultSummaryLabel, 0, wxLEFT | wxRIGHT, 20);
    sectionSizer->AddSpacer(8);

    // 可滚动分类列表
    m_resultScroller = new wxScrolledWindow(m_resultSection, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                             wxVSCROLL | wxBORDER_NONE);
    m_resultScroller->SetBackgroundColour(colors.surface);
    m_resultScroller->SetScrollRate(0, 10);

    m_resultSizer = new wxBoxSizer(wxVERTICAL);
    m_resultScroller->SetSizer(m_resultSizer);

    sectionSizer->Add(m_resultScroller, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    sectionSizer->AddSpacer(8);

    m_resultSection->SetSizer(sectionSizer);
    parentSizer->Add(m_resultSection, 1, wxEXPAND);

    // 初始隐藏
    m_resultSection->Hide();
}

// ── 扫描结果显示/隐藏 ──

void DashboardPanel::SetScanResult(const IceClean::Models::ScanResult& result) {
    m_scanResult = result;
    ShowScanResults();
}

void DashboardPanel::ShowScanResults() {
    // 隐藏快捷卡片
    if (m_quickAccessSizer) {
        m_quickAccessSizer->Show(false);
    }

    // 构建分类列表
    BuildCategoryList();

    // 更新摘要
    UpdateResultSummary();

    // 显示结果区域
    m_resultSection->Show();

    // 切换按钮：隐藏扫描，显示清理
    m_scanButton->Hide();
    m_cleanButton->Show();

    Layout();
}

void DashboardPanel::HideScanResults() {
    if (!m_resultSection || !m_resultSection->IsShown()) return;

    // 隐藏结果区域
    m_resultSection->Hide();

    // 显示快捷卡片
    if (m_quickAccessSizer) {
        m_quickAccessSizer->Show(true);
    }

    // 切换按钮：隐藏清理，显示扫描
    m_cleanButton->Hide();
    m_scanButton->Show();

    Layout();
}

void DashboardPanel::BuildCategoryList() {
    const auto& colors = ThemeManager::Instance().GetColors();
    // 清空现有控件
    m_categoryUIs.clear();
    m_resultSizer->Clear(true);

    for (size_t i = 0; i < m_scanResult.categories.size(); ++i) {
        auto& cat = m_scanResult.categories[i];
        CategoryUI ui;

        // ── 分类头部 (可点击展开/折叠) ──
        ui.headerPanel = new wxPanel(m_resultScroller, wxID_ANY);
        ui.headerPanel->SetBackgroundColour(colors.surface);
        ui.headerPanel->SetCursor(wxCURSOR_HAND);

        auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);
        headerSizer->AddSpacer(8);

        // 展开/折叠箭头
        auto* arrowLabel = new wxStaticText(ui.headerPanel, wxID_ANY, L"▶");
        arrowLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        arrowLabel->SetForegroundColour(colors.textSecondary);
        headerSizer->Add(arrowLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
        ui.arrowLabel = arrowLabel;

        // 分类名称
        auto* nameLabel = new wxStaticText(ui.headerPanel, wxID_ANY, cat.name);
        nameLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
        nameLabel->SetForegroundColour(colors.textPrimary);
        headerSizer->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        // 安全等级标识
        auto* badge = new SafetyBadge(ui.headerPanel, wxID_ANY);
        badge->SetSafetyRating(cat.safety);
        headerSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        headerSizer->AddStretchSpacer();

        // 大小标签
        auto* sizeLabel = new wxStaticText(ui.headerPanel, wxID_ANY,
            IceClean::Utils::FormatUtil::FormatFileSize(cat.totalSize));
        sizeLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                  false, L"微软雅黑"));
        sizeLabel->SetForegroundColour(colors.textSecondary);
        headerSizer->Add(sizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        // 分类复选框
        ui.categoryCheck = new wxCheckBox(ui.headerPanel, wxID_ANY, wxEmptyString);
        ui.categoryCheck->SetValue(cat.safety == IceClean::Models::SafetyRating::Safe);
        ui.categoryCheck->Bind(wxEVT_CHECKBOX, &DashboardPanel::OnCategoryToggle, this);
        headerSizer->Add(ui.categoryCheck, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        ui.headerPanel->SetSizer(headerSizer);
        ui.headerPanel->Bind(wxEVT_LEFT_DOWN, &DashboardPanel::OnCategoryHeaderClick, this);

        m_resultSizer->Add(ui.headerPanel, 0, wxEXPAND | wxBOTTOM, 1);

        // ── 分类详情 (默认隐藏) ──
        ui.detailPanel = new wxPanel(m_resultScroller, wxID_ANY);
        ui.detailPanel->SetBackgroundColour(colors.surfaceHover);
        ui.detailPanel->Hide();

        auto* detailSizer = new wxBoxSizer(wxVERTICAL);
        detailSizer->AddSpacer(4);

        // 分类描述
        if (!cat.description.empty()) {
            auto* descLabel = new wxStaticText(ui.detailPanel, wxID_ANY, cat.description);
            descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
            descLabel->SetForegroundColour(colors.textDisabled);
            descLabel->Wrap(600);
            detailSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 36);
            detailSizer->AddSpacer(4);
        }

        // 文件项列表
        for (size_t j = 0; j < cat.items.size(); ++j) {
            const auto& item = cat.items[j];
            auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
            itemSizer->AddSpacer(36);

            auto* itemCheck = new wxCheckBox(ui.detailPanel, wxID_ANY, wxEmptyString);
            itemCheck->SetValue(cat.safety == IceClean::Models::SafetyRating::Safe);
            itemCheck->Bind(wxEVT_CHECKBOX, &DashboardPanel::OnItemToggle, this);
            ui.itemChecks.push_back(itemCheck);
            itemSizer->Add(itemCheck, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

            auto* pathLabel = new wxStaticText(ui.detailPanel, wxID_ANY, item.path);
            pathLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
            pathLabel->SetForegroundColour(colors.textSecondary);
            itemSizer->Add(pathLabel, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

            auto* itemSizeLabel = new wxStaticText(ui.detailPanel, wxID_ANY,
                IceClean::Utils::FormatUtil::FormatFileSize(item.size));
            itemSizeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                         false, L"微软雅黑"));
            itemSizeLabel->SetForegroundColour(colors.textDisabled);
            itemSizer->Add(itemSizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

            detailSizer->Add(itemSizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 2);
        }

        detailSizer->AddSpacer(4);
        ui.detailPanel->SetSizer(detailSizer);

        m_resultSizer->Add(ui.detailPanel, 0, wxEXPAND | wxBOTTOM, 4);

        ui.expanded = false;
        m_categoryUIs.push_back(std::move(ui));
    }

    m_resultScroller->FitInside();
}

void DashboardPanel::UpdateResultSummary() {
    int categoryCount = static_cast<int>(m_scanResult.categories.size());
    using namespace IceClean::Utils;
    m_resultSummaryLabel->SetLabel(
        wxString::Format(L"发现 %d 个类别，可释放 %s 空间",
            categoryCount,
            FormatUtil::FormatFileSize(m_scanResult.totalSize).c_str()));
}

std::vector<std::wstring> DashboardPanel::GetSelectedPaths() const {
    std::vector<std::wstring> paths;
    for (size_t i = 0; i < m_scanResult.categories.size() && i < m_categoryUIs.size(); ++i) {
        const auto& cat = m_scanResult.categories[i];
        const auto& ui = m_categoryUIs[i];

        if (!ui.categoryCheck->GetValue()) continue;

        for (size_t j = 0; j < cat.items.size() && j < ui.itemChecks.size(); ++j) {
            if (ui.itemChecks[j]->GetValue()) {
                paths.push_back(cat.items[j].path);
            }
        }
    }
    return paths;
}

// ── 扫描结果事件处理 ──

void DashboardPanel::OnCleanButton(wxCommandEvent& event) {
    wxThreadEvent cleanEvt(wxEVT_CLEAN_PROGRESS);
    cleanEvt.SetInt(0);
    wxPostEvent(GetParent(), cleanEvt);
}

void DashboardPanel::OnCategoryToggle(wxCommandEvent& event) {
    wxCheckBox* check = static_cast<wxCheckBox*>(event.GetEventObject());
    bool checked = check->GetValue();

    for (auto& ui : m_categoryUIs) {
        if (ui.categoryCheck == check) {
            for (auto* itemCheck : ui.itemChecks) {
                itemCheck->SetValue(checked);
            }
            break;
        }
    }
    UpdateResultSummary();
}

void DashboardPanel::OnItemToggle(wxCommandEvent& event) {
    UpdateResultSummary();
}

void DashboardPanel::OnCategoryHeaderClick(wxMouseEvent& event) {
    wxWindow* clickedPanel = static_cast<wxWindow*>(event.GetEventObject());
    while (clickedPanel && clickedPanel != m_resultScroller) {
        for (size_t i = 0; i < m_categoryUIs.size(); ++i) {
            if (m_categoryUIs[i].headerPanel == clickedPanel) {
                auto& ui = m_categoryUIs[i];
                ui.expanded = !ui.expanded;
                if (ui.expanded) {
                    ui.detailPanel->Show();
                    ui.arrowLabel->SetLabel(L"▼");
                } else {
                    ui.detailPanel->Hide();
                    ui.arrowLabel->SetLabel(L"▶");
                }
                m_resultScroller->FitInside();
                return;
            }
        }
        clickedPanel = clickedPanel->GetParent();
    }
    event.Skip();
}

void DashboardPanel::LoadCumulativeStats() {
    auto totalCleaned = IceClean::Core::Safety::UsageStats::Instance().GetTotalCleanedBytes();
    m_cumulativeLabel->SetLabel(
        wxString::Format(L"累计清理: %s",
            IceClean::Utils::FormatUtil::FormatFileSize(totalCleaned).c_str()));
}

void DashboardPanel::OnDiskTimer(wxTimerEvent& /*event*/) {
    // 定时刷新C盘空间信息
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        uint64_t total = totalBytes.QuadPart;
        uint64_t free = freeBytesAvailable.QuadPart;
        uint64_t used = total - free;
        UpdateDiskInfo(used, total);
    }
}

} // namespace IceClean::Gui
