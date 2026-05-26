#include "MainWindow.h"
#include "controls/NavSidebar.h"
#include "panels/DashboardPanel.h"
#include "panels/ScanResultPanel.h"
#include "panels/DeepCleanPanel.h"
#include "panels/MigrationPanel.h"
#include "panels/StartupPanel.h"
#include "panels/DiskAnalyzerPanel.h"
#include "panels/SettingsPanel.h"
#include "gui/Events.h"

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
#include "core/optimizer/StartupOptimizer.h"
#include "core/optimizer/ServiceOptimizer.h"
#include "core/analyzer/DiskSpaceAnalyzer.h"
#include "core/safety/RestorePointManager.h"
#include "core/safety/OperationLogger.h"
#include "utils/Win32Util.h"
#include "utils/FormatUtil.h"

#include <wx/dcbuffer.h>
#include <wx/busyinfo.h>
#include <wx/artprov.h>

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
              wxDEFAULT_FRAME_STYLE)
{
    SetBackgroundColour(*wxWHITE);
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

    // 延迟初始化（等窗口显示后）
    CallAfter([this]() { InitializeApp(); });
}

MainWindow::~MainWindow() {
    // 确保工作线程结束
    if (m_workerThread.joinable()) {
        m_workerRunning = false;
        m_workerThread.detach();
    }
}

// ── Control creation ──

void MainWindow::CreateControls()
{
    // Create sidebar
    m_sidebar = new NavSidebar(this);

    // Create content area (simplebook for panel switching)
    m_contentBook = new wxSimplebook(this, wxID_ANY);
    m_contentBook->SetBackgroundColour(*wxWHITE);

    // Create all panels and add to simplebook
    m_dashboardPanel = new DashboardPanel(m_contentBook);
    m_scanResultPanel = new ScanResultPanel(m_contentBook);
    m_deepCleanPanel = new DeepCleanPanel(m_contentBook);
    m_migrationPanel = new MigrationPanel(m_contentBook);
    m_startupPanel = new StartupPanel(m_contentBook);
    m_diskAnalyzerPanel = new DiskAnalyzerPanel(m_contentBook);
    m_settingsPanel = new SettingsPanel(m_contentBook);

    m_contentBook->AddPage(m_dashboardPanel, L"首页");
    m_contentBook->AddPage(m_scanResultPanel, L"清理");
    m_contentBook->AddPage(m_deepCleanPanel, L"深度清理");
    m_contentBook->AddPage(m_migrationPanel, L"迁移");
    m_contentBook->AddPage(m_startupPanel, L"加速");
    m_contentBook->AddPage(m_diskAnalyzerPanel, L"分析");
    m_contentBook->AddPage(m_settingsPanel, L"设置");

    // Bind sidebar selection change event
    m_sidebar->Bind(wxEVT_NAV_SELECTION_CHANGED, [this](wxCommandEvent& event) {
        SwitchPanel(event.GetInt());
    });
}

void MainWindow::LayoutControls()
{
    auto* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    // Sidebar: fixed 200px width, full height
    mainSizer->Add(m_sidebar, 0, wxEXPAND);

    // Separator line between sidebar and content
    auto* separator = new wxPanel(this, wxID_ANY);
    separator->SetBackgroundColour(wxColour(230, 230, 230));
    separator->SetSize(1, -1);
    separator->SetMinSize(wxSize(1, 1));
    mainSizer->Add(separator, 0, wxEXPAND);

    // Content area: expand to fill remaining space
    mainSizer->Add(m_contentBook, 1, wxEXPAND);

    SetSizer(mainSizer);
}

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
}

// ── Panel switching ──

void MainWindow::SwitchPanel(int index)
{
    if (index >= 0 && index < m_contentBook->GetPageCount()) {
        m_contentBook->SetSelection(index);

        // 切换到启动面板时刷新数据
        if (index == 4) {
            LoadStartupData();
        }
        // 切换到设置面板时刷新日志
        else if (index == 6) {
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

void MainWindow::OnScanStop(wxThreadEvent& event)
{
    // 请求当前扫描器停止
    std::lock_guard<std::mutex> lock(m_aggregatorMutex);
    if (m_currentAggregator) {
        m_currentAggregator->RequestStop();
    }
}

void MainWindow::StartScan(int scanType)
{
    std::lock_guard<std::mutex> lock(m_workerMutex);
    if (m_workerRunning) return;

    m_workerRunning = true;

    if (scanType == 0) {
        // 普通扫描 - 更新仪表盘状态
        m_dashboardPanel->SetScanning(true);
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

            IceClean::Core::Analyzer::DiskSpaceAnalyzer analyzer;
            auto rootNode = analyzer.Scan(drive);

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
    int scanType = event.GetInt();

    if (scanType == 0) {
        // 普通扫描完成
        m_dashboardPanel->SetScanning(false);
        m_dashboardPanel->RestoreDiskInfo();  // 恢复磁盘信息显示

        auto result = event.GetPayload<IceClean::Models::ScanResult>();
        m_lastScanResult = result;

        // 将结果传递给扫描结果面板
        m_scanResultPanel->SetScanResult(result);

        // 如果有结果（包括被停止的部分结果），切换到结果面板
        if (!result.categories.empty()) {
            SwitchPanel(1);
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
        // 普通清理 - 从扫描结果面板获取选中路径
        auto paths = m_scanResultPanel->GetSelectedPaths();
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
        StartStartupOptimize(modifiedStartup, modifiedServices);
    }
}

void MainWindow::StartClean(int cleanType, const std::vector<std::wstring>& paths)
{
    std::lock_guard<std::mutex> lock(m_workerMutex);
    if (m_workerRunning) return;
    m_workerRunning = true;

    // 创建还原点
    IceClean::Core::Safety::RestorePointManager::CreateRestorePoint(L"IceClean 清理操作前自动还原点");

    m_workerThread = std::thread([this, cleanType, paths]() {
        uint64_t totalFreed = 0;

        // 文件清理
        IceClean::Core::Cleaner::FileCleaner fileCleaner;
        auto fileResult = fileCleaner.Clean(paths, [](const IceClean::Models::CleanProgress& progress) {
            // 进度回调
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

    // 创建还原点
    IceClean::Core::Safety::RestorePointManager::CreateRestorePoint(L"IceClean 深度清理前自动还原点");

    m_workerThread = std::thread([this, selectedIds]() {
        uint64_t totalFreed = 0;

        for (const auto& id : selectedIds) {
            if (id == L"winSxS") {
                IceClean::Core::Cleaner::DismCleaner dismCleaner;
                auto result = dismCleaner.Clean({L"WinSxS"});
                totalFreed += result.totalCleanedSize;
            }
            else if (id == L"compactOS") {
                IceClean::Core::Cleaner::DismCleaner dismCleaner;
                auto result = dismCleaner.Clean({L"CompactOS"});
                totalFreed += result.totalCleanedSize;
            }
            else if (id == L"hibernation") {
                IceClean::Core::Cleaner::HibernationCleaner hibCleaner;
                auto result = hibCleaner.Clean({});
                totalFreed += result.totalCleanedSize;
            }
            else if (id == L"oldWindows") {
                // 删除旧Windows安装文件
                std::vector<std::wstring> oldWinPaths = {
                    L"C:\\Windows.old",
                    L"C:\\$Windows.~BT",
                    L"C:\\$Windows.~WS"
                };
                IceClean::Core::Cleaner::FileCleaner fileCleaner;
                auto result = fileCleaner.Clean(oldWinPaths);
                totalFreed += result.totalCleanedSize;
            }
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
            IceClean::Core::Cleaner::PrivacyCleaner privacyCleaner;
            auto privacyResult = privacyCleaner.CleanPrivacy(privacyTypes, [](const IceClean::Models::CleanProgress& progress) {
                // 进度回调
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

    // 刷新磁盘信息
    RefreshDiskInfo();

    // 刷新最近操作
    RefreshRecentOperations();

    // 显示结果
    wxString msg = wxString::Format(L"清理完成！共释放 %s 空间。",
        IceClean::Utils::FormatUtil::FormatFileSize(totalCleanedSize));
    wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
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
            successCount, targetDriveW).ToStdWstring();
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
        IceClean::Utils::FormatUtil::FormatFileSize(migratedBytes));
    wxMessageBox(msg, L"IceClean", wxOK | wxICON_INFORMATION, this);
}

// ── 启动优化 ──

void MainWindow::StartStartupOptimize(const std::vector<IceClean::Models::StartupItem>& modifiedStartup,
                                       const std::vector<IceClean::Models::StartupItem>& modifiedServices)
{
    std::lock_guard<std::mutex> lock(m_workerMutex);
    if (m_workerRunning) return;
    m_workerRunning = true;

    m_workerThread = std::thread([this, modifiedStartup, modifiedServices]() {
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
    auto records = IceClean::Core::Safety::OperationLogger::GetRecentOperations(20);
    m_dashboardPanel->UpdateRecentOperations(records);
}

} // namespace IceClean::Gui
