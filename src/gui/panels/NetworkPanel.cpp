#include "NetworkPanel.h"
#include "core/optimizer/NetworkOptimizer.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/controls/ThemeManager.h"
#include "utils/FormatUtil.h"
#include <thread>
#include <algorithm>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(NetworkPanel, wxPanel)
wxEND_EVENT_TABLE()

NetworkPanel::NetworkPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();
}

void NetworkPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"网络优化");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* tipLabel = new wxStaticText(this, wxID_ANY,
        L"优化网络设置，提升网络速度和稳定性。");
    tipLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    tipLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(tipLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 网络适配器
    auto* adapterLabel = new wxStaticText(this, wxID_ANY, L"网络适配器");
    adapterLabel->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    adapterLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(adapterLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    m_adapterListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 100),
                                        wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_adapterListCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_adapterListCtrl->AppendColumn(L"连接名称", wxLIST_FORMAT_LEFT, 150);
    m_adapterListCtrl->AppendColumn(L"IP地址", wxLIST_FORMAT_LEFT, 120);
    m_adapterListCtrl->AppendColumn(L"MAC地址", wxLIST_FORMAT_LEFT, 140);
    m_adapterListCtrl->AppendColumn(L"DNS", wxLIST_FORMAT_LEFT, 200);
    m_adapterListCtrl->AppendColumn(L"状态", wxLIST_FORMAT_CENTER, 60);
    mainSizer->Add(m_adapterListCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 网络优化项
    auto* optLabel = new wxStaticText(this, wxID_ANY, L"网络优化项");
    optLabel->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    optLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(optLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    m_optimizeListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 140),
                                         wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SIMPLE);
    m_optimizeListCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_optimizeListCtrl->AppendColumn(L"优化项", wxLIST_FORMAT_LEFT, 180);
    m_optimizeListCtrl->AppendColumn(L"说明", wxLIST_FORMAT_LEFT, 240);
    m_optimizeListCtrl->AppendColumn(L"当前值", wxLIST_FORMAT_CENTER, 100);
    m_optimizeListCtrl->AppendColumn(L"推荐值", wxLIST_FORMAT_CENTER, 100);
    m_optimizeListCtrl->AppendColumn(L"状态", wxLIST_FORMAT_CENTER, 70);
    mainSizer->Add(m_optimizeListCtrl, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // DNS设置
    auto* dnsLabel = new wxStaticText(this, wxID_ANY, L"DNS设置");
    dnsLabel->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    dnsLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(dnsLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* dnsSizer = new wxBoxSizer(wxHORIZONTAL);
    dnsSizer->AddSpacer(20);

    auto* dnsNameLabel = new wxStaticText(this, wxID_ANY, L"选择DNS:");
    dnsNameLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    dnsSizer->Add(dnsNameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_dnsChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(200, 28));
    m_dnsChoice->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    dnsSizer->Add(m_dnsChoice, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_applyDnsButton = new wxButton(this, wxID_ANY, L"应用DNS", wxDefaultPosition, wxSize(90, 30));
    m_applyDnsButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_applyDnsButton->Bind(wxEVT_BUTTON, &NetworkPanel::OnApplyDns, this);
    dnsSizer->Add(m_applyDnsButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);

    m_resetDnsButton = new wxButton(this, wxID_ANY, L"自动获取", wxDefaultPosition, wxSize(80, 30));
    m_resetDnsButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_resetDnsButton->Bind(wxEVT_BUTTON, &NetworkPanel::OnResetDns, this);
    dnsSizer->Add(m_resetDnsButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_flushDnsButton = new wxButton(this, wxID_ANY, L"刷新DNS缓存", wxDefaultPosition, wxSize(110, 30));
    m_flushDnsButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_flushDnsButton->Bind(wxEVT_BUTTON, &NetworkPanel::OnFlushDns, this);
    dnsSizer->Add(m_flushDnsButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    m_pingResultLabel = new wxStaticText(this, wxID_ANY, L"");
    m_pingResultLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_pingResultLabel->SetForegroundColour(colors.textSecondary);
    dnsSizer->Add(m_pingResultLabel, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(dnsSizer, 0, wxEXPAND);
    mainSizer->AddSpacer(8);

    // 底部按钮栏
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    m_applyOptButton = new wxButton(this, wxID_ANY, L"一键优化", wxDefaultPosition, wxSize(100, 36));
    m_applyOptButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_applyOptButton->SetBackgroundColour(colors.accent);
    m_applyOptButton->SetForegroundColour(*wxWHITE);
    m_applyOptButton->Bind(wxEVT_BUTTON, &NetworkPanel::OnApplyOptimize, this);
    bottomSizer->Add(m_applyOptButton, 0, wxRIGHT, 8);

    auto* pingButton = new wxButton(this, wxID_ANY, L"Ping测试", wxDefaultPosition, wxSize(90, 36));
    pingButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    pingButton->Bind(wxEVT_BUTTON, &NetworkPanel::OnPingTest, this);
    bottomSizer->Add(pingButton, 0, wxRIGHT, 8);

    m_refreshButton = new wxButton(this, wxID_ANY, L"刷新", wxDefaultPosition, wxSize(70, 36));
    m_refreshButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_refreshButton->Bind(wxEVT_BUTTON, &NetworkPanel::OnRefresh, this);
    bottomSizer->Add(m_refreshButton, 0, wxRIGHT, 8);

    bottomSizer->AddStretchSpacer();

    m_statusLabel = new wxStaticText(this, wxID_ANY, L"");
    m_statusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_statusLabel->SetForegroundColour(colors.textSecondary);
    bottomSizer->Add(m_statusLabel, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);

    CallAfter([this]() {
        LoadAdapters();
        LoadOptimizeItems();
        LoadDnsConfigs();
    });
}

void NetworkPanel::LoadAdapters() {
    m_adapterListCtrl->DeleteAllItems();

    IceClean::Core::Optimizer::NetworkOptimizer netOpt;
    m_adapters = netOpt.GetNetworkAdapters();

    for (int i = 0; i < static_cast<int>(m_adapters.size()); ++i) {
        const auto& adapter = m_adapters[i];
        long idx = m_adapterListCtrl->InsertItem(i, adapter.connectionName);
        m_adapterListCtrl->SetItem(idx, 1, adapter.ipAddress);
        m_adapterListCtrl->SetItem(idx, 2, adapter.macAddress);
        m_adapterListCtrl->SetItem(idx, 3, adapter.dnsServers);
        m_adapterListCtrl->SetItem(idx, 4, adapter.isEnabled ? L"已连接" : L"断开");
    }
}

void NetworkPanel::LoadOptimizeItems() {
    m_optimizeListCtrl->DeleteAllItems();

    IceClean::Core::Optimizer::NetworkOptimizer netOpt;
    m_optimizeItems = netOpt.GetOptimizeItems();

    for (int i = 0; i < static_cast<int>(m_optimizeItems.size()); ++i) {
        const auto& item = m_optimizeItems[i];
        long idx = m_optimizeListCtrl->InsertItem(i, item.name);
        m_optimizeListCtrl->SetItem(idx, 1, item.description);
        m_optimizeListCtrl->SetItem(idx, 2, item.currentValue);
        m_optimizeListCtrl->SetItem(idx, 3, item.recommendedValue);
        m_optimizeListCtrl->SetItem(idx, 4, item.needsOptimize ? L"待优化" : L"已优化");
    }

    UpdateOptimizeStatus();
}

void NetworkPanel::LoadDnsConfigs() {
    m_dnsChoice->Clear();
    m_dnsChoice->Append(L"自动获取(DHCP)");

    IceClean::Core::Optimizer::NetworkOptimizer netOpt;
    m_dnsConfigs = netOpt.GetRecommendedDnsConfigs();

    for (const auto& config : m_dnsConfigs) {
        m_dnsChoice->Append(wxString::Format(L"%s (%s / %s)",
            config.providerName.c_str(), config.preferredDns.c_str(), config.alternateDns.c_str()));
    }

    m_dnsChoice->SetSelection(0);
}

void NetworkPanel::UpdateOptimizeStatus() {
    int needOpt = 0;
    for (const auto& item : m_optimizeItems) {
        if (item.needsOptimize) needOpt++;
    }
    m_statusLabel->SetLabelText(
        wxString::Format(L"共 %d 项，%d 项待优化",
            static_cast<int>(m_optimizeItems.size()), needOpt));
}

void NetworkPanel::OnRefresh(wxCommandEvent& event) {
    LoadAdapters();
    LoadOptimizeItems();
    m_statusLabel->SetLabelText(L"已刷新。");
}

void NetworkPanel::OnApplyOptimize(wxCommandEvent& event) {
    int needOpt = 0;
    for (const auto& item : m_optimizeItems) {
        if (item.needsOptimize) needOpt++;
    }

    if (needOpt == 0) {
        wxMessageBox(L"所有网络项均已优化，无需操作。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        return;
    }

    ConfirmDialog dlg(this, L"网络优化",
        wxString::Format(L"确定要应用 %d 项网络优化吗？\n\n修改注册表网络设置可能需要重启网络适配器。", needOpt),
        ConfirmDialog::DangerLevel::Caution, L"优化", L"取消");

    if (dlg.ShowModal() != wxID_OK) return;

    IceClean::Core::Optimizer::NetworkOptimizer netOpt;
    int applied = netOpt.ApplyAllOptimizes(m_optimizeItems);

    LoadOptimizeItems();
    wxString msg = wxString::Format(L"已应用 %d 项网络优化。", applied);
    wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
}

void NetworkPanel::OnApplyDns(wxCommandEvent& event) {
    int sel = m_dnsChoice->GetSelection();
    if (sel <= 0) {
        OnResetDns(event);
        return;
    }

    int dnsIdx = sel - 1;
    if (dnsIdx < 0 || dnsIdx >= static_cast<int>(m_dnsConfigs.size())) return;

    const auto& dnsConfig = m_dnsConfigs[dnsIdx];

    std::wstring adapterName;
    for (const auto& adapter : m_adapters) {
        if (adapter.isEnabled && !adapter.isVirtual && !adapter.connectionName.empty()) {
            adapterName = adapter.connectionName;
            break;
        }
    }

    if (adapterName.empty()) {
        wxMessageBox(L"未找到活跃的网络适配器。", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    IceClean::Core::Optimizer::NetworkOptimizer netOpt;
    if (netOpt.SetDnsServers(adapterName, dnsConfig.preferredDns, dnsConfig.alternateDns)) {
        wxString msg = wxString::Format(L"已将 %s 的DNS设置为 %s。", adapterName.c_str(), dnsConfig.providerName.c_str());
        wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
        LoadAdapters();
    } else {
        wxMessageBox(L"设置DNS失败，请尝试以管理员权限运行。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void NetworkPanel::OnResetDns(wxCommandEvent& event) {
    std::wstring adapterName;
    for (const auto& adapter : m_adapters) {
        if (adapter.isEnabled && !adapter.isVirtual && !adapter.connectionName.empty()) {
            adapterName = adapter.connectionName;
            break;
        }
    }

    if (adapterName.empty()) return;

    IceClean::Core::Optimizer::NetworkOptimizer netOpt;
    if (netOpt.SetAutoDns(adapterName)) {
        wxMessageBox(L"已恢复为自动获取DNS。", L"IceClean", wxOK | wxICON_INFORMATION, this);
        LoadAdapters();
    } else {
        wxMessageBox(L"恢复DNS设置失败。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void NetworkPanel::OnFlushDns(wxCommandEvent& event) {
    IceClean::Core::Optimizer::NetworkOptimizer netOpt;
    if (netOpt.FlushDnsCache()) {
        wxMessageBox(L"DNS缓存已刷新。", L"IceClean", wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(L"刷新DNS缓存失败。", L"IceClean", wxOK | wxICON_WARNING, this);
    }
}

void NetworkPanel::OnPingTest(wxCommandEvent& event) {
    m_pingResultLabel->SetLabelText(L"正在测试...");

    std::thread([this]() {
        IceClean::Core::Optimizer::NetworkOptimizer netOpt;
        int ping1 = netOpt.PingTest(L"www.baidu.com");
        int ping2 = netOpt.PingTest(L"www.qq.com");

        CallAfter([this, ping1, ping2]() {
            wxString result;
            if (ping1 >= 0) {
                result += wxString::Format(L"百度: %dms", ping1);
            } else {
                result += L"百度: 超时";
            }
            result += L"  |  ";
            if (ping2 >= 0) {
                result += wxString::Format(L"腾讯: %dms", ping2);
            } else {
                result += L"腾讯: 超时";
            }
            m_pingResultLabel->SetLabelText(result);
        });
    }).detach();
}

void NetworkPanel::OnOptimizeChecked(wxListEvent& event) {
    // No-op for now
}

} // namespace IceClean::Gui
