#include "ProcessNetPanel.h"
#include "gui/controls/ThemeManager.h"
#include <set>
#include <map>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(ProcessNetPanel, wxPanel)
    EVT_LIST_COL_CLICK(wxID_ANY, ProcessNetPanel::OnColumnClick)
wxEND_EVENT_TABLE()

ProcessNetPanel::ProcessNetPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    const auto& colors = ThemeManager::Instance().GetColors();

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"应用流量");
    titleLabel->SetFont(wxFont(13, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* descLabel = new wxStaticText(this, wxID_ANY,
        L"按进程统计网络流量，包含实时速率与启动以来累计流量。点击列标题可排序。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    m_listCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 400),
                                 wxLC_REPORT | wxLC_SINGLE_SEL);
    m_listCtrl->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    m_listCtrl->AppendColumn(L"进程", wxLIST_FORMAT_LEFT, 200);
    m_listCtrl->AppendColumn(L"PID", wxLIST_FORMAT_CENTER, 60);
    m_listCtrl->AppendColumn(L"下载速率", wxLIST_FORMAT_RIGHT, 90);
    m_listCtrl->AppendColumn(L"上传速率", wxLIST_FORMAT_RIGHT, 90);
    m_listCtrl->AppendColumn(L"累计下载", wxLIST_FORMAT_RIGHT, 110);
    m_listCtrl->AppendColumn(L"累计上传", wxLIST_FORMAT_RIGHT, 110);
    m_listCtrl->Bind(wxEVT_LIST_COL_CLICK, &ProcessNetPanel::OnColumnClick, this);
    mainSizer->Add(m_listCtrl, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);
}

ProcessNetPanel::~ProcessNetPanel() {
    StopMonitoring();
}

void ProcessNetPanel::StartMonitoring() {
    if (m_monitor) return;
    m_monitor = IceClean::Core::Analyzer::ProcessNetworkMonitor::Create();
    auto weakSelf = this;
    m_monitor->SetSnapshotCallback([weakSelf](const std::vector<IceClean::Core::Analyzer::ProcessNetworkStats>& stats) {
        if (weakSelf) {
            weakSelf->CallAfter([weakSelf, stats]() {
                weakSelf->OnSnapshot(stats);
            });
        }
    });
    m_monitor->StartMonitoring(2000);
}

void ProcessNetPanel::StopMonitoring() {
    if (m_monitor) {
        m_monitor->StopMonitoring();
        m_monitor.reset();
    }
}

void ProcessNetPanel::OnSnapshot(const std::vector<IceClean::Core::Analyzer::ProcessNetworkStats>& stats) {
    m_data = stats;
    SortAndPopulate();
}

void ProcessNetPanel::OnColumnClick(wxListEvent& event) {
    SortColumn newCol = static_cast<SortColumn>(event.GetColumn());
    if (newCol == m_sortCol) {
        m_sortAsc = !m_sortAsc;
    } else {
        m_sortCol = newCol;
        m_sortAsc = false;
    }
    SortAndPopulate();
}

void ProcessNetPanel::SortAndPopulate() {
    if (!m_listCtrl) return;

    auto sortFn = [this](const IceClean::Core::Analyzer::ProcessNetworkStats& a,
                           const IceClean::Core::Analyzer::ProcessNetworkStats& b) {
        bool result = false;
        switch (m_sortCol) {
            case ColProcess:   result = a.processName < b.processName; break;
            case ColPid:       result = a.pid < b.pid; break;
            case ColDownSpeed: result = a.currentDownBps < b.currentDownBps; break;
            case ColUpSpeed:   result = a.currentUpBps < b.currentUpBps; break;
            case ColTotalDown: result = a.sessionTotalDown < b.sessionTotalDown; break;
            case ColTotalUp:   result = a.sessionTotalUp < b.sessionTotalUp; break;
            default: result = false;
        }
        return m_sortAsc ? !result : result;
    };

    std::vector<IceClean::Core::Analyzer::ProcessNetworkStats> sorted = m_data;
    std::sort(sorted.begin(), sorted.end(), sortFn);

    // 使用数据本身作 key（PID+name），删除项时使用批量更新策略
    // 收集当前显示的 PID 集合和目标 PID 集合
    std::set<uint32_t> currentPids;
    for (int i = 0; i < m_listCtrl->GetItemCount(); ++i) {
        wxString pidStr = m_listCtrl->GetItemText(i, 1);
        long pid = 0;
        pidStr.ToLong(&pid);
        currentPids.insert(static_cast<uint32_t>(pid));
    }

    std::set<uint32_t> targetPids;
    std::map<uint32_t, const IceClean::Core::Analyzer::ProcessNetworkStats*> pidToStats;
    for (const auto& s : sorted) {
        targetPids.insert(s.pid);
        pidToStats[s.pid] = &s;
    }

    m_listCtrl->Freeze();

    // 就地更新：先按顺序重置所有现有行（不删除），剩余的 InsertItem 追加
    int existingCount = m_listCtrl->GetItemCount();
    int targetCount = static_cast<int>(sorted.size());

    // 找到目标数据中与现有行最大匹配的 PID 序列（按行顺序）
    // 简化策略：先标记哪些 PID 需要被移除（先 SetItemText 为空），然后重新填充
    for (int i = 0; i < existingCount; ++i) {
        wxString pidStr = m_listCtrl->GetItemText(i, 1);
        long pid = 0;
        pidStr.ToLong(&pid);
        uint32_t uPid = static_cast<uint32_t>(pid);
        if (targetPids.find(uPid) == targetPids.end()) {
            m_listCtrl->DeleteItem(i);
            --existingCount;
            --i;
        }
    }

    // 更新或追加
    for (int i = 0; i < targetCount; ++i) {
        const auto& s = sorted[i];
        if (i < existingCount) {
            // 现有行：更新数据
            m_listCtrl->SetItem(i, 0, s.processName);
            m_listCtrl->SetItem(i, 1, wxString::Format(L"%u", s.pid));
            m_listCtrl->SetItem(i, 2, FormatSpeed(s.currentDownBps));
            m_listCtrl->SetItem(i, 3, FormatSpeed(s.currentUpBps));
            m_listCtrl->SetItem(i, 4, FormatBytes(s.sessionTotalDown));
            m_listCtrl->SetItem(i, 5, FormatBytes(s.sessionTotalUp));
        } else {
            // 追加新行
            long idx = m_listCtrl->InsertItem(i, s.processName);
            m_listCtrl->SetItem(idx, 1, wxString::Format(L"%u", s.pid));
            m_listCtrl->SetItem(idx, 2, FormatSpeed(s.currentDownBps));
            m_listCtrl->SetItem(idx, 3, FormatSpeed(s.currentUpBps));
            m_listCtrl->SetItem(idx, 4, FormatBytes(s.sessionTotalDown));
            m_listCtrl->SetItem(idx, 5, FormatBytes(s.sessionTotalUp));
        }
    }

    m_listCtrl->Thaw();
}

wxString ProcessNetPanel::FormatBytes(uint64_t bytes) {
    if (bytes >= 1024ULL * 1024 * 1024) {
        return wxString::Format(L"%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    } else if (bytes >= 1024ULL * 1024) {
        return wxString::Format(L"%.1f MB", bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024) {
        return wxString::Format(L"%.1f KB", bytes / 1024.0);
    }
    return wxString::Format(L"%llu B", static_cast<unsigned long long>(bytes));
}

wxString ProcessNetPanel::FormatSpeed(uint64_t bytesPerSec) {
    if (bytesPerSec >= 1024ULL * 1024) {
        return wxString::Format(L"%.1f MB/s", bytesPerSec / (1024.0 * 1024.0));
    } else if (bytesPerSec >= 1024) {
        return wxString::Format(L"%.1f KB/s", bytesPerSec / 1024.0);
    }
    return wxString::Format(L"%llu B/s", static_cast<unsigned long long>(bytesPerSec));
}
} // namespace IceClean::Gui
