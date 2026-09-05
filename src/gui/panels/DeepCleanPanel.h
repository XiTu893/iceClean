#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/scrolwin.h>
#include <wx/combobox.h>
#include <vector>
#include <atomic>
#include <memory>
#include <thread>
#include "core/cleaner/RegistryCleaner.h"
#include "core/scanner/SoftwareCacheScanner.h"
#include "gui/controls/CircularProgress.h"
#include "gui/controls/ScanProgressBar.h"
#include "gui/controls/ScanInfoPanel.h"
#include "gui/controls/PauseOverlay.h"
#include "models/ScanResult.h"

namespace IceClean::Gui {

// 深度清理面板 - 2 个主按钮 + 高级面板 + 4 标签页
class DeepCleanPanel : public wxPanel {
public:
    DeepCleanPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 获取选中的深度清理项ID列表
    std::vector<wxString> GetSelectedIds() const;

private:
    // UI 控件 - 顶部概览
    IceClean::Gui::CircularProgress* m_quickRing = nullptr;
    wxStaticText* m_quickSizeLabel = nullptr;
    wxStaticText* m_quickCountLabel = nullptr;
    wxStaticText* m_quickDangerLabel = nullptr;
    IceClean::Gui::ScanProgressBar* m_scanProgressBar = nullptr;
    IceClean::Gui::ScanInfoPanel* m_scanInfoPanel = nullptr;
    IceClean::Gui::PauseOverlay* m_pauseOverlay = nullptr;
    wxButton* m_quickScanButton = nullptr;
    wxButton* m_quickCleanButton = nullptr;
    wxButton* m_pauseButton = nullptr;
    wxButton* m_stopScanButton = nullptr;
    wxButton* m_advancedToggleButton = nullptr;
    wxButton* m_cleanButton = nullptr;

    // 高级面板
    wxPanel* m_advancedPanel = nullptr;
    bool m_advancedVisible = false;

    // 状态
    int m_quickResCount = 0;
    int m_quickDangerCount = 0;
    uint64_t m_quickEstSize = 0;
    bool m_quickMemoryEnabled = true;
    bool m_isPaused = false;
    std::atomic<bool> m_pauseRequested{false};
    std::atomic<bool> m_scanCancelled{false};

    // 笔记本
    wxNotebook* m_notebook = nullptr;

    // 顶部概览卡片（一键扫描/清理）
    struct QuickCleanItem {
        wxString id;
        wxString name;
        wxString detail;
        IceClean::Models::SafetyRating safety = IceClean::Models::SafetyRating::Safe;
        wxCheckBox* check = nullptr;
        wxStaticText* sizeLabel = nullptr;
    };
    std::vector<QuickCleanItem> m_quickItems;
    QuickCleanItem* FindQuickItem(const wxString& id);

    // 系统清理标签页项
    struct SystemItem {
        wxCheckBox* checkbox = nullptr;
        wxString id;
        wxString description;
        bool isDangerous = false;
    };
    std::vector<SystemItem> m_systemItems;
    wxCheckBox* m_systemSelectAllCheck = nullptr;

    // 隐私清理标签页项
    struct PrivacyItem {
        wxCheckBox* checkbox = nullptr;
        wxString id;
        wxString description;
        bool isDangerous = false;
    };
    std::vector<PrivacyItem> m_privacyItems;
    wxCheckBox* m_privacySelectAllCheck = nullptr;

    // 注册表清理标签页
    wxButton* m_registryScanButton = nullptr;
    wxButton* m_registryCleanButton = nullptr;
    wxCheckBox* m_registrySelectAllCheck = nullptr;
    wxListCtrl* m_registryList = nullptr;
    wxStaticText* m_registryStatusLabel = nullptr;
    std::vector<IceClean::Core::Cleaner::RegistryInvalidItem> m_registryItems;
    std::vector<bool> m_registryChecked;

    // 软件专清标签页
    wxButton* m_softwareScanButton = nullptr;
    wxButton* m_softwareCleanButton = nullptr;
    wxCheckBox* m_softwareSelectAllCheck = nullptr;
    wxStaticText* m_softwareStatusLabel = nullptr;
    struct SoftwareCacheItem {
        wxCheckBox* checkbox = nullptr;
        wxString name;
        std::wstring cachePath;
        wxStaticText* sizeLabel = nullptr;
        uint64_t cacheSize = 0;
    };
    std::vector<SoftwareCacheItem> m_softwareCacheItems;
    IceClean::Core::Scanner::SoftwareCacheScanner m_softwareCacheScanner;
    IceClean::Models::ScanCategory m_softwareScanResult;

    // 按钮颜色数据（用于主题切换时刷新）
    struct ButtonColorPair {
        wxButton* btn = nullptr;
        wxColour normalBg;
        wxColour hoverBg;
    };
    std::vector<ButtonColorPair> m_buttonColors;

    // 方法
    void CreateControls();
    void CreateQuickOverview(wxSizer* mainSizer);
    void CreateAdvancedPanel(wxSizer* mainSizer);
    void CreateSystemCleanTab(wxWindow* parent);
    void CreateRegistryCleanTab(wxWindow* parent);
    void CreatePrivacyCleanTab(wxWindow* parent);
    void CreateSoftwareCacheTab(wxWindow* parent);

    // 主题切换时刷新按钮颜色
    void RefreshButtonColors();

    // 记录按钮的悬停配色（用于主题切换时重新计算悬停色）
    void TrackButtonColor(wxButton* btn, const wxColour& normalBg, const wxColour& hoverBg) {
        m_buttonColors.push_back({btn, normalBg, hoverBg});
    }

    // 事件
    void OnCleanButton(wxCommandEvent& event);
    void OnQuickScan(wxCommandEvent& event);
    void OnQuickClean(wxCommandEvent& event);
    void OnPauseButton(wxCommandEvent& event);
    void OnStopScanButton(wxCommandEvent& event);
    void OnAdvancedToggle(wxCommandEvent& event);

    void OnRegistryScan(wxCommandEvent& event);
    void OnRegistryClean(wxCommandEvent& event);
    void OnSoftwareScan(wxCommandEvent& event);
    void OnSoftwareClean(wxCommandEvent& event);

    void OnSystemSelectAll(wxCommandEvent& event);
    void OnPrivacySelectAll(wxCommandEvent& event);
    void OnRegistrySelectAll(wxCommandEvent& event);
    void OnSoftwareSelectAll(wxCommandEvent& event);

    // 工具
    void RefreshRegistryList();
    std::wstring GenerateRegistryBackupPath();
    void UpdateTabBadges();
    void UpdateQuickOverview(bool updateRing = true);
    void WaitIfPaused();
    void FinishQuickScan();
    int ComputeHealthScore() const;
    std::wstring GetQuickPreferencesPath() const;
    void ApplyQuickPreferences(bool save);

    wxString GetTypeString(IceClean::Core::Cleaner::RegistryInvalidItem::Type type) const;

    template <typename Items>
    static void UpdateSelectAllChecked(const Items& items, wxCheckBox* selectAll, bool onlyEnabled = false) {
        bool any = false;
        bool all = true;
        for (const auto& item : items) {
            if (onlyEnabled && !item.checkbox->IsEnabled()) {
                continue;
            }
            any = true;
            if (!item.checkbox->GetValue()) {
                all = false;
                break;
            }
        }
        selectAll->SetValue(any && all);
    }

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
