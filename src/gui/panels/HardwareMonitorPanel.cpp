#include "HardwareMonitorPanel.h"
#include "gui/controls/ThemeManager.h"
#include <algorithm>

namespace IceClean::Gui {

wxString HardwareMonitorPanel::FormatBytes(uint64_t bytes) {
    if (bytes >= 1024ULL * 1024 * 1024 * 1024) {
        return wxString::Format(L"%.2f TB", bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0));
    } else if (bytes >= 1024ULL * 1024 * 1024) {
        return wxString::Format(L"%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    } else if (bytes >= 1024ULL * 1024) {
        return wxString::Format(L"%.1f MB", bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024) {
        return wxString::Format(L"%.1f KB", bytes / 1024.0);
    }
    return wxString::Format(L"%llu B", static_cast<unsigned long long>(bytes));
}

wxString HardwareMonitorPanel::FormatSpeed(uint64_t bytesPerSec) {
    if (bytesPerSec >= 1024ULL * 1024) {
        return wxString::Format(L"%.1f MB/s", bytesPerSec / (1024.0 * 1024.0));
    } else if (bytesPerSec >= 1024) {
        return wxString::Format(L"%.1f KB/s", bytesPerSec / 1024.0);
    }
    return wxString::Format(L"%llu B/s", static_cast<unsigned long long>(bytesPerSec));
}

wxString HardwareMonitorPanel::FormatFrequency(double ghz) {
    if (ghz >= 1.0) {
        return wxString::Format(L"%.2f GHz", ghz);
    } else {
        return wxString::Format(L"%.0f MHz", ghz * 1000.0);
    }
}

HardwareMonitorPanel::HardwareMonitorPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.background);

    BuildLayout();

    m_monitor = IceClean::Core::Analyzer::PerformanceMonitor::Create();
    LoadHardwareSummary();

    ThemeManager::Instance().RegisterChangeCallback([this](const ThemeColors&) {
        Refresh();
    });
}

HardwareMonitorPanel::~HardwareMonitorPanel() {
    StopMonitoring();
}

void HardwareMonitorPanel::BuildLayout() {
    const auto& colors = ThemeManager::Instance().GetColors();

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // ── 页面标题 ──
    mainSizer->AddSpacer(10);
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"硬件监控");
    titleLabel->SetFont(wxFont(13, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(6);

    // ── 6 个概览卡片（2 行 x 3 列）──
    auto* gridSizer = new wxFlexGridSizer(2, 8, 8, 8);

    auto makeCard = [&](wxWindow* parent, const wxString& title, const wxColour& accent,
                        wxStaticText*& valueLabel, wxStaticText* infoLabelArg) -> wxPanel* {
        auto* card = new wxPanel(parent, wxID_ANY);
        card->SetBackgroundColour(colors.surface);
        auto* cs = new wxBoxSizer(wxVERTICAL);
        cs->AddSpacer(8);

        auto* titleLbl = new wxStaticText(card, wxID_ANY, title);
        titleLbl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
        titleLbl->SetForegroundColour(colors.textSecondary);
        cs->Add(titleLbl, 0, wxLEFT | wxRIGHT, 10);

        auto* value = new wxStaticText(card, wxID_ANY, L"--%");
        value->SetFont(wxFont(18, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
        value->SetForegroundColour(colors.textPrimary);
        cs->Add(value, 0, wxLEFT | wxRIGHT, 10);

        if (infoLabelArg) {
            auto* info = new wxStaticText(card, wxID_ANY, L"等待数据...");
            info->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
            info->SetForegroundColour(colors.textSecondary);
            cs->Add(info, 0, wxLEFT | wxRIGHT, 10);
        }
        cs->AddSpacer(6);
        card->SetSizer(cs);
        valueLabel = value;
        return card;
    };

    gridSizer->Add(makeCard(this, L"CPU", colors.accent, m_cpuUsageLabel, m_cpuInfoLabel), 1, wxEXPAND);
    gridSizer->Add(makeCard(this, L"内存", wxColour(80, 180, 120), m_memoryUsageLabel, m_memoryInfoLabel), 1, wxEXPAND);
    gridSizer->Add(makeCard(this, L"磁盘", wxColour(240, 160, 60), m_diskUsageLabel, m_diskInfoLabel), 1, wxEXPAND);
    gridSizer->Add(makeCard(this, L"GPU", wxColour(220, 80, 180), m_gpuUsageLabel, m_gpuInfoLabel), 1, wxEXPAND);
    gridSizer->Add(makeCard(this, L"网络下载", wxColour(60, 140, 220), m_networkDownLabel, nullptr), 1, wxEXPAND);
    gridSizer->Add(makeCard(this, L"网络上传", wxColour(180, 100, 220), m_networkUpLabel, nullptr), 1, wxEXPAND);

    mainSizer->Add(gridSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(10);

    // ── 三个性能图表（一行三列）──
    auto makeChartCard = [&](const wxString& label, PerformanceChart*& chart, const wxColour& lineColor) -> wxPanel* {
        auto* card = new wxPanel(this, wxID_ANY);
        card->SetBackgroundColour(colors.surface);
        auto* cs = new wxBoxSizer(wxVERTICAL);
        cs->AddSpacer(6);

        auto* titleRow = new wxBoxSizer(wxHORIZONTAL);
        auto* dot = new wxPanel(card, wxID_ANY);
        dot->SetBackgroundColour(lineColor);
        dot->SetMinSize(wxSize(8, 8));
        titleRow->Add(dot, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
        titleRow->AddSpacer(6);

        auto* labelCtrl = new wxStaticText(card, wxID_ANY, label);
        labelCtrl->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
        labelCtrl->SetForegroundColour(colors.textPrimary);
        titleRow->Add(labelCtrl, 1, wxALIGN_CENTER_VERTICAL);
        cs->Add(titleRow, 0, wxEXPAND);

        chart = new PerformanceChart(card, wxID_ANY, wxDefaultPosition, wxSize(-1, 110));
        chart->SetLineColor(lineColor);
        chart->SetFillColor(wxColour(lineColor.Red(), lineColor.Green(), lineColor.Blue(), 60));
        chart->SetMaxValue(100.0);
        chart->SetUnit(L"%");
        cs->Add(chart, 1, wxEXPAND | wxLEFT | wxRIGHT, 6);
        cs->AddSpacer(6);

        card->SetSizer(cs);
        return card;
    };

    auto* chartRow = new wxBoxSizer(wxHORIZONTAL);
    chartRow->Add(makeChartCard(L"CPU", m_cpuChart, colors.accent), 1, wxEXPAND | wxRIGHT, 6);
    chartRow->Add(makeChartCard(L"内存", m_memoryChart, wxColour(80, 180, 120)), 1, wxEXPAND | wxRIGHT, 6);
    chartRow->Add(makeChartCard(L"网络", m_networkChart, wxColour(60, 140, 220)), 1, wxEXPAND);
    mainSizer->Add(chartRow, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(10);

    // ── 硬件详情（可滚动）──
    auto* detailTitle = new wxStaticText(this, wxID_ANY, L"硬件详情");
    detailTitle->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    detailTitle->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(detailTitle, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(6);

    m_hardwareInfoScroller = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_hardwareInfoScroller->SetBackgroundColour(colors.background);
    m_hardwareInfoScroller->SetScrollRate(0, 1);

    m_hardwareInfoSizer = new wxBoxSizer(wxVERTICAL);

    auto* scrollerSizer = new wxBoxSizer(wxVERTICAL);
    scrollerSizer->Add(m_hardwareInfoSizer);
    m_hardwareInfoScroller->SetSizer(scrollerSizer);

    mainSizer->Add(m_hardwareInfoScroller, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(10);

    SetSizer(mainSizer);
}

void HardwareMonitorPanel::LoadHardwareSummary() {
    IceClean::Core::Analyzer::HardwareDetector detector;
    auto summary = detector.GetSummary();
    PopulateHardwareDetails(summary);
}

void HardwareMonitorPanel::PopulateHardwareDetails(const IceClean::Models::HardwareSummary& summary) {
    if (!m_hardwareInfoScroller) return;

    const auto& colors = ThemeManager::Instance().GetColors();

    for (auto child = m_hardwareInfoScroller->GetChildren().GetFirst(); child; child = child->GetNext()) {
        child->GetData()->Destroy();
    }
    m_hardwareInfoSizer->Clear(true);

    auto makeSectionCard = [&](const wxString& title, const wxColour& accent,
                               const std::vector<std::pair<wxString, wxString>>& rows) -> wxSizer* {
        auto* sectionSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* accentBar = new wxPanel(m_hardwareInfoScroller, wxID_ANY);
        accentBar->SetBackgroundColour(accent);
        accentBar->SetMinSize(wxSize(3, -1));
        sectionSizer->Add(accentBar, 0, wxEXPAND | wxRIGHT, 6);

        auto* card = new wxPanel(m_hardwareInfoScroller, wxID_ANY);
        card->SetBackgroundColour(colors.surface);
        auto* cs = new wxBoxSizer(wxVERTICAL);
        cs->AddSpacer(5);

        auto* titleLbl = new wxStaticText(card, wxID_ANY, title);
        titleLbl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
        titleLbl->SetForegroundColour(colors.textPrimary);
        cs->Add(titleLbl, 0, wxLEFT | wxRIGHT, 8);

        for (const auto& [label, value] : rows) {
            auto* row = new wxBoxSizer(wxHORIZONTAL);
            auto* lbl = new wxStaticText(card, wxID_ANY, label);
            lbl->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
            lbl->SetForegroundColour(colors.textSecondary);
            lbl->SetMinSize(wxSize(40, -1));
            row->Add(lbl, 0, wxALIGN_CENTER_VERTICAL);

            auto* val = new wxStaticText(card, wxID_ANY, value);
            val->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
            val->SetForegroundColour(colors.textPrimary);
            row->Add(val, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 3);

            cs->Add(row, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
            cs->AddSpacer(2);
        }

        cs->AddSpacer(4);
        card->SetSizer(cs);
        sectionSizer->Add(card, 1, wxEXPAND);
        return sectionSizer;
    };

    // 第一行：CPU + GPU + 内存
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);
        row->Add(makeSectionCard(L"处理器", colors.accent, {
            {L"名称:", summary.cpu.name},
            {L"规格:", wxString::Format(L"%dC/%dT", summary.cpu.coreCount, summary.cpu.logicalProcessorCount)},
            {L"频率:", FormatFrequency(summary.cpu.maxClockSpeedGHz)},
            {L"缓存:", wxString::Format(L"L2 %dKB  L3 %dKB", summary.cpu.l2CacheKB, summary.cpu.l3CacheKB)},
        }), 1, wxEXPAND | wxRIGHT, 4);

        if (!summary.gpus.empty()) {
            const auto& gpu = summary.gpus[0];
            row->Add(makeSectionCard(L"显卡", wxColour(220, 80, 180), {
                {L"名称:", gpu.name},
                {L"显存:", wxString::Format(L"%llu MB", static_cast<unsigned long long>(gpu.dedicatedMemoryMB))},
                {L"分辨率:", gpu.resolution},
                {L"驱动:", gpu.driverVersion},
            }), 1, wxEXPAND | wxRIGHT, 4);
        }

        row->Add(makeSectionCard(L"内存", wxColour(80, 180, 120), {
            {L"总容量:", wxString::Format(L"%.1f GB", summary.memory.totalPhysicalMB / 1024.0)},
            {L"可用:", wxString::Format(L"%.1f GB", summary.memory.availablePhysicalMB / 1024.0)},
            {L"插槽:", wxString::Format(L"%d/%d", summary.memory.memoryModuleCount, summary.memory.memorySlotCount)},
            {L"频率:", wxString::Format(L"%d MHz", summary.memory.memorySpeed)},
        }), 1, wxEXPAND);

        m_hardwareInfoSizer->Add(row, 0, wxEXPAND | wxBOTTOM, 4);
    }

    // 第二行：磁盘 + 网络 + 系统信息
    {
        auto* row = new wxBoxSizer(wxHORIZONTAL);

        std::vector<std::pair<wxString, wxString>> diskRows;
        for (const auto& disk : summary.disks) {
            wxString drive = (summary.disks.size() > 1) ? wxString::Format(L"%s:", disk.driveLetter) : wxString(L"系统");
            diskRows.push_back({drive, wxString::Format(L"%s | %s | %lluGB",
                disk.model.empty() ? wxString(L"本地磁盘") : disk.model,
                disk.isSSD ? L"SSD" : L"HDD",
                static_cast<unsigned long long>(disk.totalGB))});
        }
        row->Add(makeSectionCard(summary.disks.size() > 1 ? L"磁盘" : L"系统磁盘", wxColour(240, 160, 60), diskRows), 1, wxEXPAND | wxRIGHT, 4);

        std::vector<std::pair<wxString, wxString>> netRows;
        for (const auto& net : summary.networkAdapters) {
            netRows.push_back({net.connectionType, net.ipAddress.empty() ? L"无IP" : net.ipAddress});
        }
        row->Add(makeSectionCard(L"网络", wxColour(60, 140, 220), netRows), 1, wxEXPAND | wxRIGHT, 4);

        row->Add(makeSectionCard(L"系统信息", wxColour(100, 160, 160), {
            {L"主板:", summary.motherboard.product.empty() ? summary.motherboard.manufacturer : summary.motherboard.product},
            {L"BIOS:", summary.motherboard.biosVersion},
            {L"系统:", summary.osVersion},
            {L"运行:", summary.systemUptime},
        }), 1, wxEXPAND);

        m_hardwareInfoSizer->Add(row, 0, wxEXPAND | wxBOTTOM, 4);
    }

    m_hardwareInfoScroller->FitInside();
}

void HardwareMonitorPanel::StartMonitoring() {
    if (m_monitoring.exchange(true)) return;

    auto weakSelf = this;
    m_monitor->SetSnapshotCallback([weakSelf](const IceClean::Models::PerformanceSnapshot& snap) {
        if (weakSelf) {
            weakSelf->CallAfter([weakSelf, snap]() {
                weakSelf->OnSnapshot(snap);
            });
        }
    });
    m_monitor->StartMonitoring(1000);
}

void HardwareMonitorPanel::StopMonitoring() {
    if (m_monitoring.exchange(false)) {
        if (m_monitor) m_monitor->StopMonitoring();
    }
}

void HardwareMonitorPanel::OnSnapshot(const IceClean::Models::PerformanceSnapshot& snapshot) {
    UpdateCpuCard(snapshot.cpu);
    UpdateMemoryCard(snapshot.memory);
    UpdateDiskCard(snapshot.disks);
    UpdateNetworkCard(snapshot.network);
    UpdateGpuCard(snapshot.gpus);
    UpdateCpuChart(snapshot.cpu);
    UpdateMemoryChart(snapshot.memory);
    UpdateNetworkChart(snapshot.network);
}

void HardwareMonitorPanel::UpdateCpuCard(const IceClean::Models::CpuSnapshot& cpu) {
    if (m_cpuUsageLabel)
        m_cpuUsageLabel->SetLabel(wxString::Format(L"%.0f%%", cpu.usagePercent));
    if (m_cpuInfoLabel) {
        if (cpu.coreCount > 0 && cpu.logicalProcessorCount > 0) {
            m_cpuInfoLabel->SetLabel(wxString::Format(L"%dC/%dT  %s",
                cpu.coreCount, cpu.logicalProcessorCount, FormatFrequency(cpu.frequencyGHz)));
        } else {
            m_cpuInfoLabel->SetLabel(L"无法获取 CPU 信息");
        }
    }
}

void HardwareMonitorPanel::UpdateMemoryCard(const IceClean::Models::MemorySnapshot& memory) {
    if (m_memoryUsageLabel)
        m_memoryUsageLabel->SetLabel(wxString::Format(L"%.0f%%", memory.usagePercent));
    if (m_memoryInfoLabel) {
        m_memoryInfoLabel->SetLabel(wxString::Format(L"%s / %s",
            FormatBytes(memory.usedBytes), FormatBytes(memory.totalBytes)));
    }
}

void HardwareMonitorPanel::UpdateDiskCard(const std::vector<IceClean::Models::DiskSnapshot>& disks) {
    if (disks.empty()) return;
    const IceClean::Models::DiskSnapshot* cDisk = nullptr;
    for (const auto& d : disks) {
        if (d.driveLetter == L'C') { cDisk = &d; break; }
    }
    if (!cDisk) cDisk = &disks.front();

    if (m_diskUsageLabel)
        m_diskUsageLabel->SetLabel(wxString::Format(L"%.0f%%", cDisk->usagePercent));
    if (m_diskInfoLabel) {
        wxString label = cDisk->volumeLabel.empty() ? L"本地磁盘" : cDisk->volumeLabel;
        wxString driveStr(cDisk->driveLetter);
        m_diskInfoLabel->SetLabel(wxString::Format(L"%s %s: %s / %s",
            label, driveStr, FormatBytes(cDisk->usedBytes), FormatBytes(cDisk->totalBytes)));
    }
}

void HardwareMonitorPanel::UpdateNetworkCard(const IceClean::Models::NetworkSnapshot& network) {
    if (m_networkDownLabel)
        m_networkDownLabel->SetLabel(FormatSpeed(network.downloadSpeedBps));
    if (m_networkUpLabel)
        m_networkUpLabel->SetLabel(FormatSpeed(network.uploadSpeedBps));
}

void HardwareMonitorPanel::UpdateGpuCard(const std::vector<IceClean::Models::GpuSnapshot>& gpus) {
    if (gpus.empty()) return;
    const auto& gpu = gpus[0];
    if (m_gpuUsageLabel) {
        if (gpu.available && gpu.usagePercent >= 0) {
            m_gpuUsageLabel->SetLabel(wxString::Format(L"%.0f%%", gpu.usagePercent));
        } else {
            m_gpuUsageLabel->SetLabel(L"N/A");
        }
    }
    if (m_gpuInfoLabel) {
        if (gpu.available) {
            if (gpu.memoryUsagePercent >= 0) {
                m_gpuInfoLabel->SetLabel(wxString::Format(L"显存: %.0f%%", gpu.memoryUsagePercent));
            } else {
                m_gpuInfoLabel->SetLabel(L"正在检测...");
            }
        } else {
            m_gpuInfoLabel->SetLabel(L"无可用 GPU");
        }
    }
}

void HardwareMonitorPanel::UpdateCpuChart(const IceClean::Models::CpuSnapshot& cpu) {
    m_cpuHistory.push_back(cpu.usagePercent);
    while (m_cpuHistory.size() > kHistorySize) m_cpuHistory.pop_front();
    if (m_cpuChart) m_cpuChart->SetData(m_cpuHistory);
}

void HardwareMonitorPanel::UpdateMemoryChart(const IceClean::Models::MemorySnapshot& memory) {
    m_memoryHistory.push_back(memory.usagePercent);
    while (m_memoryHistory.size() > kHistorySize) m_memoryHistory.pop_front();
    if (m_memoryChart) m_memoryChart->SetData(m_memoryHistory);
}

void HardwareMonitorPanel::UpdateNetworkChart(const IceClean::Models::NetworkSnapshot& network) {
    double downKB = network.downloadSpeedBps / 1024.0;
    m_netDownHistory.push_back(downKB);
    while (m_netDownHistory.size() > kHistorySize) m_netDownHistory.pop_front();
    if (m_networkChart) m_networkChart->SetData(m_netDownHistory);
}

} // namespace IceClean::Gui
