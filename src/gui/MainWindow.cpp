#include "MainWindow.h"
#include "controls/NavSidebar.h"
#include "controls/CustomTitleBar.h"
#include "controls/ThemeManager.h"
#include "panels/DashboardPanel.h"
#include "panels/ScanResultPanel.h"
#include "panels/MigrationPanel.h"
#include "panels/DuplicateFilePanel.h"
#include "panels/StartupPanel.h"
#include "panels/UninstallPanel.h"
#include "panels/SoftwareRecommendPanel.h"
#include "panels/DiskAnalyzerPanel.h"
#include "panels/DriverPanel.h"
#include "panels/NetworkPanel.h"
#include "panels/SecurityPanel.h"
#include "panels/SettingsPanel.h"
#include "panels/AboutPanel.h"
#include "panels/HardwareInfoPanel.h"
#include "panels/WindowsDebloaterPanel.h"
#include "panels/PrivacyOptimizerPanel.h"
#include "panels/SystemFileManagerPanel.h"
#include "panels/FileTypeAnalyzerPanel.h"
#include "panels/DownloadManagerPanel.h"
#include "gui/Events.h"
#include "gui/dialogs/CleanProgressDialog.h"
#include "gui/dialogs/UnifiedProgressDialog.h"
#include "core/utils/ProgressReporter.h"

// Core logic
#include "core/scanner/ScannerAggregator.h"
#include "core/cleaner/FileCleaner.h"
#include "core/cleaner/RecycleBinCleaner.h"
#include "core/cleaner/BrowserCleaner.h"
#include "core/cleaner/HibernationCleaner.h"
#include "core/cleaner/DismCleaner.h"
#include "core/cleaner/PrivacyCleaner.h"
#include "core/migrator/LargeFolderDetector.h"
#include "core/migrator/FolderMigrator.h"
#include "core/migrator/WeChatMigrator.h"
#include "core/migrator/QQMigrator.h"
#include "core/migrator/SteamMigrator.h"
#include "core/migrator/UserFolderMigrator.h"
#include "core/migrator/DevCacheMigrator.h"
#include "core/optimizer/StartupOptimizer.h"
#include "core/optimizer/ServiceOptimizer.h"
#include "core/optimizer/ScheduledTaskOptimizer.h"
#include "core/analyzer/DiskSpaceAnalyzer.h"
#include "core/safety/RestorePointManager.h"
#include "core/safety/OperationLogger.h"
#include "core/safety/RollbackManager.h"
#include "core/safety/UsageStats.h"
#include "utils/Win32Util.h"
#include "utils/FormatUtil.h"

#include <wx/dcbuffer.h>
#include <wx/busyinfo.h>
#include <wx/artprov.h>
#include <wx/notebook.h>

#ifdef __WXMSW__
#include <windowsx.h>
#endif

namespace IceClean::Gui {

// ── Event table ──

wxBEGIN_EVENT_TABLE(MainWindow, wxFrame)
    EVT_CLOSE(MainWindow::OnClose)
    EVT_SIZE(MainWindow::OnSize)
wxEND_EVENT_TABLE()

// ── Constructor / Destructor ──

MainWindow::MainWindow()
    : wxFrame(nullptr, wxID_ANY, L"IceClean - 智能C盘清理工具",
              wxDefaultPosition, wxSize(960, 680),
              wxNO_BORDER | wxCLIP_CHILDREN)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    SetMinSize(wxSize(800, 600));
    CreateControls();
    LayoutControls();

    // 绑定面板事件到 m_contentBook（面板的 GetParent() 返回 m_contentBook）
    m_contentBook->Bind(wxEVT_SCAN_REQUEST, &MainWindow::OnScanRequest, this);
    m_contentBook->Bind(wxEVT_SCAN_STOP, &MainWindow::OnScanStop, this);
    m_contentBook->Bind(wxEVT_CLEAN_PROGRESS, &MainWindow::OnCleanRequest, this);
    m_contentBook->Bind(wxEVT_MIGRATE_PROGRESS, &MainWindow::OnMigrateRequest, this);

    // 绑定完成事件到自身
    Bind(wxEVT_SCAN_COMPLETE, &MainWindow::OnScanComplete, this);
    Bind(wxEVT_CLEAN_COMPLETE, &MainWindow::OnCleanComplete, this);
    Bind(wxEVT_MIGRATE_COMPLETE, &MainWindow::OnMigrateComplete, this);

    // 绑定扫描进度更新事件
    Bind(wxEVT_SCAN_PROGRESS_UPDATE, &MainWindow::OnScanProgressUpdate, this);

    // 创建停止超时定时器
    m_stopTimeoutTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &MainWindow::OnStopTimeout, this, m_stopTimeoutTimer->GetId());

    // 延迟初始化（等窗口显示后）
    CallAfter([this]() { InitializeApp(); });

    // 键盘快捷键
    Bind(wxEVT_CHAR_HOOK, &MainWindow::OnKeyDown, this);

    // 注册主题变更回调
    ThemeManager::Instance().RegisterChangeCallback([this](const ThemeColors& colors) {
        CallAfter([this, colors]() {
            ThemeManager::Instance().ApplyTheme(this);
            Refresh();
        });
    });
}

MainWindow::~MainWindow() {
    // 停止超时定时器
    if (m_stopTimeoutTimer) {
        m_stopTimeoutTimer->Stop();
        delete m_stopTimeoutTimer;
        m_stopTimeoutTimer = nullptr;
    }
    // 确保工作线程结束
    if (m_workerThread.joinable()) {
        m_workerRunning = false;
        m_workerThread.detach();
    }
}

// ── Control creation ──

void MainWindow::CreateControls()
{
    // Create custom title bar
    m_titleBar = new CustomTitleBar(this, this);

    // Create sidebar
    m_sidebar = new NavSidebar(this);

    // Create content area (simplebook for panel switching)
    m_contentBook = new wxSimplebook(this, wxID_ANY);
    m_contentBook->SetBackgroundColour(ThemeManager::Instance().GetColors().background);

    // ════════════════════════════════════════════════════════
    //  0: 首页
    // ════════════════════════════════════════════════════════
    m_dashboardPanel = new DashboardPanel(m_contentBook);

    // ════════════════════════════════════════════════════════
    //  1: 深度清理
    // ════════════════════════════════════════════════════════
    m_scanResultPanel = new ScanResultPanel(m_contentBook);

    // ════════════════════════════════════════════════════════
    //  2: 智能迁移（仅保留迁移+重复文件，磁盘分析移至独立导航项）
    // ════════════════════════════════════════════════════════
    auto* migrationComboPanel = new wxPanel(m_contentBook);
    migrationComboPanel->SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    auto* migrationNotebook = new wxNotebook(migrationComboPanel, wxID_ANY);
    m_migrationPanel = new MigrationPanel(migrationNotebook);
    m_duplicateFilePanel = new DuplicateFilePanel(migrationNotebook);
    migrationNotebook->AddPage(m_migrationPanel, L"智能迁移");
    migrationNotebook->AddPage(m_duplicateFilePanel, L"重复文件");
    auto* migrationSizer = new wxBoxSizer(wxVERTICAL);
    migrationSizer->Add(migrationNotebook, 1, wxEXPAND);
    migrationComboPanel->SetSizer(migrationSizer);

    // ════════════════════════════════════════════════════════
    //  3: 加速优化（扩展为 wxNotebook，含 4 个子标签）
    //     启动管理 / Windows组件精简 / 隐私策略 / 系统文件
    // ════════════════════════════════════════════════════════
    auto* startupComboPanel = new wxPanel(m_contentBook);
    startupComboPanel->SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    auto* startupNotebook = new wxNotebook(startupComboPanel, wxID_ANY);

    m_startupPanel = new StartupPanel(startupNotebook);
    auto* debloatPanel = new WindowsDebloaterPanel(startupNotebook);
    auto* privacyPanel = new PrivacyOptimizerPanel(startupNotebook);
    auto* sysFilePanel = new SystemFileManagerPanel(startupNotebook);

    startupNotebook->AddPage(m_startupPanel, L"启动管理");
    startupNotebook->AddPage(debloatPanel, L"Windows组件精简");
    startupNotebook->AddPage(privacyPanel, L"隐私策略");
    startupNotebook->AddPage(sysFilePanel, L"系统文件");

    auto* startupSizer = new wxBoxSizer(wxVERTICAL);
    startupSizer->Add(startupNotebook, 1, wxEXPAND);
    startupComboPanel->SetSizer(startupSizer);

    // ════════════════════════════════════════════════════════
    //  4: 软件管理
    // ════════════════════════════════════════════════════════
    m_uninstallPanel = new UninstallPanel(m_contentBook);

    // ════════════════════════════════════════════════════════
    //  5: 软件推荐
    // ════════════════════════════════════════════════════════
    m_softwareRecommendPanel = new SoftwareRecommendPanel(m_contentBook);

    // ════════════════════════════════════════════════════════
    //  6: 安全防护
    // ════════════════════════════════════════════════════════
    m_securityPanel = new SecurityPanel(m_contentBook);

    // ════════════════════════════════════════════════════════
    //  7: 网络优化（网络优化 + 驱动管理）
    // ════════════════════════════════════════════════════════
    auto* networkComboPanel = new wxPanel(m_contentBook);
    networkComboPanel->SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    auto* networkNotebook = new wxNotebook(networkComboPanel, wxID_ANY);
    m_networkPanel = new NetworkPanel(networkNotebook);
    m_driverPanel = new DriverPanel(networkNotebook);
    networkNotebook->AddPage(m_networkPanel, L"网络优化");
    networkNotebook->AddPage(m_driverPanel, L"驱动管理");
    auto* networkSizer = new wxBoxSizer(wxVERTICAL);
    networkSizer->Add(networkNotebook, 1, wxEXPAND);
    networkComboPanel->SetSizer(networkSizer);

    // ════════════════════════════════════════════════════════
    //  8: 磁盘分析（从智能迁移拆出，成为独立导航项）
    // ════════════════════════════════════════════════════════
    m_diskAnalyzerPanel = new DiskAnalyzerPanel(m_contentBook);

    // ════════════════════════════════════════════════════════
    //  9: 文件分类统计
    // ════════════════════════════════════════════════════════
    auto* fileTypePanel = new FileTypeAnalyzerPanel(m_contentBook);

    // ════════════════════════════════════════════════════════
    //  10: 下载文件管理
    // ════════════════════════════════════════════════════════
    auto* downloadPanel = new DownloadManagerPanel(m_contentBook);

    // ════════════════════════════════════════════════════════
    //  11: 设置
    // ════════════════════════════════════════════════════════
    m_settingsPanel = new SettingsPanel(m_contentBook);

    // ════════════════════════════════════════════════════════
    //  12: 关于 + 硬件信息（wxNotebook 组合）
    // ════════════════════════════════════════════════════════
    auto* aboutComboPanel = new wxPanel(m_contentBook);
    aboutComboPanel->SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    auto* aboutNotebook = new wxNotebook(aboutComboPanel, wxID_ANY);

    m_aboutPanel = new AboutPanel(aboutNotebook);
    auto* hardwarePanel = new HardwareInfoPanel(aboutNotebook);

    aboutNotebook->AddPage(m_aboutPanel, L"关于 IceClean");
    aboutNotebook->AddPage(hardwarePanel, L"硬件信息");

    auto* aboutSizer = new wxBoxSizer(wxVERTICAL);
    aboutSizer->Add(aboutNotebook, 1, wxEXPAND);
    aboutComboPanel->SetSizer(aboutSizer);

    // ── 按导航顺序添加页面（13 页） ──
    m_contentBook->AddPage(m_dashboardPanel, L"首页");                // 0
    m_contentBook->AddPage(m_scanResultPanel, L"深度清理");            // 1
    m_contentBook->AddPage(migrationComboPanel, L"智能迁移");          // 2
    m_contentBook->AddPage(startupComboPanel, L"加速优化");         // 3
    m_contentBook->AddPage(m_uninstallPanel, L"软件管理");             // 4
    m_contentBook->AddPage(m_softwareRecommendPanel, L"软件推荐");     // 5
    m_contentBook->AddPage(m_securityPanel, L"安全防护");              // 6
    m_contentBook->AddPage(networkComboPanel, L"网络优化");            // 7
    m_contentBook->AddPage(m_diskAnalyzerPanel, L"磁盘分析");          // 8
    m_contentBook->AddPage(fileTypePanel, L"文件分类");            // 9
    m_contentBook->AddPage(downloadPanel, L"下载管理");            // 10
    m_contentBook->AddPage(m_settingsPanel, L"设置");                  // 11
    m_contentBook->AddPage(aboutComboPanel, L"关于");               // 12

    // 绑定面板事件
    m_contentBook->Bind(wxEVT_SCAN_REQUEST, &MainWindow::OnScanRequest, this);
    m_contentBook->Bind(wxEVT_SCAN_STOP, &MainWindow::OnScanStop, this);
    m_contentBook->Bind(wxEVT_CLEAN_PROGRESS, &MainWindow::OnCleanRequest, this);
    m_contentBook->Bind(wxEVT_MIGRATE_PROGRESS, &MainWindow::OnMigrateRequest, this);

    // 绑定完成事件到自身
    Bind(wxEVT_SCAN_COMPLETE, &MainWindow::OnScanComplete, this);
    Bind(wxEVT_CLEAN_COMPLETE, &MainWindow::OnCleanComplete, this);
    Bind(wxEVT_MIGRATE_COMPLETE, &MainWindow::OnMigrateComplete, this);

    // 绑定通用操作事件（Phase 1 新增）
    Bind(wxEVT_OPERATION_PROGRESS_UPDATE, &MainWindow::OnOperationProgressUpdate, this);
    Bind(wxEVT_OPERATION_COMPLETE, &MainWindow::OnOperationComplete, this);

    // 绑定扫描进度更新事件
    Bind(wxEVT_SCAN_PROGRESS_UPDATE, &MainWindow::OnScanProgressUpdate, this);

    // 创建停止超时定时器
    m_stopTimeoutTimer = new wxTimer(this);
    Bind(wxEVT_TIMER, &MainWindow::OnStopTimeout, this, m_stopTimeoutTimer->GetId());

    // 延迟初始化
    CallAfter([this]() { InitializeApp(); });

    // 注册主题变更回调
    ThemeManager::Instance().RegisterChangeCallback([this](const ThemeColors& colors) {
        CallAfter([this, colors]() {
            ThemeManager::Instance().ApplyTheme(this);
            Refresh();
        });
    });

    // 绑定侧边栏切换事件
    m_sidebar->Bind(wxEVT_NAV_SELECTION_CHANGED, [this](wxCommandEvent& event) {
        SwitchPanel(event.GetInt());
    });
}

void MainWindow::LayoutControls()
{
    // 整体垂直布局：标题栏在顶部，侧边栏+内容区在下方
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // 标题栏 - 固定高度在顶部
    rootSizer->Add(m_titleBar, 0, wxEXPAND);

    // 下方区域：侧边栏 + 内容区
    auto* bodySizer = new wxBoxSizer(wxHORIZONTAL);

    // Sidebar: fixed 200px width, full height
    bodySizer->Add(m_sidebar, 0, wxEXPAND);

    // Separator line between sidebar and content
    auto* separator = new wxPanel(this, wxID_ANY);
    separator->SetBackgroundColour(ThemeManager::Instance().GetColors().divider);
    separator->SetSize(1, -1);
    separator->SetMinSize(wxSize(1, 1));
    bodySizer->Add(separator, 0, wxEXPAND);

    // Content area: expand to fill remaining space
    bodySizer->Add(m_contentBook, 1, wxEXPAND);

    rootSizer->Add(bodySizer, 1, wxEXPAND);

    // ── 状态栏 ──
    m_statusBar = new wxStatusBar(this, wxID_ANY);
    m_statusBar->SetFieldsCount(3);
    m_statusBar->SetStatusStyles(3, {wxSB_NORMAL, wxSB_NORMAL, wxSB_NORMAL});
    m_statusBar->SetMinHeight(24);
    int widths[] = { -2, -1, 180 };
    m_statusBar->SetStatusWidths(3, widths);
    m_statusBar->SetStatusText(L"就绪", 0);
    m_statusBar->SetStatusText(L"", 1);
    m_statusBar->SetStatusText(L"IceClean v1.0", 2);
    m_statusBar->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    rootSizer->Add(m_statusBar, 0, wxEXPAND);

    SetSizer(rootSizer);
    Layout();

    // 状态栏右上角文字对齐
    m_statusBar->Bind(wxEVT_SIZE, [this](wxSizeEvent&) {
        m_statusBar->SetStatusText(L"IceClean v1.0", 2);
    });

// ── Initialization ──

void MainWindow::InitializeApp()
{
    // 加载设置
    m_settingsPanel->LoadSettings();

    // 初始化系统托盘图标
    m_taskBarIcon = new wxTaskBarIcon();
    wxIcon trayIcon = wxArtProvider::GetIcon(wxART_INFORMATION, wxART_OTHER, wxSize(16, 16));
    if (trayIcon.IsOk()) {
        m_taskBarIcon->SetIcon(trayIcon, L"IceClean - 智能C盘清理工具");
    }

    // 托盘图标菜单
    m_taskBarIcon->Bind(wxEVT_TASKBAR_LEFT_DOWN, [this](wxEvent&) {
        Show(true);
        Iconize(false);
        Raise();
    });

    // 刷新C盘空间信息
    RefreshDiskInfo();

    // 加载最近操作记录
    RefreshRecentOperations();

    // 加载启动项数据
    LoadStartupData();

    // 加载回滚日志
    IceClean::Core::Safety::RollbackManager::Instance().LoadRollbackLog();

    // 启动时检查更新（静默）
    m_settingsPanel->CheckForUpdate();
}

// ── Panel switching ──

void MainWindow::SwitchPanel(int index)
{
    if (index >= 0 && index < static_cast<int>(m_contentBook->GetPageCount())) {
        m_contentBook->SetSelection(index);

        // 切换到加速优化面板时刷新启动项数据
        if (index == static_cast<int>(NavPage::Startup)) {
            LoadStartupData();
        }
        // 切换到设置面板时刷新日志
        else if (index == static_cast<int>(NavPage::Settings)) {
            m_settingsPanel->RefreshLog();
        }
    }
}

// ── Event handlers ──

void MainWindow::OnClose(wxCloseEvent& event)
{
    if (m_workerRunning) {
        // 工作线程正在运行，等待完成
        if (event.CanVeto()) {
            event.Veto();
            return;
        }
    }

    // 检查是否最小化到托盘
    if (m_settingsPanel->IsMinimizeToTrayEnabled() && m_taskBarIcon && m_taskBarIcon->IsIconInstalled()) {
        // 最小化到托盘而不是关闭
        if (event.CanVeto()) {
            Hide();
            event.Veto();
            return;
        }
    }

    // 保存设置
    m_settingsPanel->SaveSettings();

    // 移除托盘图标
    if (m_taskBarIcon) {
        m_taskBarIcon->RemoveIcon();
        delete m_taskBarIcon;
        m_taskBarIcon = nullptr;
    }

    if (m_workerThread.joinable()) {
        m_workerThread.detach();
    }

    Destroy();
}

void MainWindow::OnSize(wxSizeEvent& event)
{
    event.Skip();
}

// ── 扫描请求处理 ──

void MainWindow::OnScanRequest(wxThreadEvent& event)
{
    int scanType = event.GetInt();  // 0=普通扫描, 1=迁移扫描, 3=磁盘分析
    StartScan(scanType);
}

void MainWindow::OnScanProgressUpdate(wxThreadEvent& event)
{
    auto progress = event.GetPayload<IceClean::Core::Scanner::ScanProgressInfo>();
    m_dashboardPanel->UpdateScanProgress(progress);
}

void MainWindow::OnScanStop(wxThreadEvent& event) {
    int stopMode = event.GetInt();  // 0=正常停止, 1=强制停止

    if (stopMode == 1) {
        // 强制停止：立即恢复UI，detach工作线程
        ForceStopScan();
        return;
    }

    // 正常停止：请求扫描器停止
    m_stopRequested = true;
    {
        std::lock_guard<std::mutex> lock(m_aggregatorMutex);
        if (m_currentAggregator) {
            m_currentAggregator->RequestStop();
        }
    }

    // 停止磁盘分析器
    if (m_diskAnalyzer) {
        m_diskAnalyzer->Cancel();
    }

    // 启动3秒超时定时器，超时后自动强制停止
    if (m_stopTimeoutTimer) {
        m_stopTimeoutTimer->StartOnce(3000);
    }
}

void MainWindow::OnStopTimeout(wxTimerEvent& event) {
    // 超时：扫描线程3秒内未结束，强制停止
    if (m_workerRunning && m_stopRequested) {
        ForceStopScan();
    }
}

void MainWindow::ForceStopScan() {
    // 停止超时定时器
    if (m_stopTimeoutTimer) {
        m_stopTimeoutTimer->Stop();
    }
    m_stopRequested = false;

    // 清空aggregator指针，防止后续访问
    {
        std::lock_guard<std::mutex> lock(m_aggregatorMutex);
        m_currentAggregator = nullptr;
    }

    // detach工作线程，让它自行结束
    if (m_workerThread.joinable()) {
        m_workerThread.detach();
    }
    m_workerRunning = false;

    // 立即恢复UI
    m_dashboardPanel->SetScanning(false);
    m_dashboardPanel->RestoreDiskInfo();
}

void MainWindow::StartScan(int scanType)
{
    std::lock_guard<std::mutex> lock(m_workerMutex);
    if (m_workerRunning) return;

    m_workerRunning = true;

    if (scanType == 0) {
        // 普通扫描 - 更新仪表盘状态
        m_dashboardPanel->SetScanning(true);
        SetStatusBusy(L"正在扫描系统垃圾...");
    }

    m_workerThread = std::thread([this, scanType]() {
        if (scanType == 0) {
            // ── 普通垃圾扫描 ──
            IceClean::Core::Scanner::ScannerAggregator aggregator;
            {
                std::lock_guard<std::mutex> lock(m_aggregatorMutex);
                m_currentAggregator = &aggregator;
            }
            auto result = aggregator.ScanAll(this);
            {
                std::lock_guard<std::mutex> lock(m_aggregatorMutex);
                m_currentAggregator = nullptr;
            }

            // 发送完成事件
            wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_SCAN_COMPLETE);
            completeEvt->SetInt(0);
            completeEvt->SetPayload(result);
            wxQueueEvent(this, completeEvt);
        }
        else if (scanType == 1) {
            // ── 迁移扫描（大文件检测）──
            IceClean::Core::Migrator::LargeFolderDetector detector(500);
            auto items = detector.Detect([this](const std::wstring& currentPath) {
                // 进度回调（可选）
            });

            // 同时检测微信/QQ/Steam
            IceClean::Core::Migrator::WeChatMigrator wechatMigrator;
            auto wechatItems = wechatMigrator.Detect();
            items.insert(items.end(), wechatItems.begin(), wechatItems.end());

            IceClean::Core::Migrator::QQMigrator qqMigrator;
            auto qqItems = qqMigrator.Detect();
            items.insert(items.end(), qqItems.begin(), qqItems.end());

            IceClean::Core::Migrator::SteamMigrator steamMigrator;
            auto steamItems = steamMigrator.Detect();
            items.insert(items.end(), steamItems.begin(), steamItems.end());

            IceClean::Core::Migrator::UserFolderMigrator userMigrator;
            auto userItems = userMigrator.Detect();
            items.insert(items.end(), userItems.begin(), userItems.end());

            wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_SCAN_COMPLETE);
            completeEvt->SetInt(1);
            completeEvt->SetPayload(items);
            wxQueueEvent(this, completeEvt);
        }
        else if (scanType == 3) {
            // ── 磁盘分析扫描 ──
            auto drive = m_diskAnalyzerPanel->GetSelectedDrive().ToStdWstring();
            if (drive.empty()) drive = L"C:\\";

            delete m_diskAnalyzer;
            m_diskAnalyzer = new IceClean::Core::Analyzer::DiskSpaceAnalyzer();
            auto rootNode = m_diskAnalyzer->Scan(drive, [this](const IceClean::Core::Analyzer::ScanProgress& progress) {
                if (progress.scannedDirs % 100 == 0) {
                    wxString status = wxString::Format(L"正在扫描: %s (已扫描 %llu 个目录)",
                        progress.currentPath.c_str(), progress.scannedDirs);
                    CallAfter([this, status]() {
                        // 更新磁盘分析面板的状态栏
                        // 注意：这里不能直接访问m_diskAnalyzerPanel的私有成员
                    });
                }
            });

            wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_SCAN_COMPLETE);
            completeEvt->SetInt(3);
            completeEvt->SetPayload(rootNode);
            wxQueueEvent(this, completeEvt);
        }

        m_workerRunning = false;
    });

    m_workerThread.detach();
}

// ── 扫描完成处理 ──

void MainWindow::OnScanComplete(wxThreadEvent& event)
{
    // 停止超时定时器（正常完成，不需要超时了）
    if (m_stopTimeoutTimer) {
        m_stopTimeoutTimer->Stop();
    }
    m_stopRequested = false;

    int scanType = event.GetInt();

    if (scanType == 0) {
        // 普通扫描完成
        m_dashboardPanel->SetScanning(false);
        m_dashboardPanel->RestoreDiskInfo();  // 恢复磁盘信息显示

        auto result = event.GetPayload<IceClean::Models::ScanResult>();
        m_lastScanResult = result;

        // 将结果传递给扫描结果面板（深度清理标签页仍可访问）
        m_scanResultPanel->SetScanResult(result);

        // 更新健康评分
        m_dashboardPanel->SetLastScanResult(result);

        // 将结果显示在首页面板中，不再切换到扫描结果面板
        if (!result.categories.empty()) {
            m_dashboardPanel->SetScanResult(result);
        }
    }
    else if (scanType == 1) {
        // 迁移扫描完成
        auto items = event.GetPayload<std::vector<IceClean::Models::MigrationItem>>();
        m_migrationPanel->SetMigrationItems(items);
    }
    else if (scanType == 3) {
        // 磁盘分析完成
        auto rootNode = event.GetPayload<std::shared_ptr<IceClean::Models::DiskNode>>();
        m_diskAnalyzerPanel->SetDiskData(rootNode);
    }
}

// ── 清理请求处理 ──

void MainWindow::OnCleanRequest(wxThreadEvent& event)
{
    int cleanType = event.GetInt();

    if (cleanType == 0) {
        // 普通清理 - 优先从仪表盘面板获取选中路径，其次从扫描结果面板获取
        auto paths = m_dashboardPanel->GetSelectedPaths();
        if (paths.empty()) {
            paths = m_scanResultPanel->GetSelectedPaths();
        }
        if (paths.empty()) return;
        StartClean(cleanType, paths);
    }
    else if (cleanType == 1) {
        // 深度清理 - 从事件payload获取选中的清理项ID
        auto selectedIds = event.GetPayload<std::vector<wxString>>();
        if (selectedIds.empty()) return;
        StartDeepClean(selectedIds);
    }
    else if (cleanType == 2) {
        // 启动优化
        auto modifiedStartup = m_startupPanel->GetModifiedStartupItems();
        auto modifiedServices = m_startupPanel->GetModifiedServices();
        auto modifiedTasks = m_startupPanel->GetModifiedScheduledTasks();
        StartStartupOptimize(modifiedStartup, modifiedServices, modifiedTasks);
    }
}

void MainWindow::StartClean(int cleanType, const std::vector<std::wstring>& paths)
{
    std::lock_guard<std::mutex> lock(m_workerMutex);
    if (m_workerRunning) return;
    m_workerRunning = true;

    // 显示清理进度对话框
    ShowCleanProgress(L"正在清理垃圾文件");
    SetStatusBusy(L"正在清理垃圾文件...");

    // 创建还原点
    IceClean::Core::Safety::RestorePointManager::CreateRestorePoint(L"IceClean 清理操作前自动还原点");

    m_workerThread = std::thread([this, cleanType, paths]() {
        uint64_t totalFreed = 0;

        // 文件清理
        IceClean::Core::Cleaner::FileCleaner fileCleaner;
        auto fileResult = fileCleaner.Clean(paths, [this](const IceClean::Models::CleanProgress& progress) {
            int pct = progress.totalItems > 0 ? (progress.currentItem * 100 / progress.totalItems) : 0;
            wxString file(progress.currentFile);
            uint64_t cleaned = progress.cleanedSize;
            CallAfter([this, pct, file, cleaned]() {
                UpdateCleanProgress(pct, L"垃圾文件清理", file, cleaned);
            });
        });
        totalFreed += fileResult.totalCleanedSize;

        // 记录操作日志
        IceClean::Models::OperationRecord record;
        record.type = IceClean::Models::OperationType::Clean;
        record.description = L"清理垃圾文件";
        record.size = totalFreed;
        record.timestamp = std::chrono::system_clock::now();
        record.success = fileResult.success;
        IceClean::Core::Safety::OperationLogger::LogOperation(record);

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_CLEAN_COMPLETE);
        completeEvt->SetInt(cleanType);
        completeEvt->SetPayload(totalFreed);
        wxQueueEvent(this, completeEvt);

        m_workerRunning = false;
    });

    m_workerThread.detach();
}

void MainWindow::StartDeepClean(const std::vector<wxString>& selectedIds)
{
    std::lock_guard<std::mutex> lock(m_workerMutex);
    if (m_workerRunning) return;
    m_workerRunning = true;

    // 显示清理进度对话框
    ShowCleanProgress(L"正在深度清理");
    SetStatusBusy(L"正在深度清理...");

    // 创建还原点
    IceClean::Core::Safety::RestorePointManager::CreateRestorePoint(L"IceClean 深度清理前自动还原点");

    int totalSteps = static_cast<int>(selectedIds.size());
    int currentStep = 0;

    m_workerThread = std::thread([this, selectedIds, totalSteps]() {
        uint64_t totalFreed = 0;
        int currentStep = 0;

        for (const auto& id : selectedIds) {
            currentStep++;
            int percent = (currentStep * 100) / (totalSteps + 1);

            wxString category;
            if (id == L"winSxS") {
                category = L"WinSxS组件清理";
                CallAfter([this, percent, category]() {
                    UpdateCleanProgress(percent, category, L"正在清理WinSxS组件...", 0);
                });
                IceClean::Core::Cleaner::DismCleaner dismCleaner;
                auto result = dismCleaner.Clean({L"WinSxS"});
                totalFreed += result.totalCleanedSize;
            }
            else if (id == L"compactOS") {
                category = L"CompactOS压缩";
                CallAfter([this, percent, category]() {
                    UpdateCleanProgress(percent, category, L"正在压缩系统文件...", 0);
                });
                IceClean::Core::Cleaner::DismCleaner dismCleaner;
                auto result = dismCleaner.Clean({L"CompactOS"});
                totalFreed += result.totalCleanedSize;
            }
            else if (id == L"hibernation") {
                category = L"关闭休眠功能";
                CallAfter([this, percent, category]() {
                    UpdateCleanProgress(percent, category, L"正在关闭休眠并删除hiberfil.sys...", 0);
                });
                IceClean::Core::Cleaner::HibernationCleaner hibCleaner;
                auto result = hibCleaner.Clean({});
                totalFreed += result.totalCleanedSize;
            }
            else if (id == L"oldWindows") {
                category = L"删除旧Windows安装";
                CallAfter([this, percent, category]() {
                    UpdateCleanProgress(percent, category, L"正在删除旧安装文件...", 0);
                });
                std::vector<std::wstring> oldWinPaths = {
                    L"C:\\Windows.old",
                    L"C:\\$Windows.~BT",
                    L"C:\\$Windows.~WS"
                };
                IceClean::Core::Cleaner::FileCleaner fileCleaner;
                auto result = fileCleaner.Clean(oldWinPaths);
                totalFreed += result.totalCleanedSize;
            }

            // 更新已释放大小
            uint64_t freedSoFar = totalFreed;
            CallAfter([this, percent, freedSoFar]() {
                if (m_cleanProgressDlg) {
                    m_cleanProgressDlg->SetCleanedSize(freedSoFar);
                }
            });
        }

        // 处理隐私清理项
        std::vector<IceClean::Core::Cleaner::PrivacyType> privacyTypes;
        for (const auto& id : selectedIds) {
            if (id == L"cookies") {
                privacyTypes.push_back(IceClean::Core::Cleaner::PrivacyType::Cookies);
            }
            else if (id == L"history") {
                privacyTypes.push_back(IceClean::Core::Cleaner::PrivacyType::History);
            }
            else if (id == L"formData") {
                privacyTypes.push_back(IceClean::Core::Cleaner::PrivacyType::FormData);
            }
        }
        if (!privacyTypes.empty()) {
            CallAfter([this]() {
                UpdateCleanProgress(90, L"隐私清理", L"正在清理隐私数据...", 0);
            });
            IceClean::Core::Cleaner::PrivacyCleaner privacyCleaner;
            auto privacyResult = privacyCleaner.CleanPrivacy(privacyTypes, [this](const IceClean::Models::CleanProgress& progress) {
                int pct = progress.totalItems > 0 ? (progress.currentItem * 100 / progress.totalItems) : 0;
                wxString file(progress.currentFile);
                uint64_t cleaned = progress.cleanedSize;
                CallAfter([this, pct, file, cleaned]() {
                    UpdateCleanProgress(pct, L"隐私清理", file, cleaned);
                });
            });
            totalFreed += privacyResult.totalCleanedSize;
        }

        // 记录操作日志
        IceClean::Models::OperationRecord record;
        record.type = IceClean::Models::OperationType::Clean;
        record.description = L"深度清理";
        record.size = totalFreed;
        record.timestamp = std::chrono::system_clock::now();
        record.success = true;
        IceClean::Core::Safety::OperationLogger::LogOperation(record);

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_CLEAN_COMPLETE);
        completeEvt->SetInt(1);  // 1=深度清理
        completeEvt->SetPayload(totalFreed);
        wxQueueEvent(this, completeEvt);

        m_workerRunning = false;
    });

    m_workerThread.detach();
}

// ── 清理完成处理 ──

void MainWindow::OnCleanComplete(wxThreadEvent& event)
{
    int cleanType = event.GetInt();
    auto totalCleanedSize = event.GetPayload<uint64_t>();

    // 刷新磁盘信息并恢复仪表盘默认视图（隐藏扫描结果）
    RefreshDiskInfo();
    m_dashboardPanel->RestoreDiskInfo();

    // 刷新最近操作
    RefreshRecentOperations();

    if (cleanType == 2) {
        // 启动优化完成
        int disabledCount = static_cast<int>(totalCleanedSize);
        wxString msg = wxString::Format(L"启动优化完成！已禁用 %d 个项目。", disabledCount);
        CloseCleanProgress(totalCleanedSize);
        wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
        // 刷新启动项列表
        LoadStartupData();
    } else {
        // 清理完成 - 记录累计统计并显示结果
        if (totalCleanedSize > 0) {
            IceClean::Core::Safety::UsageStats::Instance().RecordClean(totalCleanedSize);
        }
        CloseCleanProgress(totalCleanedSize);
        // 刷新仪表盘累计统计
        m_dashboardPanel->LoadCumulativeStats();
    }
}

// ── 迁移请求处理 ──

void MainWindow::OnMigrateRequest(wxThreadEvent& event)
{
    auto items = m_migrationPanel->GetSelectedItems();
    auto targetDrive = m_migrationPanel->GetTargetDrive();

    if (items.empty() || targetDrive.IsEmpty()) return;

    StartMigration(items, targetDrive);
}

void MainWindow::StartMigration(const std::vector<IceClean::Models::MigrationItem>& items,
                                 const wxString& targetDrive)
{
    std::lock_guard<std::mutex> lock(m_workerMutex);
    if (m_workerRunning) return;
    m_workerRunning = true;

    // 创建还原点
    IceClean::Core::Safety::RestorePointManager::CreateRestorePoint(L"IceClean 迁移操作前自动还原点");

    auto targetDriveW = targetDrive.ToStdWstring();
    auto itemsCopy = items;

    m_workerThread = std::thread([this, itemsCopy, targetDriveW]() {
        uint64_t totalMigrated = 0;
        int successCount = 0;

        for (const auto& item : itemsCopy) {
            std::unique_ptr<IceClean::Core::Migrator::IMigrator> migrator;

            switch (item.type) {
                case IceClean::Models::MigrationType::SteamGame:
                    migrator = std::make_unique<IceClean::Core::Migrator::SteamMigrator>();
                    break;
                case IceClean::Models::MigrationType::UserFolder:
                    migrator = std::make_unique<IceClean::Core::Migrator::UserFolderMigrator>();
                    break;
                case IceClean::Models::MigrationType::WeChatCache:
                    migrator = std::make_unique<IceClean::Core::Migrator::WeChatMigrator>();
                    break;
                case IceClean::Models::MigrationType::QQCache:
                    migrator = std::make_unique<IceClean::Core::Migrator::QQMigrator>();
                    break;
                case IceClean::Models::MigrationType::DevCache:
                    migrator = std::make_unique<IceClean::Core::Migrator::DevCacheMigrator>();
                    break;
                case IceClean::Models::MigrationType::CustomFolder:
                case IceClean::Models::MigrationType::LargeSoftware:
                default:
                    migrator = std::make_unique<IceClean::Core::Migrator::FolderMigrator>(
                        item.sourcePath, item.name);
                    break;
            }

            auto result = migrator->Migrate({item}, targetDriveW);
            if (result.success) {
                totalMigrated += item.size;
                successCount++;
            }
        }

        // 记录操作日志
        IceClean::Models::OperationRecord record;
        record.type = IceClean::Models::OperationType::Migrate;
    record.description = wxString::Format(L"迁移 %d 个项目至 %s",
        successCount, targetDriveW.c_str()).ToStdWstring();
        record.size = totalMigrated;
        record.timestamp = std::chrono::system_clock::now();
        record.success = (successCount > 0);
        IceClean::Core::Safety::OperationLogger::LogOperation(record);

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_MIGRATE_COMPLETE);
        completeEvt->SetPayload(totalMigrated);
        wxQueueEvent(this, completeEvt);

        m_workerRunning = false;
    });

    m_workerThread.detach();
}

// ── 迁移完成处理 ──

void MainWindow::OnMigrateComplete(wxThreadEvent& event)
{
    auto migratedBytes = event.GetPayload<uint64_t>();

    // 刷新磁盘信息
    RefreshDiskInfo();
    RefreshRecentOperations();

    wxString msg = wxString::Format(L"迁移完成！共迁移 %s 数据。",
        IceClean::Utils::FormatUtil::FormatFileSize(migratedBytes).c_str());
    wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
}

// ── 启动优化 ──

void MainWindow::StartStartupOptimize(const std::vector<IceClean::Models::StartupItem>& modifiedStartup,
                                       const std::vector<IceClean::Models::StartupItem>& modifiedServices,
                                       const std::vector<IceClean::Models::StartupItem>& modifiedTasks)
{
    std::lock_guard<std::mutex> lock(m_workerMutex);
    if (m_workerRunning) return;
    m_workerRunning = true;

    m_workerThread = std::thread([this, modifiedStartup, modifiedServices, modifiedTasks]() {
        int disabledCount = 0;

        IceClean::Core::Optimizer::StartupOptimizer startupOpt;
        for (const auto& item : modifiedStartup) {
            if (!item.isEnabled) {
                if (startupOpt.DisableItem(item)) {
                    disabledCount++;
                }
            }
        }

        IceClean::Core::Optimizer::ServiceOptimizer serviceOpt;
        for (const auto& item : modifiedServices) {
            if (!item.isEnabled) {
                if (serviceOpt.DisableService(item.name)) {
                    disabledCount++;
                }
            }
        }

        // 禁用计划任务
        IceClean::Core::Optimizer::ScheduledTaskOptimizer taskOpt;
        for (const auto& item : modifiedTasks) {
            if (!item.isEnabled) {
                if (taskOpt.DisableTask(item.path, item.name)) {
                    disabledCount++;
                }
            }
        }

        // 记录操作日志
        IceClean::Models::OperationRecord record;
        record.type = IceClean::Models::OperationType::Optimize;
        record.description = wxString::Format(L"优化启动项，禁用 %d 项", disabledCount).ToStdWstring();
        record.size = 0;
        record.timestamp = std::chrono::system_clock::now();
        record.success = (disabledCount > 0);
        IceClean::Core::Safety::OperationLogger::LogOperation(record);

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_CLEAN_COMPLETE);
        completeEvt->SetInt(2);  // 2=启动优化
        completeEvt->SetPayload(static_cast<uint64_t>(disabledCount));
        wxQueueEvent(this, completeEvt);

        m_workerRunning = false;
    });

    m_workerThread.detach();
}

// ── 加载启动项数据 ──

void MainWindow::LoadStartupData()
{
    // 在后台线程加载启动项和服务
    std::thread([this]() {
        IceClean::Core::Optimizer::StartupOptimizer startupOpt;
        auto startupItems = startupOpt.GetStartupItems();

        IceClean::Core::Optimizer::ServiceOptimizer serviceOpt;
        auto serviceItems = serviceOpt.GetDisablableServices();

        // 在主线程更新UI
        CallAfter([this, startupItems = std::move(startupItems),
                          serviceItems = std::move(serviceItems)]() mutable {
            m_startupPanel->SetStartupItems(startupItems);
            m_startupPanel->SetServices(serviceItems);
        });
    }).detach();
}

// ── UI更新方法 ──

void MainWindow::RefreshDiskInfo()
{
    uint64_t totalBytes = 0, freeBytes = 0;
    if (IceClean::Utils::Win32Util::GetDiskSpace(L"C:\\", totalBytes, freeBytes)) {
        uint64_t usedBytes = totalBytes - freeBytes;
        m_dashboardPanel->UpdateDiskInfo(usedBytes, totalBytes);
    }
}

void MainWindow::RefreshRecentOperations()
{
    // 操作日志已移至设置面板，首页不再显示
}

// ── 清理进度对话框管理 ──

void MainWindow::ShowCleanProgress(const wxString& title) {
    if (m_cleanProgressDlg) {
        m_cleanProgressDlg->Destroy();
        m_cleanProgressDlg = nullptr;
    }
    m_cleanProgressDlg = new CleanProgressDialog(this);
    m_cleanProgressDlg->SetTitle(title);
    m_cleanProgressDlg->Show();
}

void MainWindow::UpdateCleanProgress(int percent, const wxString& category, const wxString& detail, uint64_t cleanedSize) {
    if (!m_cleanProgressDlg) return;
    m_cleanProgressDlg->SetProgress(percent);
    if (!category.empty()) {
        m_cleanProgressDlg->SetCategory(category);
    }
    if (!detail.empty()) {
        m_cleanProgressDlg->SetCurrentFile(detail);
    }
    if (cleanedSize > 0) {
        m_cleanProgressDlg->SetCleanedSize(cleanedSize);
    }
}

void MainWindow::CloseCleanProgress(uint64_t totalCleaned) {
    if (!m_cleanProgressDlg) return;
    m_cleanProgressDlg->SetFinished(totalCleaned);
    // SetFinished会将按钮改为"关闭"，用户点击后对话框自动关闭
    // 对话框关闭时自动销毁
    m_cleanProgressDlg->Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent&) {
        m_cleanProgressDlg = nullptr;
    });
}

// ── 无边框窗口的边框缩放支持 ──

#ifdef __WXMSW__
WXLRESULT MainWindow::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) {
    // WM_NCHITTEST: 让无边框窗口支持边框拖拽缩放
    if (nMsg == WM_NCHITTEST) {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        // 转换为窗口客户区坐标
        POINT pt = { x, y };
        ::ScreenToClient(GetHWND(), &pt);

        RECT rc;
        ::GetClientRect(GetHWND(), &rc);

        const int border = 6;  // 边框感应区域宽度

        // 判断鼠标在哪个边框区域
        bool onLeft   = pt.x < border;
        bool onRight  = pt.x > rc.right - border;
        bool onTop    = pt.y < border;
        bool onBottom = pt.y > rc.bottom - border;

        if (onTop && onLeft)     return HTTOPLEFT;
        if (onTop && onRight)    return HTTOPRIGHT;
        if (onBottom && onLeft)  return HTBOTTOMLEFT;
        if (onBottom && onRight) return HTBOTTOMRIGHT;
        if (onLeft)              return HTLEFT;
        if (onRight)             return HTRIGHT;
        if (onTop)               return HTTOP;
        if (onBottom)            return HTBOTTOM;
    }

    // WM_NCLBUTTONDBLCLK: 双击标题栏区域最大化/还原
    if (nMsg == WM_NCLBUTTONDBLCLK) {
        if (IsMaximized()) {
            Restore();
        } else {
            Maximize();
        }
        return 0;
    }

    return wxFrame::MSWWindowProc(nMsg, wParam, lParam);
}
#endif

// ── 键盘快捷键 ──

void MainWindow::OnKeyDown(wxKeyEvent& event) {
    if (!event.ControlDown()) {
        event.Skip();
        return;
    }

    switch (event.GetKeyCode()) {
        case 'S': {
            wxThreadEvent scanEvent(wxEVT_SCAN_REQUEST);
            scanEvent.SetInt(0);
            wxQueueEvent(this, new wxThreadEvent(scanEvent));
            break;
        }
        case 'C': {
            wxThreadEvent stopEvent(wxEVT_SCAN_STOP);
            stopEvent.SetInt(0);
            wxQueueEvent(this, new wxThreadEvent(stopEvent));
            break;
        }
        case '1': SwitchPanel(static_cast<int>(NavPage::Dashboard)); break;
        case '2': SwitchPanel(static_cast<int>(NavPage::DeepClean)); break;
        case '3': SwitchPanel(static_cast<int>(NavPage::Migration)); break;
        case '4': SwitchPanel(static_cast<int>(NavPage::Startup)); break;
        case '5': SwitchPanel(static_cast<int>(NavPage::SoftwareManage)); break;
        case '6': SwitchPanel(static_cast<int>(NavPage::SoftwareRecommend)); break;
        case '7': SwitchPanel(static_cast<int>(NavPage::Security)); break;
        case '8': SwitchPanel(static_cast<int>(NavPage::NetworkOpt)); break;
        case '9': SwitchPanel(static_cast<int>(NavPage::DiskAnalyzer)); break;
        case '0': SwitchPanel(static_cast<int>(NavPage::Settings)); break;
        default:
            event.Skip();
            break;
    }
}

// ── 通用操作进度/完成处理（Debloat / Privacy / 系统文件等） ──

void MainWindow::OnOperationProgressUpdate(wxThreadEvent& event) {
    auto snapshot = event.GetPayload<IceClean::Core::Utils::ProgressSnapshot>();

    if (m_cleanProgressDlg) {
        UnifiedProgressData data;
        data.percent = snapshot.percent;
        data.stage = snapshot.stage;
        data.detail = snapshot.detail;
        data.subPercent = snapshot.subPercent;
        data.subDetail = snapshot.subDetail;
        data.processedBytes = snapshot.processedBytes;
        data.totalBytes = snapshot.totalBytes;
        data.processedItems = snapshot.processedItems;
        data.totalItems = snapshot.totalItems;
        data.speedBytesPerSec = snapshot.speedBytesPerSec;

        static_cast<UnifiedProgressDialog*>(m_cleanProgressDlg)->UpdateData(data);
    }
}

void MainWindow::OnOperationComplete(wxThreadEvent& event) {
    bool success = event.GetInt() != 0;
    auto summary = event.GetPayload<wxString>();

    if (m_cleanProgressDlg) {
        static_cast<UnifiedProgressDialog*>(m_cleanProgressDlg)->SetFinished(success, summary);
        m_cleanProgressDlg->Bind(wxEVT_DESTROY, [this](wxWindowDestroyEvent&) {
            m_cleanProgressDlg = nullptr;
        });
    }

    // 刷新磁盘信息
    RefreshDiskInfo();
    RefreshRecentOperations();
    SetStatusIdle();
}

// ── 状态栏方法 ──

void MainWindow::SetStatusText(const wxString& text, int field) {
    if (m_statusBar) {
        m_statusBar->SetStatusText(text, field);
    }
}

void MainWindow::SetStatusBusy(const wxString& task) {
    if (m_statusBar) {
        m_statusBar->SetStatusText(task, 0);
        m_statusBar->SetStatusText(L"⏳ 运行中...", 1);
    }
}

void MainWindow::SetStatusIdle() {
    if (m_statusBar) {
        m_statusBar->SetStatusText(L"就绪", 0);

        // 显示上次操作时间
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        tm local;
        localtime_s(&local, &now);
        wxString timeStr = wxString::Format(L"上次操作: %02d:%02d:%02d",
            local.tm_hour, local.tm_min, local.tm_sec);
        m_statusBar->SetStatusText(timeStr, 1);
    }
}

} // namespace IceClean::Gui
