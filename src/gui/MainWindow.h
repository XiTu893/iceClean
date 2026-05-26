#pragma once
#include <wx/wx.h>
#include <wx/simplebook.h>
#include <wx/taskbar.h>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include "models/ScanResult.h"
#include "models/MigrationItem.h"
#include "models/StartupItem.h"

namespace IceClean::Gui {

// Forward declarations for panels
class NavSidebar;
class DashboardPanel;
class ScanResultPanel;
class DeepCleanPanel;
class MigrationPanel;
class StartupPanel;
class DiskAnalyzerPanel;
class SettingsPanel;

class MainWindow : public wxFrame {
public:
    MainWindow();
    ~MainWindow() override;

    // Switch to a specific panel by index
    void SwitchPanel(int index);

    // Get the scan result panel
    ScanResultPanel* GetScanResultPanel() { return m_scanResultPanel; }

private:
    void CreateControls();
    void LayoutControls();
    void InitializeApp();

    // ── 事件处理 ──
    void OnClose(wxCloseEvent& event);
    void OnSize(wxSizeEvent& event);

    // 面板事件处理
    void OnScanRequest(wxThreadEvent& event);
    void OnScanProgressUpdate(wxThreadEvent& event);
    void OnCleanRequest(wxThreadEvent& event);
    void OnMigrateRequest(wxThreadEvent& event);

    // 后台任务完成回调
    void OnScanComplete(wxThreadEvent& event);
    void OnCleanComplete(wxThreadEvent& event);
    void OnMigrateComplete(wxThreadEvent& event);

    // ── 核心操作 ──
    void StartScan(int scanType);  // 0=普通扫描, 1=迁移扫描, 3=磁盘分析
    void StartClean(int cleanType, const std::vector<std::wstring>& paths);
    void StartDeepClean(const std::vector<wxString>& selectedIds);
    void StartMigration(const std::vector<IceClean::Models::MigrationItem>& items, const wxString& targetDrive);
    void StartStartupOptimize(const std::vector<IceClean::Models::StartupItem>& modifiedStartup,
                               const std::vector<IceClean::Models::StartupItem>& modifiedServices);
    void LoadStartupData();

    // ── UI更新 ──
    void RefreshDiskInfo();
    void RefreshRecentOperations();

    // ── 成员变量 ──
    NavSidebar* m_sidebar = nullptr;
    wxSimplebook* m_contentBook = nullptr;
    wxTaskBarIcon* m_taskBarIcon = nullptr;

    // Panels
    DashboardPanel* m_dashboardPanel = nullptr;
    ScanResultPanel* m_scanResultPanel = nullptr;
    DeepCleanPanel* m_deepCleanPanel = nullptr;
    MigrationPanel* m_migrationPanel = nullptr;
    StartupPanel* m_startupPanel = nullptr;
    DiskAnalyzerPanel* m_diskAnalyzerPanel = nullptr;
    SettingsPanel* m_settingsPanel = nullptr;

    // 后台线程管理
    std::thread m_workerThread;
    std::atomic<bool> m_workerRunning{false};
    std::mutex m_workerMutex;

    // 扫描结果缓存
    IceClean::Models::ScanResult m_lastScanResult;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
