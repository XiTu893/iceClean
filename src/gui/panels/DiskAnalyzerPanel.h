#pragma once
#include <wx/wx.h>
#include <memory>
#include <vector>
#include "models/DiskNode.h"

namespace IceClean::Gui {

// 磁盘分析面板
class DiskAnalyzerPanel : public wxPanel {
public:
    DiskAnalyzerPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 设置磁盘分析数据
    void SetDiskData(std::shared_ptr<IceClean::Models::DiskNode> root);

    // 获取选中的驱动器
    wxString GetSelectedDrive() const;

private:
    std::shared_ptr<IceClean::Models::DiskNode> m_rootNode;

    // 控件
    wxChoice* m_driveChoice = nullptr;
    wxButton* m_scanButton = nullptr;
    wxPanel* m_treemapPanel = nullptr;
    wxCheckListBox* m_typeFilter = nullptr;
    wxStaticText* m_statusLabel = nullptr;

    // 矩形树图数据
    struct TreemapRect {
        std::shared_ptr<IceClean::Models::DiskNode> node;
        int x, y, width, height;
        wxColour color;
    };
    std::vector<TreemapRect> m_treemapRects;
    int m_hoveredRect = -1;

    void CreateControls();

    // 矩形树图绘制
    void OnTreemapPaint(wxPaintEvent& event);
    void OnTreemapMouseMotion(wxMouseEvent& event);
    void OnTreemapLeftClick(wxMouseEvent& event);
    void OnTreemapRightClick(wxMouseEvent& event);

    // 矩形树图算法
    void BuildTreemap();
    void Squarify(std::shared_ptr<IceClean::Models::DiskNode> node,
                  int x, int y, int w, int h);
    void LayoutRow(std::vector<std::shared_ptr<IceClean::Models::DiskNode>>& row,
                   int x, int y, int w, int h, bool horizontal);

    // 颜色映射
    wxColour GetNodeColor(const std::shared_ptr<IceClean::Models::DiskNode>& node) const;

    // 事件处理
    void OnScanButton(wxCommandEvent& event);
    void OnDriveChoice(wxCommandEvent& event);
    void OnTypeFilter(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
