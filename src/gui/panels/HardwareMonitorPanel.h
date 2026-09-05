#pragma once
#include <wx/wx.h>
#include <wx/scrolwin.h>
#include <deque>
#include <memory>
#include <atomic>
#include "core/analyzer/PerformanceMonitor.h"
#include "core/analyzer/HardwareDetector.h"
#include "gui/controls/PerformanceChart.h"

namespace IceClean::Gui {

class HardwareMonitorPanel : public wxPanel {
public:
    HardwareMonitorPanel(wxWindow* parent);
    ~HardwareMonitorPanel();

    void StartMonitoring();
    void StopMonitoring();

private:
    void OnSnapshot(const IceClean::Models::PerformanceSnapshot& snapshot);

    void LoadHardwareSummary();
    void BuildLayout();
    void PopulateHardwareDetails(const IceClean::Models::HardwareSummary& summary);

    void UpdateCpuCard(const IceClean::Models::CpuSnapshot& cpu);
    void UpdateMemoryCard(const IceClean::Models::MemorySnapshot& memory);
    void UpdateDiskCard(const std::vector<IceClean::Models::DiskSnapshot>& disks);
    void UpdateNetworkCard(const IceClean::Models::NetworkSnapshot& network);
    void UpdateGpuCard(const std::vector<IceClean::Models::GpuSnapshot>& gpus);

    void UpdateCpuChart(const IceClean::Models::CpuSnapshot& cpu);
    void UpdateMemoryChart(const IceClean::Models::MemorySnapshot& memory);
    void UpdateNetworkChart(const IceClean::Models::NetworkSnapshot& network);

    static wxString FormatBytes(uint64_t bytes);
    static wxString FormatSpeed(uint64_t bytesPerSec);
    static wxString FormatFrequency(double ghz);

    std::unique_ptr<IceClean::Core::Analyzer::PerformanceMonitor> m_monitor;
    std::atomic<bool> m_monitoring{false};

    // 概览卡片
    wxStaticText* m_cpuUsageLabel = nullptr;
    wxStaticText* m_cpuInfoLabel = nullptr;
    wxStaticText* m_memoryUsageLabel = nullptr;
    wxStaticText* m_memoryInfoLabel = nullptr;
    wxStaticText* m_diskUsageLabel = nullptr;
    wxStaticText* m_diskInfoLabel = nullptr;
    wxStaticText* m_networkDownLabel = nullptr;
    wxStaticText* m_networkUpLabel = nullptr;
    wxStaticText* m_gpuUsageLabel = nullptr;
    wxStaticText* m_gpuInfoLabel = nullptr;

    // 性能图表（紧凑）
    PerformanceChart* m_cpuChart = nullptr;
    PerformanceChart* m_memoryChart = nullptr;
    PerformanceChart* m_networkChart = nullptr;

    // 硬件详情容器
    wxScrolledWindow* m_hardwareInfoScroller = nullptr;
    wxBoxSizer* m_hardwareInfoSizer = nullptr;

    std::deque<double> m_cpuHistory;
    std::deque<double> m_memoryHistory;
    std::deque<double> m_netDownHistory;
    static constexpr size_t kHistorySize = 60;
};

} // namespace IceClean::Gui
