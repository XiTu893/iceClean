#include "DashboardPanel.h"
#include "gui/controls/ThemeManager.h"
#include "gui/Events.h"
#include "core/analyzer/PerformanceMonitor.h"
#include "core/safety/UsageStats.h"
#include "utils/FormatUtil.h"
#include <windows.h>
#include <algorithm>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(DashboardPanel, wxPanel)
    EVT_TIMER(wxID_ANY, DashboardPanel::OnUpdateTimer)
wxEND_EVENT_TABLE()

DashboardPanel::DashboardPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();

    m_performanceMonitor = Core::Analyzer::PerformanceMonitor::Create();
    m_performanceMonitor->StartMonitoring(1000);

    m_updateTimer = new wxTimer(this);
    m_updateTimer->Start(1000);
    Bind(wxEVT_TIMER, &DashboardPanel::OnUpdateTimer, this, m_updateTimer->GetId());

    LoadCumulativeStats();
}

DashboardPanel::~DashboardPanel() {
    if (m_updateTimer) {
        m_updateTimer->Stop();
        delete m_updateTimer;
    }
}

void DashboardPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(0, 20);

    // ── Row 1: 系统健康 + C盘空间 (并排) ──
    auto* summarySizer = new wxBoxSizer(wxHORIZONTAL);
    summarySizer->Add(20, 0);

    // 系统健康卡片
    {
        auto* statsCard = new CardPanel(this, wxID_ANY, L"系统健康");
        statsCard->SetBackgroundColour(colors.surface);

        m_healthProgress = new CircularProgress(statsCard, wxID_ANY, wxDefaultPosition, wxSize(100, 100));
        m_healthProgress->SetValue(100);
        m_healthProgress->SetLabel(L"100");
        m_healthProgress->SetSubLabel(L"健康评分");

        m_healthScoreLabel = new wxStaticText(statsCard, wxID_ANY, L"系统健康评分");
        m_healthScoreLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
        m_healthScoreLabel->SetForegroundColour(colors.textSecondary);

        m_healthDescLabel = new wxStaticText(statsCard, wxID_ANY, L"状态良好");
        m_healthDescLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
        m_healthDescLabel->SetForegroundColour(colors.success);

        m_cumulativeLabel = new wxStaticText(statsCard, wxID_ANY, L"累计清理: 0 B");
        m_cumulativeLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
        m_cumulativeLabel->SetForegroundColour(colors.accent);

        auto* cardContent = statsCard->GetCardSizer();
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(m_healthProgress, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
        auto* infoCol = new wxBoxSizer(wxVERTICAL);
        infoCol->Add(m_healthScoreLabel, 0, wxBOTTOM, 4);
        infoCol->Add(m_healthDescLabel, 0, wxBOTTOM, 8);
        infoCol->Add(m_cumulativeLabel, 0);
        row->Add(infoCol, 1, wxALIGN_CENTER_VERTICAL);
        cardContent->Add(row, 0, wxEXPAND);

        summarySizer->Add(statsCard, 1, wxEXPAND | wxRIGHT, 10);
    }

    // C盘空间卡片（含环形图 + 文字信息）
    {
        auto* diskInfoCard = new CardPanel(this, wxID_ANY, L"C盘空间");
        diskInfoCard->SetBackgroundColour(colors.surface);

        m_diskProgress = new CircularProgress(diskInfoCard, wxID_ANY, wxDefaultPosition, wxSize(100, 100));
        m_diskProgress->SetValue(0);
        m_diskProgress->SetLabel(L"0%");
        m_diskProgress->SetSubLabel(L"使用率");

        auto* diskTitle = new wxStaticText(diskInfoCard, wxID_ANY, L"本地磁盘 (C:)");
        diskTitle->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
        diskTitle->SetForegroundColour(colors.textPrimary);

        m_diskUsageLabel = new wxStaticText(diskInfoCard, wxID_ANY, L"0 B / 0 B");
        m_diskUsageLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
        m_diskUsageLabel->SetForegroundColour(colors.textPrimary);

        auto* diskHealthLabel = new wxStaticText(diskInfoCard, wxID_ANY, L"健康: 100%");
        diskHealthLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
        diskHealthLabel->SetForegroundColour(colors.textSecondary);

        m_spaceWarningLabel = new wxStaticText(diskInfoCard, wxID_ANY, L"");
        m_spaceWarningLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
        m_spaceWarningLabel->Hide();

        auto* cardContent = diskInfoCard->GetCardSizer();
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(m_diskProgress, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);
        auto* infoCol = new wxBoxSizer(wxVERTICAL);
        infoCol->Add(diskTitle, 0, wxBOTTOM, 4);
        infoCol->Add(m_diskUsageLabel, 0, wxBOTTOM, 4);
        infoCol->Add(diskHealthLabel, 0, wxBOTTOM, 4);
        infoCol->Add(m_spaceWarningLabel, 0);
        row->Add(infoCol, 1, wxALIGN_CENTER_VERTICAL);
        cardContent->Add(row, 0, wxEXPAND);

        summarySizer->Add(diskInfoCard, 1, wxEXPAND);
    }

    mainSizer->Add(summarySizer, 0, wxEXPAND | wxRIGHT, 20);
    mainSizer->Add(0, 20);

    // ── Row 2: CPU / 内存 / 网络 ──
    auto* monitorSizer = new wxBoxSizer(wxHORIZONTAL);
    monitorSizer->Add(20, 0);

    // CPU监控卡片
    {
        auto* card = new CardPanel(this, wxID_ANY, L"CPU使用率");
        card->SetBackgroundColour(colors.surface);
        card->GetCardSizer()->Add(0, 8);
        m_cpuProgress = new CircularProgress(card, wxID_ANY, wxDefaultPosition, wxSize(100, 100));
        m_cpuProgress->SetValue(0);
        m_cpuProgress->SetLabel(L"0%");
        m_cpuProgress->SetSubLabel(L"使用率");
        card->GetCardSizer()->Add(m_cpuProgress, 0, wxALIGN_CENTER);
        m_cpuInfoLabel = new wxStaticText(card, wxID_ANY, L"--");
        m_cpuInfoLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
        m_cpuInfoLabel->SetForegroundColour(colors.textSecondary);
        card->GetCardSizer()->Add(m_cpuInfoLabel, 0, wxTOP | wxLEFT | wxRIGHT, 8);
        card->GetCardSizer()->Add(0, 4);
        monitorSizer->Add(card, 1, wxEXPAND | wxALL, 8);
    }

    // 内存监控卡片
    {
        auto* card = new CardPanel(this, wxID_ANY, L"内存使用率");
        card->SetBackgroundColour(colors.surface);
        card->GetCardSizer()->Add(0, 8);
        m_memProgress = new CircularProgress(card, wxID_ANY, wxDefaultPosition, wxSize(100, 100));
        m_memProgress->SetValue(0);
        m_memProgress->SetLabel(L"0%");
        m_memProgress->SetSubLabel(L"已用/总");
        card->GetCardSizer()->Add(m_memProgress, 0, wxALIGN_CENTER);
        m_memInfoLabel = new wxStaticText(card, wxID_ANY, L"-- / --");
        m_memInfoLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
        m_memInfoLabel->SetForegroundColour(colors.textSecondary);
        card->GetCardSizer()->Add(m_memInfoLabel, 0, wxTOP | wxLEFT | wxRIGHT, 8);
        card->GetCardSizer()->Add(0, 4);
        monitorSizer->Add(card, 1, wxEXPAND | wxALL, 8);
    }

    // 网络监控卡片
    {
        auto* card = new CardPanel(this, wxID_ANY, L"网络速率");
        card->SetBackgroundColour(colors.surface);
        card->GetCardSizer()->Add(0, 8);
        m_netLabel = new wxStaticText(card, wxID_ANY, L"↓0 KB/s ↑0 KB/s");
        m_netLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
        m_netLabel->SetForegroundColour(colors.textPrimary);
        card->GetCardSizer()->Add(m_netLabel, 0, wxALIGN_CENTER | wxTOP, 12);
        auto* netInfo = new wxStaticText(card, wxID_ANY, L"实时监控");
        netInfo->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
        netInfo->SetForegroundColour(colors.textSecondary);
        card->GetCardSizer()->Add(netInfo, 0, wxTOP | wxLEFT | wxRIGHT, 4);
        card->GetCardSizer()->Add(0, 4);
        monitorSizer->Add(card, 1, wxEXPAND | wxALL, 8);
    }

    mainSizer->Add(monitorSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    mainSizer->AddStretchSpacer();
    SetSizer(mainSizer);
}

void DashboardPanel::OnUpdateTimer(wxTimerEvent& /*event*/) {
    UpdatePerformanceStats();
    UpdateDiskSpaceInfo();
    UpdateHealthScore();
}

void DashboardPanel::UpdatePerformanceStats() {
    if (!m_performanceMonitor) return;

    auto snapshot = m_performanceMonitor->GetFullSnapshot();

    int cpuPercent = static_cast<int>(snapshot.cpu.usagePercent);
    if (m_cpuProgress) {
        m_cpuProgress->SetValue(cpuPercent);
        m_cpuProgress->SetLabel(wxString::Format(L"%d%%", cpuPercent));
    }
    if (m_cpuInfoLabel) {
        if (snapshot.cpu.coreCount > 0 && snapshot.cpu.logicalProcessorCount > 0) {
            m_cpuInfoLabel->SetLabel(wxString::Format(L"%dC/%dT  %s",
                snapshot.cpu.coreCount,
                snapshot.cpu.logicalProcessorCount,
                FormatFrequency(snapshot.cpu.frequencyGHz)));
        } else {
            m_cpuInfoLabel->SetLabel(L"CPU信息不可用");
        }
    }

    int memPercent = static_cast<int>(snapshot.memory.usagePercent);
    if (m_memProgress) {
        m_memProgress->SetValue(memPercent);
        m_memProgress->SetLabel(wxString::Format(L"%d%%", memPercent));
    }
    if (m_memInfoLabel) {
        m_memInfoLabel->SetLabel(wxString::Format(L"%s / %s",
            Utils::FormatUtil::FormatFileSize(snapshot.memory.usedBytes),
            Utils::FormatUtil::FormatFileSize(snapshot.memory.totalBytes)));
    }

    if (m_diskProgress && !snapshot.disks.empty()) {
        const Models::DiskSnapshot* cDisk = &snapshot.disks[0];
        for (const auto& disk : snapshot.disks) {
            if (disk.driveLetter == L'C') { cDisk = &disk; break; }
        }
        int diskPercent = static_cast<int>(cDisk->usagePercent);
        m_diskProgress->SetValue(diskPercent);
        m_diskProgress->SetLabel(wxString::Format(L"%d%%", diskPercent));
    }

    if (m_netLabel) {
        m_netLabel->SetLabel(wxString::Format(L"↓%s ↑%s",
            Utils::FormatUtil::FormatSpeed(snapshot.network.downloadSpeedBps),
            Utils::FormatUtil::FormatSpeed(snapshot.network.uploadSpeedBps)));
    }
}

void DashboardPanel::UpdateDiskSpaceInfo() {
    const auto& colors = ThemeManager::Instance().GetColors();

    const uint64_t now = ::GetTickCount64();
    if (now - m_lastDiskCheck < 5000 && m_lastDiskCheck != 0) return;
    m_lastDiskCheck = now;

    ULARGE_INTEGER freeBytesAvailable{}, totalBytes{}, totalFreeBytes{};
    if (!::GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvailable, &totalBytes, &totalFreeBytes)) return;

    const uint64_t total = totalBytes.QuadPart;
    const uint64_t free  = freeBytesAvailable.QuadPart;
    const uint64_t used  = total > free ? total - free : 0;

    if (m_diskUsageLabel) {
        m_diskUsageLabel->SetLabel(wxString::Format(L"%s / %s",
            Utils::FormatUtil::FormatFileSize(used),
            Utils::FormatUtil::FormatFileSize(total)));
    }

    int percent = 0;
    if (total > 0) percent = static_cast<int>((used * 100) / total);

    if (m_spaceWarningLabel) {
        if (percent > 90) {
            m_spaceWarningLabel->SetLabel(L"C盘空间不足！");
            m_spaceWarningLabel->SetForegroundColour(colors.danger);
            m_spaceWarningLabel->Show();
        } else if (percent > 80) {
            m_spaceWarningLabel->SetLabel(L"C盘空间偏低");
            m_spaceWarningLabel->SetForegroundColour(colors.warning);
            m_spaceWarningLabel->Show();
        } else {
            m_spaceWarningLabel->Hide();
        }
        Layout();
    }
}

void DashboardPanel::UpdateHealthScore() {
    const auto& colors = ThemeManager::Instance().GetColors();
    int score = 100;

    auto subtract = [](int usage, int tier1, int tier2, int tier3) {
        if (usage > tier1) return 3;
        if (usage > tier2) return 2;
        if (usage > tier3) return 1;
        return 0;
    };

    if (m_cpuProgress) {
        const int deduction[] = {0, 8, 15, 25};
        score -= deduction[subtract(m_cpuProgress->GetValue(), 90, 80, 70)];
    }
    if (m_memProgress) {
        const int deduction[] = {0, 8, 15, 25};
        score -= deduction[subtract(m_memProgress->GetValue(), 90, 80, 70)];
    }
    if (m_diskProgress) {
        const int deduction[] = {0, 6, 12, 20};
        score -= deduction[subtract(m_diskProgress->GetValue(), 90, 80, 70)];
    }

    score = std::clamp(score, 0, 100);

    if (m_healthProgress) {
        m_healthProgress->SetValue(score);
        m_healthProgress->SetLabel(wxString::Format(L"%d", score));
    }

    if (m_healthDescLabel && m_healthScoreLabel) {
        if (score >= 80) {
            m_healthScoreLabel->SetForegroundColour(colors.success);
            m_healthDescLabel->SetLabel(L"状态良好");
            m_healthDescLabel->SetForegroundColour(colors.success);
        } else if (score >= 60) {
            m_healthScoreLabel->SetForegroundColour(colors.warning);
            m_healthDescLabel->SetLabel(L"建议优化系统");
            m_healthDescLabel->SetForegroundColour(colors.warning);
        } else {
            m_healthScoreLabel->SetForegroundColour(colors.danger);
            m_healthDescLabel->SetLabel(L"系统负载较高");
            m_healthDescLabel->SetForegroundColour(colors.danger);
        }
    }
}

void DashboardPanel::LoadCumulativeStats() {
    if (!m_cumulativeLabel) return;
    uint64_t total = IceClean::Core::Safety::UsageStats::Instance().GetTotalCleanedBytes();
    m_cumulativeLabel->SetLabel(wxString::Format(L"累计清理: %s",
        Utils::FormatUtil::FormatFileSize(total)));
}

wxString DashboardPanel::FormatBytes(uint64_t bytes) {
    return Utils::FormatUtil::FormatFileSize(bytes);
}

wxString DashboardPanel::FormatSpeed(uint64_t bytesPerSec) {
    return Utils::FormatUtil::FormatSpeed(bytesPerSec);
}

wxString DashboardPanel::FormatFrequency(double ghz) {
    if (ghz >= 1.0) return wxString::Format(L"%.2f GHz", ghz);
    return wxString::Format(L"%.0f MHz", ghz * 1000.0);
}

} // namespace IceClean::Gui