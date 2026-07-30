#include "HardwareInfoPanel.h"
#include "gui/controls/ThemeManager.h"

namespace IceClean::Gui {

HardwareInfoPanel::HardwareInfoPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.background);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"硬件信息");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* descLabel = new wxStaticText(this, wxID_ANY, L"当前计算机的硬件配置和系统信息。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(16);

    // ── 可滚动容器 ──
    auto* scrollWin = new wxScrolledWindow(this, wxID_ANY);
    scrollWin->SetBackgroundColour(colors.background);
    scrollWin->SetScrollRate(0, 10);
    auto* cardSizer = new wxBoxSizer(wxVERTICAL);
    cardSizer->AddSpacer(4);

    // ── 辅助函数：创建信息卡片 ──
    auto createCard = [&](const wxString& icon, const wxString& title) -> wxPanel* {
        auto* card = new wxPanel(scrollWin, wxID_ANY);
        card->SetBackgroundColour(colors.surface);
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        sizer->AddSpacer(12);

        auto* headerRow = new wxBoxSizer(wxHORIZONTAL);
        auto* iconText = new wxStaticText(card, wxID_ANY, icon);
        iconText->SetFont(wxFont(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        headerRow->Add(iconText, 0, wxLEFT, 16);
        headerRow->AddSpacer(12);

        auto* nameLabel = new wxStaticText(card, wxID_ANY, title);
        nameLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
        headerRow->Add(nameLabel, 1, wxALIGN_CENTER_VERTICAL);
        sizer->Add(headerRow, 0, wxEXPAND);
        sizer->AddSpacer(8);

        auto* infoSizer = new wxBoxSizer(wxVERTICAL);
        infoSizer->AddSpacer(4);
        sizer->Add(infoSizer, 0, wxLEFT | wxRIGHT, 48);
        sizer->AddSpacer(12);

        card->SetSizer(sizer);
        return card;
    };

    auto addRow = [&](wxSizer* sizer, const wxString& label, wxString initialValue) -> wxStaticText* {
        auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* labelText = new wxStaticText(scrollWin, wxID_ANY, label);
        labelText->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                  false, L"微软雅黑"));
        labelText->SetForegroundColour(colors.textSecondary);
        rowSizer->Add(labelText, 0, wxALIGN_TOP | wxRIGHT, 8);

        auto* valueText = new wxStaticText(scrollWin, wxID_ANY, initialValue);
        valueText->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
        valueText->SetForegroundColour(colors.textPrimary);
        rowSizer->Add(valueText, 1, wxEXPAND | wxLEFT, 8);

        // Find the parent card's infoSizer
        auto* items = sizer->GetChildren();
        if (!items->empty()) {
            auto* lastItem = items->back();
            if (lastItem->IsSizer()) {
                lastItem->GetSizer()->Add(rowSizer, 0, wxEXPAND | wxBOTTOM, 6);
            }
        }

        return valueText;
    };

    // ── CPU 卡片 ──
    {
        auto* card = createCard(L"⚡", L"中央处理器 (CPU)");
        m_cpuInfo = new wxStaticText(card, wxID_ANY, L"检测中...");
        m_cpuInfo->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
        m_cpuInfo->SetForegroundColour(colors.textPrimary);

        auto* sizer = card->GetSizer();
        auto* infoSizer = new wxBoxSizer(wxVERTICAL);
        infoSizer->Add(m_cpuInfo, 0, wxBOTTOM, 6);
        sizer->Add(infoSizer, 0, wxLEFT | wxRIGHT, 48);
        sizer->AddSpacer(12);

        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    // ── GPU 卡片 ──
    {
        auto* card = createCard(L"🎮", L"图形处理器 (GPU)");
        m_gpuInfo = new wxStaticText(card, wxID_ANY, L"检测中...");
        m_gpuInfo->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
        m_gpuInfo->SetForegroundColour(colors.textPrimary);

        auto* sizer = card->GetSizer();
        auto* infoSizer = new wxBoxSizer(wxVERTICAL);
        infoSizer->Add(m_gpuInfo, 0, wxBOTTOM, 6);
        sizer->Add(infoSizer, 0, wxLEFT | wxRIGHT, 48);
        sizer->AddSpacer(12);

        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    // ── 内存卡片 ──
    {
        auto* card = createCard(L"💾", L"内存 (RAM)");
        m_memoryInfo = new wxStaticText(card, wxID_ANY, L"检测中...");
        m_memoryInfo->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                     false, L"微软雅黑"));
        m_memoryInfo->SetForegroundColour(colors.textPrimary);

        auto* sizer = card->GetSizer();
        auto* infoSizer = new wxBoxSizer(wxVERTICAL);
        infoSizer->Add(m_memoryInfo, 0, wxBOTTOM, 6);
        sizer->Add(infoSizer, 0, wxLEFT | wxRIGHT, 48);
        sizer->AddSpacer(12);

        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    // ── 磁盘卡片 ──
    {
        auto* card = createCard(L"💿", L"磁盘 (Storage)");
        m_diskInfo = new wxStaticText(card, wxID_ANY, L"检测中...");
        m_diskInfo->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                   false, L"微软雅黑"));
        m_diskInfo->SetForegroundColour(colors.textPrimary);

        auto* sizer = card->GetSizer();
        auto* infoSizer = new wxBoxSizer(wxVERTICAL);
        infoSizer->Add(m_diskInfo, 0, wxBOTTOM, 6);
        sizer->Add(infoSizer, 0, wxLEFT | wxRIGHT, 48);
        sizer->AddSpacer(12);

        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    // ── 主板卡片 ──
    {
        auto* card = createCard(L"🔌", L"主板 (Motherboard)");
        m_motherboardInfo = new wxStaticText(card, wxID_ANY, L"检测中...");
        m_motherboardInfo->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                          false, L"微软雅黑"));
        m_motherboardInfo->SetForegroundColour(colors.textPrimary);

        auto* sizer = card->GetSizer();
        auto* infoSizer = new wxBoxSizer(wxVERTICAL);
        infoSizer->Add(m_motherboardInfo, 0, wxBOTTOM, 6);
        sizer->Add(infoSizer, 0, wxLEFT | wxRIGHT, 48);
        sizer->AddSpacer(12);

        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    // ── 操作系统卡片 ──
    {
        auto* card = createCard(L"🖥️", L"操作系统");
        m_osInfo = new wxStaticText(card, wxID_ANY, L"检测中...");
        m_osInfo->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
        m_osInfo->SetForegroundColour(colors.textPrimary);

        auto* sizer = card->GetSizer();
        auto* infoSizer = new wxBoxSizer(wxVERTICAL);
        infoSizer->Add(m_osInfo, 0, wxBOTTOM, 6);
        sizer->Add(infoSizer, 0, wxLEFT | wxRIGHT, 48);
        sizer->AddSpacer(12);

        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    // ── 运行时间卡片 ──
    {
        auto* card = createCard(L"⏱️", L"系统运行时间");
        m_uptimeInfo = new wxStaticText(card, wxID_ANY, L"检测中...");
        m_uptimeInfo->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                     false, L"微软雅黑"));
        m_uptimeInfo->SetForegroundColour(colors.textPrimary);

        auto* sizer = card->GetSizer();
        auto* infoSizer = new wxBoxSizer(wxVERTICAL);
        infoSizer->Add(m_uptimeInfo, 0, wxBOTTOM, 6);
        sizer->Add(infoSizer, 0, wxLEFT | wxRIGHT, 48);
        sizer->AddSpacer(12);

        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    cardSizer->AddSpacer(12);
    scrollWin->SetSizer(cardSizer);
    mainSizer->Add(scrollWin, 1, wxEXPAND);

    SetSizer(mainSizer);

    // 延迟加载硬件信息
    CallAfter([this]() { LoadHardwareInfo(); });
}

void HardwareInfoPanel::LoadHardwareInfo() {
    std::thread([this]() {
        auto summary = m_detector.GetSummary();
        auto cpuInfo = m_detector.GetCpuInfo();
        auto gpuInfo = m_detector.GetGpuInfo();
        auto memoryInfo = m_detector.GetMemoryInfo();
        auto diskInfo = m_detector.GetDiskInfo();
        auto motherboardInfo = m_detector.GetMotherboardInfo();
        std::wstring osVersion, osBuild;
        m_detector.GetOsInfo(osVersion, osBuild);
        auto uptime = m_detector.GetSystemUptime();

        CallAfter([this, cpuInfo, gpuInfo, memoryInfo, diskInfo,
                         motherboardInfo, osVersion, osBuild, uptime]() {
            // CPU
            wxString cpuStr = wxString::Format(L"%s\n%d 核 %d 逻辑处理器 | %.0f%% 利用率",
                cpuInfo.name, cpuInfo.physicalCores, cpuInfo.logicalCores,
                cpuInfo.usagePercent);
            m_cpuInfo->SetLabel(cpuStr);

            // GPU
            wxString gpuStr;
            for (const auto& gpu : gpuInfo) {
                if (!gpuStr.empty()) gpuStr += L"\n";
                gpuStr += wxString::Format(L"%s | %s | %s",
                    gpu.name, gpu.vendor, gpu.dedicatedMemory);
            }
            m_gpuInfo->SetLabel(gpuStr);

            // Memory
            wxString memStr = wxString::Format(L"总计: %s | 已用: %s | 可用: %s\n利用率: %.0f%%",
                memoryInfo.totalFormatted, memoryInfo.usedFormatted,
                memoryInfo.availableFormatted, memoryInfo.usagePercent);
            m_memoryInfo->SetLabel(memStr);

            // Disk
            wxString diskStr;
            for (const auto& disk : diskInfo) {
                if (!diskStr.empty()) diskStr += L"\n";
                diskStr += wxString::Format(L"%s (%s) | %s / %s | %s | 健康度: %.0f%%",
                    disk.driveLetter, disk.fileSystem,
                    disk.usedFormatted, disk.totalFormatted,
                    disk.isSSD ? L"SSD" : L"HDD", disk.healthPercent);
            }
            m_diskInfo->SetLabel(diskStr);

            // Motherboard
            wxString mbStr = wxString::Format(L"%s %s\nBIOS: %s (%s)",
                motherboardInfo.manufacturer, motherboardInfo.product,
                motherboardInfo.biosVersion, motherboardInfo.biosDate);
            m_motherboardInfo->SetLabel(mbStr);

            // OS
            wxString osStr = wxString::Format(L"Windows %s\n版本号: %s", osVersion, osBuild);
            m_osInfo->SetLabel(osStr);

            // Uptime
            m_uptimeInfo->SetLabel(uptime);
        });
    }).detach();
}

} // namespace IceClean::Gui
