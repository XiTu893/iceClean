#include "DashboardPanel.h"
#include "gui/Events.h"
#include "utils/FormatUtil.h"

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(DashboardPanel, wxPanel)
wxEND_EVENT_TABLE()

DashboardPanel::DashboardPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(*wxWHITE);
    CreateControls();
}

void DashboardPanel::CreateControls() {
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
    titleLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    infoSizer->Add(titleLabel, 0, wxBOTTOM, 8);

    m_diskInfoLabel = new wxStaticText(healthCard, wxID_ANY, L"0 GB / 0 GB");
    m_diskInfoLabel->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
    m_diskInfoLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    infoSizer->Add(m_diskInfoLabel, 0, wxBOTTOM, 4);

    auto* statusLabel = new wxStaticText(healthCard, wxID_ANY, L"状态良好");
    statusLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    statusLabel->SetForegroundColour(wxColour(0x10, 0x7C, 0x10));
    infoSizer->Add(statusLabel);

    healthContentSizer->Add(infoSizer, 1, wxALIGN_CENTER_VERTICAL);
    healthSizer->Add(healthContentSizer, 0, wxEXPAND);

    mainSizer->Add(healthCard, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(16);

    // ── 一键扫描按钮 ──
    auto* scanSizer = new wxBoxSizer(wxHORIZONTAL);
    scanSizer->AddStretchSpacer();

    m_scanButton = new wxButton(this, wxID_ANY, L"一键扫描", wxDefaultPosition, wxSize(200, 48));
    m_scanButton->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    m_scanButton->SetBackgroundColour(wxColour(0x00, 0x78, 0xD4));
    m_scanButton->SetForegroundColour(*wxWHITE);
    m_scanButton->SetCursor(wxCURSOR_HAND);
    m_scanButton->Bind(wxEVT_BUTTON, &DashboardPanel::OnScanButton, this);
    scanSizer->Add(m_scanButton, 0);

    scanSizer->AddStretchSpacer();
    mainSizer->Add(scanSizer, 0, wxEXPAND);
    mainSizer->AddSpacer(16);

    // ── 快捷入口卡片 ──
    CreateQuickAccessCards(mainSizer);
    mainSizer->AddSpacer(16);

    // ── 最近操作 ──
    CreateRecentOperations(mainSizer);
    mainSizer->AddSpacer(20);

    SetSizer(mainSizer);
}

void DashboardPanel::CreateQuickAccessCards(wxSizer* parentSizer) {
    auto* gridSizer = new wxGridSizer(2, 2, 10, 10);

    // 临时文件卡片
    m_cardTemp = new CardPanel(this, wxID_ANY, L"临时文件");
    m_cardTemp->SetClickable(true);
    m_cardTemp->SetMinSize(wxSize(200, 80));
    auto* tempLabel = new wxStaticText(m_cardTemp, wxID_ANY, L"清理系统临时文件\n释放磁盘空间");
    tempLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    tempLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    m_cardTemp->GetContentSizer()->Add(tempLabel, 1, wxEXPAND);
    m_cardTemp->Bind(wxEVT_BUTTON, &DashboardPanel::OnQuickAccessCard, this);
    gridSizer->Add(m_cardTemp, 1, wxEXPAND);

    // 更新缓存卡片
    m_cardUpdate = new CardPanel(this, wxID_ANY, L"更新缓存");
    m_cardUpdate->SetClickable(true);
    m_cardUpdate->SetMinSize(wxSize(200, 80));
    auto* updateLabel = new wxStaticText(m_cardUpdate, wxID_ANY, L"清理Windows更新缓存\n释放大量空间");
    updateLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    updateLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    m_cardUpdate->GetContentSizer()->Add(updateLabel, 1, wxEXPAND);
    m_cardUpdate->Bind(wxEVT_BUTTON, &DashboardPanel::OnQuickAccessCard, this);
    gridSizer->Add(m_cardUpdate, 1, wxEXPAND);

    // 浏览器缓存卡片
    m_cardBrowser = new CardPanel(this, wxID_ANY, L"浏览器缓存");
    m_cardBrowser->SetClickable(true);
    m_cardBrowser->SetMinSize(wxSize(200, 80));
    auto* browserLabel = new wxStaticText(m_cardBrowser, wxID_ANY, L"清理浏览器缓存\n保护隐私安全");
    browserLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    browserLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    m_cardBrowser->GetContentSizer()->Add(browserLabel, 1, wxEXPAND);
    m_cardBrowser->Bind(wxEVT_BUTTON, &DashboardPanel::OnQuickAccessCard, this);
    gridSizer->Add(m_cardBrowser, 1, wxEXPAND);

    // 休眠文件卡片
    m_cardHibernation = new CardPanel(this, wxID_ANY, L"休眠文件");
    m_cardHibernation->SetClickable(true);
    m_cardHibernation->SetMinSize(wxSize(200, 80));
    auto* hibernationLabel = new wxStaticText(m_cardHibernation, wxID_ANY, L"关闭休眠功能\n释放数GB空间");
    hibernationLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
    hibernationLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    m_cardHibernation->GetContentSizer()->Add(hibernationLabel, 1, wxEXPAND);
    m_cardHibernation->Bind(wxEVT_BUTTON, &DashboardPanel::OnQuickAccessCard, this);
    gridSizer->Add(m_cardHibernation, 1, wxEXPAND);

    // 将网格放入带边距的容器
    auto* cardContainer = new wxBoxSizer(wxHORIZONTAL);
    cardContainer->Add(gridSizer, 1, wxEXPAND);
    parentSizer->Add(cardContainer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
}

void DashboardPanel::CreateRecentOperations(wxSizer* parentSizer) {
    auto* recentCard = new CardPanel(this, wxID_ANY, L"最近操作");
    auto* contentSizer = recentCard->GetContentSizer();

    m_recentList = new wxListCtrl(recentCard, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_recentList->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));

    m_recentList->AppendColumn(L"操作", wxLIST_FORMAT_LEFT, 120);
    m_recentList->AppendColumn(L"描述", wxLIST_FORMAT_LEFT, 250);
    m_recentList->AppendColumn(L"释放空间", wxLIST_FORMAT_LEFT, 100);
    m_recentList->AppendColumn(L"时间", wxLIST_FORMAT_LEFT, 140);

    contentSizer->Add(m_recentList, 1, wxEXPAND | wxALL, 0);

    parentSizer->Add(recentCard, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
}

void DashboardPanel::UpdateDiskInfo(uint64_t usedBytes, uint64_t totalBytes) {
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
        FormatUtil::FormatFileSize(usedBytes),
        FormatUtil::FormatFileSize(totalBytes));
    m_diskInfoLabel->SetLabel(info);

    // 根据使用率调整颜色
    if (percent > 90) {
        m_progressCtrl->SetProgressColor(wxColour(0xE8, 0x11, 0x23)); // 红色
    } else if (percent > 75) {
        m_progressCtrl->SetProgressColor(wxColour(0xFF, 0x8C, 0x00)); // 橙色
    } else {
        m_progressCtrl->SetProgressColor(wxColour(0x00, 0x78, 0xD4)); // 蓝色
    }
}

void DashboardPanel::UpdateRecentOperations(const std::vector<IceClean::Models::OperationRecord>& records) {
    m_recentList->DeleteAllItems();
    using namespace IceClean::Models;
    using namespace IceClean::Utils;

    for (size_t i = 0; i < records.size() && i < 20; ++i) {
        const auto& record = records[i];

        wxString typeStr;
        switch (record.type) {
            case OperationType::Clean:    typeStr = L"清理"; break;
            case OperationType::Migrate:  typeStr = L"迁移"; break;
            case OperationType::Optimize: typeStr = L"优化"; break;
            case OperationType::Restore:  typeStr = L"还原"; break;
        }

        long idx = m_recentList->InsertItem(static_cast<long>(i), typeStr);
        m_recentList->SetItem(idx, 1, record.description);
        m_recentList->SetItem(idx, 2, FormatUtil::FormatFileSize(record.size));

        // 时间格式化
        auto timeT = std::chrono::system_clock::to_time_t(record.timestamp);
        struct tm tmBuf;
        localtime_s(&tmBuf, &timeT);
        wchar_t timeStr[64];
        wcsftime(timeStr, 64, L"%Y-%m-%d %H:%M", &tmBuf);
        m_recentList->SetItem(idx, 3, timeStr);
    }
}

void DashboardPanel::SetScanning(bool scanning) {
    if (scanning) {
        m_scanButton->SetLabel(L"扫描中...");
        m_scanButton->Disable();
        // 切换 CircularProgress 为扫描进度模式
        m_progressCtrl->SetValue(0);
        m_progressCtrl->SetLabel(L"0%");
        m_progressCtrl->SetSubLabel(L"扫描中");
        m_progressCtrl->SetProgressColor(wxColour(0x00, 0x78, 0xD4));
        m_diskInfoLabel->SetLabel(L"准备扫描...");
    } else {
        m_scanButton->SetLabel(L"一键扫描");
        m_scanButton->Enable();
    }
}

void DashboardPanel::UpdateScanProgress(int completedScanners, int totalScanners,
                                         const std::wstring& currentScanner) {
    if (totalScanners <= 0) return;

    int percent = (completedScanners * 100) / totalScanners;
    m_progressCtrl->SetValue(percent);
    m_progressCtrl->SetLabel(wxString::Format(L"%d%%", percent));
    m_progressCtrl->SetSubLabel(L"扫描中");

    // 显示当前正在扫描的项目
    wxString scannerName(currentScanner);
    if (!scannerName.IsEmpty()) {
        m_diskInfoLabel->SetLabel(L"正在扫描: " + scannerName);
    }
}

void DashboardPanel::RestoreDiskInfo() {
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
        m_progressCtrl->SetProgressColor(wxColour(0xE8, 0x11, 0x23));
    } else if (percent > 75) {
        m_progressCtrl->SetProgressColor(wxColour(0xFF, 0x8C, 0x00));
    } else {
        m_progressCtrl->SetProgressColor(wxColour(0x00, 0x78, 0xD4));
    }

    // 恢复磁盘信息文字
    using namespace IceClean::Utils;
    wxString info = wxString::Format(L"%s / %s",
        FormatUtil::FormatFileSize(m_usedBytes),
        FormatUtil::FormatFileSize(m_totalBytes));
    m_diskInfoLabel->SetLabel(info);
}

void DashboardPanel::OnScanButton(wxCommandEvent& event) {
    SetScanning(true);
    // 发送扫描请求事件，由MainWindow处理
    wxThreadEvent scanEvt(wxEVT_SCAN_REQUEST);
    scanEvt.SetInt(0);
    wxPostEvent(GetParent(), scanEvt);
}

void DashboardPanel::OnQuickAccessCard(wxCommandEvent& event) {
    // 所有快捷卡片都触发一键扫描
    OnScanButton(event);
}

} // namespace IceClean::Gui
