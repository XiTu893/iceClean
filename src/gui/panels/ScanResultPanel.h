#pragma once
#include <wx/wx.h>
#include <vector>
#include "models/ScanResult.h"

namespace IceClean::Gui {

// 扫描结果面板
class ScanResultPanel : public wxPanel {
public:
    ScanResultPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 设置扫描结果数据
    void SetScanResult(const IceClean::Models::ScanResult& result);

    // 获取选中的清理项路径列表
    std::vector<std::wstring> GetSelectedPaths() const;

    // 获取选中的清理总大小
    uint64_t GetSelectedSize() const;

private:
    IceClean::Models::ScanResult m_result;

    // 控件
    wxStaticText* m_durationLabel = nullptr;
    wxScrolledWindow* m_categoryScroller = nullptr;
    wxBoxSizer* m_categorySizer = nullptr;
    wxStaticText* m_totalSizeLabel = nullptr;
    wxButton* m_cleanButton = nullptr;

    // 每个分类的UI数据
    struct CategoryUI {
        wxPanel* headerPanel = nullptr;
        wxPanel* detailPanel = nullptr;
        wxStaticText* arrowLabel = nullptr;
        wxStaticText* sizeLabel = nullptr;
        wxCheckBox* categoryCheck = nullptr;
        bool expanded = false;
        std::vector<wxCheckBox*> itemChecks;
    };
    std::vector<CategoryUI> m_categoryUIs;

    void CreateControls();
    void BuildCategoryList();
    void UpdateTotalSize();

    // 事件处理
    void OnCategoryHeaderClick(wxMouseEvent& event);
    void OnCategoryCheck(wxCommandEvent& event);
    void OnItemCheck(wxCommandEvent& event);
    void OnCleanButton(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
