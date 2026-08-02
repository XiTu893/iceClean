#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/scrolwin.h>
#include "gui/controls/CardPanel.h"
#include "gui/controls/CircularProgress.h"
#include "gui/controls/SafetyBadge.h"
#include "models/ScanResult.h"
#include "core/scanner/ScannerAggregator.h"

namespace IceClean::Gui {

// 首页仪表盘面板
class DashboardPanel : public wxPanel {
public:
    DashboardPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 更新C盘空间信息
    void UpdateDiskInfo(uint64_t usedBytes, uint64_t totalBytes);

    // 设置扫描按钮状态
    void SetScanning(bool scanning);

    // 更新扫描进度（扫描过程中实时调用）
    void UpdateScanProgress(int completedScanners, int totalScanners,
                            const std::wstring& currentScanner);
    // 更新扫描进度（使用完整的 ScanProgressInfo，包含文件数）
    void UpdateScanProgress(const IceClean::Core::Scanner::ScanProgressInfo& info);

    // 恢复磁盘信息显示（扫描完成后调用）
    void RestoreDiskInfo();

    // 计算并更新健康评分
    void UpdateHealthScore();

    // 设置上次扫描结果（用于健康评分计算）
    void SetLastScanResult(const IceClean::Models::ScanResult& result);

    // 设置扫描结果并显示在面板中
    void SetScanResult(const IceClean::Models::ScanResult& result);

    // 获取选中的清理项路径列表
    std::vector<std::wstring> GetSelectedPaths() const;

    // 刷新累计清理统计显示
    void LoadCumulativeStats();

private:
    // C盘信息
    uint64_t m_usedBytes = 0;
    uint64_t m_totalBytes = 0;

    // 上次扫描结果
    IceClean::Models::ScanResult m_lastScanResult;

    // 当前扫描结果（用于显示和清理）
    IceClean::Models::ScanResult m_scanResult;

    // 控件
    CircularProgress* m_progressCtrl = nullptr;
    wxStaticText* m_diskInfoLabel = nullptr;
    wxButton* m_scanButton = nullptr;
    wxButton* m_stopButton = nullptr;
    wxStaticText* m_healthScoreLabel = nullptr;
    wxStaticText* m_healthDescLabel = nullptr;

    // 快捷卡片
    CardPanel* m_cardTemp = nullptr;
    CardPanel* m_cardUpdate = nullptr;
    CardPanel* m_cardBrowser = nullptr;
    CardPanel* m_cardHibernation = nullptr;
    CardPanel* m_cardStartup = nullptr;
    CardPanel* m_cardSoftware = nullptr;

    // 累计清理统计
    wxStaticText* m_cumulativeLabel = nullptr;

    // C盘空间预警
    wxStaticText* m_spaceWarningLabel = nullptr;

    // 快捷卡片的容器（用于显示/隐藏）
    wxSizer* m_quickAccessSizer = nullptr;

    // 扫描结果控件
    wxPanel* m_resultSection = nullptr;
    wxScrolledWindow* m_resultScroller = nullptr;
    wxBoxSizer* m_resultSizer = nullptr;
    wxStaticText* m_resultSummaryLabel = nullptr;
    wxButton* m_cleanButton = nullptr;
    std::vector<bool> m_categoryChecked;

    // 每个分类的UI数据
    struct CategoryUI {
        wxPanel* headerPanel = nullptr;
        wxPanel* detailPanel = nullptr;
        wxStaticText* arrowLabel = nullptr;
        wxCheckBox* categoryCheck = nullptr;
        std::vector<wxCheckBox*> itemChecks;
        bool expanded = false;
    };
    std::vector<CategoryUI> m_categoryUIs;

    // 停止按钮状态
    bool m_stopForceMode = false;

    void CreateControls();
    void CreateQuickAccessCards(wxSizer* parentSizer);
    void CreateResultSection(wxSizer* parentSizer);

    // 扫描结果显示/隐藏
    void ShowScanResults();
    void HideScanResults();
    void BuildCategoryList();
    void UpdateResultSummary();

    // 事件处理
    void OnScanButton(wxCommandEvent& event);
    void OnStopButton(wxCommandEvent& event);
    void OnQuickAccessCard(wxCommandEvent& event);
    void OnCleanButton(wxCommandEvent& event);
    void OnCategoryToggle(wxCommandEvent& event);
    void OnItemToggle(wxCommandEvent& event);
    void OnCategoryHeaderClick(wxMouseEvent& event);

    // C盘空间监控定时器
    wxTimer* m_diskTimer = nullptr;
    void OnDiskTimer(wxTimerEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
