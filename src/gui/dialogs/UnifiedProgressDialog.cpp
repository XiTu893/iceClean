#include "UnifiedProgressDialog.h"
#include "gui/controls/ThemeManager.h"
#include <algorithm>

namespace IceClean::Gui {

UnifiedProgressDialog::UnifiedProgressDialog(wxWindow* parent, const wxString& title)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(460, 320),
               wxDEFAULT_DIALOG_STYLE & ~wxCLOSE_BOX & ~wxMINIMIZE_BOX & ~wxMAXIMIZE_BOX | wxCENTRE)
{
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        if (!m_finished && !m_cancelled) {
            // 正在运行时关闭窗口 = 取消操作
            m_cancelled = true;
        }
        Destroy();
    });

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(16);

    // ── 阶段名称 ──
    m_stageLabel = new wxStaticText(this, wxID_ANY, L"准备中...");
    m_stageLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    mainSizer->Add(m_stageLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(6);

    // ── 主进度条 + 百分比 ──
    auto* progressRow = new wxBoxSizer(wxHORIZONTAL);
    m_mainGauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(340, 22));
    m_mainGauge->SetValue(0);
    progressRow->Add(m_mainGauge, 1, wxEXPAND);

    m_percentLabel = new wxStaticText(this, wxID_ANY, L"0%");
    m_percentLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                   false, L"微软雅黑"));
    progressRow->AddSpacer(8);
    progressRow->Add(m_percentLabel, 0, wxALIGN_CENTER_VERTICAL);
    mainSizer->Add(progressRow, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(6);

    // ── 详细描述 ──
    m_detailLabel = new wxStaticText(this, wxID_ANY, L"");
    m_detailLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                  false, L"微软雅黑"));
    m_detailLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
    mainSizer->Add(m_detailLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(10);

    // ── 子进度条 ──
    m_subGauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(420, 12),
                             wxGA_SMOOTH);
    m_subGauge->SetValue(0);
    mainSizer->Add(m_subGauge, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(6);

    // ── 统计信息行 ──
    auto* statsGrid = new wxFlexGridSizer(2, 4, 8, 16);
    statsGrid->AddGrowableCol(1, 1);

    // 项目数
    auto* itemsTitle = new wxStaticText(this, wxID_ANY, L"已处理:");
    itemsTitle->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    m_itemsLabel = new wxStaticText(this, wxID_ANY, L"0 / 0");
    m_itemsLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    m_itemsLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
    statsGrid->Add(itemsTitle, 0, wxALIGN_LEFT);
    statsGrid->Add(m_itemsLabel, 0, wxALIGN_LEFT);

    // 大小
    auto* sizeTitle = new wxStaticText(this, wxID_ANY, L"已释放:");
    sizeTitle->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    m_sizeLabel = new wxStaticText(this, wxID_ANY, L"0 B");
    m_sizeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    m_sizeLabel->SetForegroundColour(ThemeManager::Instance().GetColors().success);
    statsGrid->Add(sizeTitle, 0, wxALIGN_LEFT);
    statsGrid->Add(m_sizeLabel, 0, wxALIGN_LEFT);

    // 速度
    auto* speedTitle = new wxStaticText(this, wxID_ANY, L"速度:");
    speedTitle->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    m_speedLabel = new wxStaticText(this, wxID_ANY, L"--");
    m_speedLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    m_speedLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
    statsGrid->Add(speedTitle, 0, wxALIGN_LEFT);
    statsGrid->Add(m_speedLabel, 0, wxALIGN_LEFT);

    // 剩余时间
    auto* etaTitle = new wxStaticText(this, wxID_ANY, L"剩余时间:");
    etaTitle->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
    m_etaLabel = new wxStaticText(this, wxID_ANY, L"--");
    m_etaLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    m_etaLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
    statsGrid->Add(etaTitle, 0, wxALIGN_LEFT);
    statsGrid->Add(m_etaLabel, 0, wxALIGN_LEFT);

    mainSizer->Add(statsGrid, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(16);

    // ── 取消按钮 ──
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    m_cancelButton = new wxButton(this, wxID_CANCEL, L"取消");
    m_cancelButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_cancelButton->Bind(wxEVT_BUTTON, &UnifiedProgressDialog::OnCancel, this);
    buttonSizer->Add(m_cancelButton, 0, wxRIGHT, 20);
    mainSizer->Add(buttonSizer, 0, wxEXPAND);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);
    Layout();

    // ── 刷新定时器（每 300ms 刷新显示） ──
    m_refreshTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &UnifiedProgressDialog::OnTimerTick, this, m_refreshTimer->GetId());
    m_refreshTimer->Start(300);
}

void UnifiedProgressDialog::UpdateData(const UnifiedProgressData& data) {
    m_lastData = data;

    m_mainGauge->SetValue(std::clamp(data.percent, 0, 100));
    m_percentLabel->SetLabel(wxString::Format(L"%d%%", data.percent));

    if (!data.stage.empty()) {
        m_stageLabel->SetLabel(data.stage);
    }
    if (!data.detail.empty()) {
        wxString display = data.detail;
        if (display.length() > 60) {
            display = display.Left(28) + L"..." + display.Right(28);
        }
        m_detailLabel->SetLabel(display);
    }

    m_subGauge->SetValue(std::clamp(data.subPercent, 0, 100));

    // 统计信息
    FormatSize(data.processedBytes, m_sizeLabel->GetLabel());
    m_sizeLabel->GetParent()->Layout();

    if (data.totalItems > 0) {
        m_itemsLabel->SetLabel(wxString::Format(L"%d / %d", data.processedItems, data.totalItems));
    }

    if (data.speedBytesPerSec > 0.0) {
        wxString speedStr;
        FormatSpeed(static_cast<uint64_t>(data.speedBytesPerSec), speedStr);
        m_speedLabel->SetLabel(speedStr);
    }

    if (data.speedBytesPerSec > 0.0 && data.totalBytes > data.processedBytes) {
        uint64_t remaining = data.totalBytes - data.processedBytes;
        double remainingSec = remaining / data.speedBytesPerSec;
        if (remainingSec < 86400.0) {
            wxString etaStr;
            FormatEta(remaining, static_cast<uint64_t>(data.speedBytesPerSec), etaStr);
            m_etaLabel->SetLabel(etaStr);
        }
    }
}

void UnifiedProgressDialog::SetFinished(bool success, const wxString& summary) {
    m_finished = true;
    m_mainGauge->SetValue(100);
    m_percentLabel->SetLabel(L"100%");

    if (success) {
        m_stageLabel->SetForegroundColour(ThemeManager::Instance().GetColors().success);
        m_stageLabel->SetLabel(L"✅ " + summary);
    } else {
        m_stageLabel->SetForegroundColour(ThemeManager::Instance().GetColors().danger);
        m_stageLabel->SetLabel(L"❌ " + summary);
    }

    m_detailLabel->SetLabel(L"");
    m_subGauge->SetValue(0);
    m_speedLabel->SetLabel(L"--");
    m_etaLabel->SetLabel(L"--");

    m_cancelButton->SetLabel(L"关闭");
    m_cancelButton->Unbind(wxEVT_BUTTON, &UnifiedProgressDialog::OnCancel, this);
    m_cancelButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        Close();
    });

    if (m_refreshTimer) {
        m_refreshTimer->Stop();
    }
}

void UnifiedProgressDialog::OnCancel(wxCommandEvent& /*event*/) {
    wxMessageDialog confirm(this, L"确定要取消当前操作吗？\n已完成的步骤不会回滚。",
                            L"确认取消", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    if (confirm.ShowModal() == wxID_YES) {
        m_cancelled = true;
        m_cancelButton->Disable();
        m_stageLabel->SetLabel(L"正在取消...");
    }
}

void UnifiedProgressDialog::OnTimerTick(wxTimerEvent& /*event*/) {
    // 定时刷新可以在这里做一些自适应显示更新
}

void UnifiedProgressDialog::FormatSpeed(uint64_t bytesPerSec, wxString& out) const {
    if (bytesPerSec >= 1024ULL * 1024 * 1024) {
        double gb = static_cast<double>(bytesPerSec) / (1024.0 * 1024.0 * 1024.0);
        out = wxString::Format(L"%.2f GB/s", gb);
    } else if (bytesPerSec >= 1024 * 1024) {
        double mb = static_cast<double>(bytesPerSec) / (1024.0 * 1024.0);
        out = wxString::Format(L"%.1f MB/s", mb);
    } else if (bytesPerSec >= 1024) {
        double kb = static_cast<double>(bytesPerSec) / 1024.0;
        out = wxString::Format(L"%.0f KB/s", kb);
    } else {
        out = wxString::Format(L"%llu B/s", bytesPerSec);
    }
}

void UnifiedProgressDialog::FormatEta(uint64_t remainingBytes, double speed, wxString& out) const {
    if (speed <= 0.0) { out = L"--"; return; }
    double seconds = static_cast<double>(remainingBytes) / speed;

    if (seconds < 60.0) {
        out = wxString::Format(L"%.0f 秒", seconds);
    } else if (seconds < 3600.0) {
        out = wxString::Format(L"%.0f 分 %.0f 秒", seconds / 60.0, fmod(seconds, 60.0));
    } else if (seconds < 86400.0) {
        double hours = seconds / 3600.0;
        double mins = fmod(seconds, 3600.0) / 60.0;
        out = wxString::Format(L"%.0f 时 %.0f 分", hours, mins);
    } else {
        out = L"> 1 天";
    }
}

void UnifiedProgressDialog::FormatSize(uint64_t bytes, wxString& out) const {
    if (bytes >= 1024ULL * 1024 * 1024) {
        double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
        out = wxString::Format(L"%.2f GB", gb);
    } else if (bytes >= 1024 * 1024) {
        double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
        out = wxString::Format(L"%.1f MB", mb);
    } else if (bytes >= 1024) {
        double kb = static_cast<double>(bytes) / 1024.0;
        out = wxString::Format(L"%.0f KB", kb);
    } else {
        out = wxString::Format(L"%llu B", bytes);
    }
}

} // namespace IceClean::Gui
