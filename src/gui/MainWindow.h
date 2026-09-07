#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/timer.h>
#include <wx/taskbar.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>

#include "core/scanner/ScannerAggregator.h"
#include "core/migrator/LargeFolderDetector.h"
#include "models/ScanResult.h"
#include "models/MigrationItem.h"
#include "models/CleanItem.h"
#include "models/StartupItem.h"
#include "controls/TrayIcon.h"

namespace IceClean::Gui {

class CustomTitleBar;
class NavSidebar;
class DashboardPanel;
class ScanResultPanel;
class MigrationPanel;
class StartupPanel;
class UninstallPanel;
class SoftwareRecommendPanel;
class DriverPanel;
class NetworkPanel;
class ProcessNetPanel;
class SecurityPanel;
class SettingsPanel;
class AboutPanel;
class HardwareMonitorPanel;
class WindowsDebloaterPanel;
class PrivacyOptimizerPanel;
class SystemFileManagerPanel;

class CleanProgressDialog;
class UnifiedProgressDialog;

// ── 托盘菜单命令 ID ──
enum class TrayMenuId : int {
    ShowWindow = 1000,
    QuickClean,
    SilentClean,
    CheckUpdate,
    OpenSettings,
    Startup,
    BackgroundMonitor,
    CloseToTray,
    CloseToExit,
    About,
    Exit,
};

// ── 内容书页索引（与 NavSidebar 导航项一一对应） ──
enum class NavPage : int {
    Dashboard = 0,       // 首页（硬件监控）
    DeepClean,           // 深度清理
    Migration,           // 智能迁移
    Startup,             // 加速优化
    SoftwareManage,      // 软件管理
    Security,            // 安全防护
    NetworkOpt,          // 网络优化
    Settings,            // 设置
    About,               // 关于
};

class MainWindow : public wxFrame {
public:
    MainWindow();
    ~MainWindow();

    void SwitchPanel(int index);

private:
    // ── 初始化 ──
    void CreateControls();
    void LayoutControls();
    void InitializeApp();

    // ── 事件处理 ──
    void OnClose(wxCloseEvent& event);
    void OnSize(wxSizeEvent& event);

    // 扫描请求/进度/停止/完成
    void OnScanRequest(wxThreadEvent& event);
    void OnMigrationScanRequest(wxThreadEvent& event);  // 携带阈值的迁移扫描请求
    void OnScanProgressUpdate(wxThreadEvent& event);
    void OnScanStop(wxThreadEvent& event);

    void OnStopTimeout(wxTimerEvent& event);
    void ForceStopScan();
    void StartScan(int scanType, int thresholdMB = 100);

    // 扫描完成
    void OnScanComplete(wxThreadEvent& event);

    // 清理请求/完成
    void OnCleanRequest(wxThreadEvent& event);
    void StartClean(int cleanType, const std::vector<std::wstring>& paths);
    void StartDeepClean(const std::vector<wxString>& selectedIds);
    void OnCleanComplete(wxThreadEvent& event);

    // 清理进度对话框
    void ShowCleanProgress(const wxString& title);
    void UpdateCleanProgress(int percent, const wxString& category,
                             const wxString& detail, uint64_t cleanedSize);
    void CloseCleanProgress(uint64_t totalCleaned);

    // 迁移请求/完成
    void OnMigrateRequest(wxThreadEvent& event);
    void StartMigration(const std::vector<IceClean::Models::MigrationItem>& items,
                        const wxString& targetDrive);
    void OnMigrateComplete(wxThreadEvent& event);

    // 通用操作进度/完成（Debloat/Privacy/系统文件等）
    void OnOperationProgressUpdate(wxThreadEvent& event);
    void OnOperationComplete(wxThreadEvent& event);

    // 启动优化
    void StartStartupOptimize(const std::vector<IceClean::Models::StartupItem>& modifiedStartup,
                              const std::vector<IceClean::Models::StartupItem>& modifiedServices,
                              const std::vector<IceClean::Models::StartupItem>& modifiedTasks);

    // 数据加载
    void LoadStartupData();
    void RefreshDiskInfo();
    void RefreshRecentOperations();

    // ── 状态栏 ──
    void SetStatusText(const wxString& text, int field = 0);
    void SetStatusBusy(const wxString& task);
    void SetStatusIdle();

    // ── UI 控件 ──
    CustomTitleBar* m_titleBar = nullptr;
    NavSidebar* m_sidebar = nullptr;
    wxSimplebook* m_contentBook = nullptr;
    wxStatusBar* m_statusBar = nullptr;

    // 面板
    DashboardPanel* m_dashboardPanel = nullptr;
    ScanResultPanel* m_scanResultPanel = nullptr;
    MigrationPanel* m_migrationPanel = nullptr;
    StartupPanel* m_startupPanel = nullptr;
    UninstallPanel* m_uninstallPanel = nullptr;
    SoftwareRecommendPanel* m_softwareRecommendPanel = nullptr;
    DriverPanel* m_driverPanel = nullptr;
    NetworkPanel* m_networkPanel = nullptr;
    ProcessNetPanel* m_processNetPanel = nullptr;
    SecurityPanel* m_securityPanel = nullptr;
    SettingsPanel* m_settingsPanel = nullptr;
AboutPanel* m_aboutPanel = nullptr;
HardwareMonitorPanel* m_hardwareMonitorPanel = nullptr;
WindowsDebloaterPanel* m_windowsDebloaterPanel = nullptr;
PrivacyOptimizerPanel* m_privacyOptimizerPanel = nullptr;
SystemFileManagerPanel* m_systemFileManagerPanel = nullptr;

    // ── 工作线程 ──
    std::thread m_workerThread;
    std::atomic<bool> m_workerRunning{false};
    std::mutex m_workerMutex;
    std::mutex m_aggregatorMutex;
    IceClean::Core::Scanner::ScannerAggregator* m_currentAggregator = nullptr;
    IceClean::Core::Migrator::LargeFolderDetector* m_activeFolderDetector = nullptr;  // 迁移扫描取消用（m_aggregatorMutex 保护）
    std::atomic<bool> m_stopRequested{false};
    wxTimer* m_stopTimeoutTimer = nullptr;
    wxString m_lastOptimizeFailures;  // 最近一次启动优化的失败项明细（worker→UI 经 m_workerMutex 同步）

    // ── 数据 ──
    IceClean::Models::ScanResult m_lastScanResult;

    // ── 系统托盘 ──
    Gui::TrayIcon* m_taskBarIcon = nullptr;
    wxMenu* CreateTrayMenuMenu();
    void OnTrayLeftClick(wxEvent& event);
    void OnTrayMenuCommand(wxCommandEvent& event);
    void ShowTrayNotification(const wxString& title, const wxString& message, int flags = wxICON_INFORMATION);
    void UpdateTrayIconState(bool isScanning);

    // ── 进度对话框 ──
    wxDialog* m_cleanProgressDlg = nullptr;

    // 事件表
    wxDECLARE_EVENT_TABLE();

#ifdef __WXMSW__
    WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override;
#endif

    // 键盘快捷键
    void OnKeyDown(wxKeyEvent& event);
};

} // namespace IceClean::Gui
