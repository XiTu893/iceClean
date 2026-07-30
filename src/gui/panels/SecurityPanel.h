#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <vector>
#include <memory>
#include <thread>
#include "models/PopupBlockerInfo.h"
#include "models/BrowserProtectionInfo.h"
#include "models/FileWatchInfo.h"
#include "core/safety/MalwareDetector.h"

namespace IceClean::Core::Safety { class FileWatcher; }

namespace IceClean::Gui {

// 安全防护面板（包含弹窗拦截 + 浏览器保护）
class SecurityPanel : public wxPanel {
public:
    SecurityPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    ~SecurityPanel() override;

    // 刷新数据
    void RefreshData();

private:
    wxNotebook* m_notebook = nullptr;

    // ── 弹窗拦截相关 ──
    std::vector<IceClean::Models::PopupBlockerItem> m_popupItems;
    IceClean::Models::PopupBlockerStats m_popupStats;

    wxPanel* m_popupPage = nullptr;
    wxStaticText* m_totalBlockedLabel = nullptr;
    wxStaticText* m_todayBlockedLabel = nullptr;
    wxStaticText* m_rulesCountLabel = nullptr;
    wxCheckBox* m_enablePopupCheckBox = nullptr;
    wxListCtrl* m_popupListCtrl = nullptr;
    wxButton* m_refreshPopupButton = nullptr;
    wxButton* m_blockButton = nullptr;
    wxButton* m_unblockButton = nullptr;
    wxButton* m_blockAllButton = nullptr;
    wxStaticText* m_popupStatusLabel = nullptr;

    // ── 浏览器保护相关 ──
    std::vector<IceClean::Models::BrowserProtectionItem> m_browserItems;

    wxPanel* m_browserPage = nullptr;
    wxListCtrl* m_browserListCtrl = nullptr;
    wxButton* m_scanBrowserButton = nullptr;
    wxButton* m_lockHomePageButton = nullptr;
    wxButton* m_restoreHomePageButton = nullptr;
    wxCheckBox* m_lockHomePageCheckBox = nullptr;
    wxCheckBox* m_lockSearchEngineCheckBox = nullptr;
    wxTextCtrl* m_homePageTextCtrl = nullptr;
    wxStaticText* m_browserStatusLabel = nullptr;

    // ── 文件监控相关 ──
    std::unique_ptr<IceClean::Core::Safety::FileWatcher> m_fileWatcher;
    std::vector<IceClean::Models::FileWatchConfig> m_watchConfigs;

    wxPanel* m_fileWatchPage = nullptr;
    wxListCtrl* m_fileWatchListCtrl = nullptr;
    wxButton* m_startWatchButton = nullptr;
    wxButton* m_stopWatchButton = nullptr;
    wxButton* m_addWatchPathButton = nullptr;
    wxButton* m_removeWatchPathButton = nullptr;
    wxListBox* m_watchPathListBox = nullptr;
    wxStaticText* m_fileWatchStatusLabel = nullptr;
    wxStaticText* m_totalChangesLabel = nullptr;
    wxStaticText* m_todayChangesLabel = nullptr;

    // ── 启动保护相关 ──
    wxPanel* m_startupProtectPage = nullptr;
    wxListCtrl* m_startupChangeListCtrl = nullptr;
    wxButton* m_buildBaselineButton = nullptr;
    wxButton* m_detectStartupChangeButton = nullptr;
    wxCheckBox* m_lockStartupCheckBox = nullptr;
    wxStaticText* m_startupProtectStatusLabel = nullptr;

    // ── 恶意软件检测相关 ──
    wxPanel* m_malwareScanPage = nullptr;
    wxListCtrl* m_malwareListCtrl = nullptr;
    wxButton* m_fullScanButton = nullptr;
    wxStaticText* m_malwareScanStatusLabel = nullptr;
    wxGauge* m_scanProgressGauge = nullptr;

    // ── 后台工作线程 ──
    std::thread m_workerThread;

    // ── 自定义ID ──
    enum {
        ID_POPUP_LIST = wxID_HIGHEST + 200,
        ID_REFRESH_POPUP,
        ID_BLOCK,
        ID_UNBLOCK,
        ID_BLOCK_ALL,
        ID_ENABLE_POPUP,
        ID_BROWSER_LIST,
        ID_SCAN_BROWSER,
        ID_LOCK_HOME,
        ID_RESTORE_HOME,
        ID_LOCK_HOME_CHECK,
        ID_LOCK_SEARCH_CHECK,
        ID_HOME_PAGE_TEXT,
        ID_FILE_WATCH_LIST,
        ID_START_WATCH,
        ID_STOP_WATCH,
        ID_ADD_WATCH_PATH,
        ID_REMOVE_WATCH_PATH,
        ID_WATCH_PATH_LIST,
        ID_STARTUP_CHANGE_LIST,
        ID_BUILD_BASELINE,
        ID_DETECT_STARTUP_CHANGE,
        ID_LOCK_STARTUP,
        ID_MALWARE_LIST,
        ID_FULL_SCAN
    };

    void CreatePopupPage();
    void CreateBrowserPage();
    void CreateFileWatchPage();
    void CreateStartupProtectPage();
    void CreateMalwareScanPage();

    // 弹窗拦截事件
    void OnRefreshPopup(wxCommandEvent& event);
    void OnBlock(wxCommandEvent& event);
    void OnUnblock(wxCommandEvent& event);
    void OnBlockAll(wxCommandEvent& event);
    void OnEnablePopupToggle(wxCommandEvent& event);
    void OnPopupItemSelected(wxListEvent& event);
    void OnPopupItemActivated(wxListEvent& event);

    // 浏览器保护事件
    void OnScanBrowser(wxCommandEvent& event);
    void OnLockHomePage(wxCommandEvent& event);
    void OnRestoreHomePage(wxCommandEvent& event);
    void OnLockHomeCheckToggle(wxCommandEvent& event);
    void OnLockSearchCheckToggle(wxCommandEvent& event);

    // 文件监控事件
    void OnStartWatch(wxCommandEvent& event);
    void OnStopWatch(wxCommandEvent& event);
    void OnAddWatchPath(wxCommandEvent& event);
    void OnRemoveWatchPath(wxCommandEvent& event);
    void OnFileChangeReceived(wxThreadEvent& event);

    // 启动保护事件
    void OnBuildBaseline(wxCommandEvent& event);
    void OnDetectStartupChange(wxCommandEvent& event);
    void OnLockStartupToggle(wxCommandEvent& event);

    // 恶意软件检测事件
    void OnFullScan(wxCommandEvent& event);

    // 辅助方法
    void LoadPopupItems();
    void UpdatePopupStats();
    void LoadBrowserItems();
    void LoadWatchConfigs();
    void UpdateFileWatchStats();
    int GetSelectedPopupIndex() const;
    int GetSelectedBrowserIndex() const;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
