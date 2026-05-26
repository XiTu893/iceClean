#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "gui/controls/CardPanel.h"
#include "gui/controls/CircularProgress.h"
#include "models/ScanResult.h"
#include "models/OperationRecord.h"

namespace IceClean::Gui {

// 首页仪表盘面板
class DashboardPanel : public wxPanel {
public:
    DashboardPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 更新C盘空间信息
    void UpdateDiskInfo(uint64_t usedBytes, uint64_t totalBytes);

    // 更新最近操作列表
    void UpdateRecentOperations(const std::vector<IceClean::Models::OperationRecord>& records);

    // 设置扫描按钮状态
    void SetScanning(bool scanning);

    // 更新扫描进度（扫描过程中实时调用）
    void UpdateScanProgress(int completedScanners, int totalScanners,
                            const std::wstring& currentScanner);

    // 恢复磁盘信息显示（扫描完成后调用）
    void RestoreDiskInfo();

private:
    // C盘信息
    uint64_t m_usedBytes = 0;
    uint64_t m_totalBytes = 0;

    // 控件
    CircularProgress* m_progressCtrl = nullptr;
    wxStaticText* m_diskInfoLabel = nullptr;
    wxButton* m_scanButton = nullptr;
    wxListCtrl* m_recentList = nullptr;

    // 快捷卡片
    CardPanel* m_cardTemp = nullptr;
    CardPanel* m_cardUpdate = nullptr;
    CardPanel* m_cardBrowser = nullptr;
    CardPanel* m_cardHibernation = nullptr;

    void CreateControls();
    void CreateQuickAccessCards(wxSizer* parentSizer);
    void CreateRecentOperations(wxSizer* parentSizer);

    // 事件处理
    void OnScanButton(wxCommandEvent& event);
    void OnQuickAccessCard(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
