#pragma once
#include <wx/wx.h>
#include <vector>
#include "models/ScanResult.h"

namespace IceClean::Gui {

// 前向声明
class DeepCleanPanel;

// 清理面板（深度清理）
class ScanResultPanel : public wxPanel {
public:
    ScanResultPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    ~ScanResultPanel() override;

    // 设置扫描结果数据（保留接口兼容性，实际由首页处理）
    void SetScanResult(const IceClean::Models::ScanResult& result);

    // 获取选中的清理项路径列表（保留接口兼容性）
    std::vector<std::wstring> GetSelectedPaths() const;

    // 获取选中的清理总大小（保留接口兼容性）
    uint64_t GetSelectedSize() const;

    // 获取深度清理面板指针
    DeepCleanPanel* GetDeepCleanPanel() const { return m_deepCleanPanel; }

private:
    IceClean::Models::ScanResult m_result;

    // 深度清理面板
    DeepCleanPanel* m_deepCleanPanel = nullptr;

    void CreateControls();

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
