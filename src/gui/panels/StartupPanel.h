#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <vector>
#include "models/StartupItem.h"
#include "core/analyzer/ProcessAnalyzer.h"
#include "core/optimizer/ScheduledTaskOptimizer.h"

namespace IceClean::Gui {

// 启动优化面板
class StartupPanel : public wxPanel {
public:
    StartupPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 设置启动项列表
    void SetStartupItems(const std::vector<IceClean::Models::StartupItem>& items);

    // 设置服务列表
    void SetServices(const std::vector<IceClean::Models::StartupItem>& services);

    // 获取已修改的启动项
    std::vector<IceClean::Models::StartupItem> GetModifiedStartupItems() const;

    // 获取已修改的服务
    std::vector<IceClean::Models::StartupItem> GetModifiedServices() const;

    // 获取已修改的计划任务
    std::vector<IceClean::Models::StartupItem> GetModifiedScheduledTasks() const;

private:
    std::vector<IceClean::Models::StartupItem> m_startupItems;
    std::vector<IceClean::Models::StartupItem> m_services;
    std::vector<IceClean::Models::StartupItem> m_scheduledTasks;
    // 保存原始状态，用于检测哪些项被修改
    std::vector<bool> m_startupOriginalState;
    std::vector<bool> m_serviceOriginalState;
    std::vector<bool> m_scheduledTaskOriginalState;

    // 进程列表
    std::vector<IceClean::Core::Analyzer::ProcessInfo> m_processes;

    // 进程分组
    struct ProcessGroup {
        std::wstring name;
        std::wstring companyName;
        uint64_t totalMemory = 0;
        int instanceCount = 0;
        bool isSystemProcess = false;
        IceClean::Core::Analyzer::ProcessSafety safety;
        std::vector<size_t> processIndices;  // indices into m_processes
    };
    std::vector<ProcessGroup> m_processGroups;

    // 控件
    wxNotebook* m_notebook = nullptr;
    wxScrolledWindow* m_startupScroller = nullptr;
    wxBoxSizer* m_startupSizer = nullptr;
    wxScrolledWindow* m_serviceScroller = nullptr;
    wxBoxSizer* m_serviceSizer = nullptr;
    wxScrolledWindow* m_taskScroller = nullptr;
    wxBoxSizer* m_taskSizer = nullptr;
    wxScrolledWindow* m_processScroller = nullptr;
    wxBoxSizer* m_processSizer = nullptr;
    wxListCtrl* m_processList = nullptr;
    int m_processSortColumn = -1;
    bool m_processSortAsc = true;
    wxStaticText* m_bootTimeLabel = nullptr;
    wxButton* m_optimizeButton = nullptr;
    wxButton* m_refreshProcessButton = nullptr;

    // Toggle开关控件数据
    struct ToggleItem {
        wxPanel* rowPanel = nullptr;
        wxPanel* togglePanel = nullptr;
        wxStaticText* nameLabel = nullptr;
        wxStaticText* publisherLabel = nullptr;
        wxStaticText* typeLabel = nullptr;
        wxStaticText* statusLabel = nullptr;  // 运行状态标签
        bool isOn = true;
        bool isSystemCritical = false;
        bool isProcessRunning = false;  // 关联进程是否正在运行
        int itemIndex = -1; // 在m_startupItems或m_services中的索引
    };
    std::vector<ToggleItem> m_startupToggles;
    std::vector<ToggleItem> m_serviceToggles;
    std::vector<ToggleItem> m_taskToggles;

    void CreateControls();
    void BuildStartupList();
    void BuildServiceList();
    void BuildScheduledTaskList();
    void LoadScheduledTasks();
    void BuildProcessList();
    void LoadProcessList();
    void SortProcessGroups();
    void UpdateBootTimeEstimate();

    // 自绘Toggle开关
    wxPanel* CreateToggleSwitch(wxWindow* parent, bool isOn, bool canToggle);
    void DrawToggle(wxPanel* panel, bool isOn, bool canToggle);
    void OnToggleClick(wxMouseEvent& event);

    void OnOptimizeButton(wxCommandEvent& event);
    void OnRefreshProcess(wxCommandEvent& event);
    void OnKillProcess(wxListEvent& event);
    void OnNotebookPageChanged(wxBookCtrlEvent& event);
    void OnProcessColumnClick(wxListEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
