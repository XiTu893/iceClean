#include "DiskAnalyzerPanel.h"
#include "gui/Events.h"
#include "utils/FormatUtil.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shellapi.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(DiskAnalyzerPanel, wxPanel)
wxEND_EVENT_TABLE()

DiskAnalyzerPanel::DiskAnalyzerPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(*wxWHITE);
    CreateControls();
}

void DiskAnalyzerPanel::CreateControls() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"磁盘分析");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 驱动器选择 + 扫描按钮
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* driveLabel = new wxStaticText(this, wxID_ANY, L"选择驱动器:");
    driveLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    topSizer->Add(driveLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 20);

    m_driveChoice = new wxChoice(this, wxID_ANY);
    m_driveChoice->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                  false, L"微软雅黑"));
    // 枚举驱动器
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (drives & (1 << i)) {
            wchar_t root[] = {static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0'};
            UINT type = GetDriveTypeW(root);
            if (type == DRIVE_FIXED) {
                m_driveChoice->Append(wxString(root).Left(2));
            }
        }
    }
    if (m_driveChoice->GetCount() > 0) {
        m_driveChoice->SetSelection(0);
    }
    m_driveChoice->Bind(wxEVT_CHOICE, &DiskAnalyzerPanel::OnDriveChoice, this);
    topSizer->Add(m_driveChoice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

    m_scanButton = new wxButton(this, wxID_ANY, L"扫描", wxDefaultPosition, wxSize(100, 32));
    m_scanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    m_scanButton->SetBackgroundColour(wxColour(0x00, 0x78, 0xD4));
    m_scanButton->SetForegroundColour(*wxWHITE);
    m_scanButton->Bind(wxEVT_BUTTON, &DiskAnalyzerPanel::OnScanButton, this);
    topSizer->Add(m_scanButton, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

    topSizer->AddStretchSpacer();

    mainSizer->Add(topSizer, 0, wxEXPAND);
    mainSizer->AddSpacer(8);

    // 主内容区域: 矩形树图 + 文件类型筛选
    auto* contentSizer = new wxBoxSizer(wxHORIZONTAL);

    // 矩形树图
    m_treemapPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  wxFULL_REPAINT_ON_RESIZE);
    m_treemapPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_treemapPanel->SetBackgroundColour(wxColour(0xF5, 0xF5, 0xF5));
    m_treemapPanel->Bind(wxEVT_PAINT, &DiskAnalyzerPanel::OnTreemapPaint, this);
    m_treemapPanel->Bind(wxEVT_MOTION, &DiskAnalyzerPanel::OnTreemapMouseMotion, this);
    m_treemapPanel->Bind(wxEVT_LEFT_DOWN, &DiskAnalyzerPanel::OnTreemapLeftClick, this);
    m_treemapPanel->Bind(wxEVT_RIGHT_DOWN, &DiskAnalyzerPanel::OnTreemapRightClick, this);

    contentSizer->Add(m_treemapPanel, 1, wxEXPAND | wxLEFT, 20);

    // 文件类型筛选
    auto* filterSizer = new wxBoxSizer(wxVERTICAL);
    auto* filterLabel = new wxStaticText(this, wxID_ANY, L"文件类型筛选");
    filterLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                false, L"微软雅黑"));
    filterSizer->Add(filterLabel, 0, wxBOTTOM, 4);

    wxArrayString types;
    types.Add(L"视频");
    types.Add(L"音频");
    types.Add(L"图片");
    types.Add(L"文档");
    types.Add(L"压缩包");
    types.Add(L"应用程序");
    types.Add(L"系统文件");
    types.Add(L"其他");

    m_typeFilter = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, wxSize(120, 200),
                                      types);
    m_typeFilter->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    // 默认全部选中
    for (unsigned int i = 0; i < types.GetCount(); ++i) {
        m_typeFilter->Check(i, true);
    }
    m_typeFilter->Bind(wxEVT_CHECKLISTBOX, &DiskAnalyzerPanel::OnTypeFilter, this);
    filterSizer->Add(m_typeFilter, 1, wxEXPAND);

    contentSizer->Add(filterSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);

    mainSizer->Add(contentSizer, 1, wxEXPAND | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 状态栏
    m_statusLabel = new wxStaticText(this, wxID_ANY, L"选择驱动器并点击扫描开始分析");
    m_statusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                  false, L"微软雅黑"));
    m_statusLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    mainSizer->Add(m_statusLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    SetSizer(mainSizer);
}

void DiskAnalyzerPanel::SetDiskData(std::shared_ptr<IceClean::Models::DiskNode> root) {
    m_rootNode = root;
    BuildTreemap();
    m_treemapPanel->Refresh();
}

wxString DiskAnalyzerPanel::GetSelectedDrive() const {
    int sel = m_driveChoice->GetSelection();
    if (sel == wxNOT_FOUND) return L"C:";
    return m_driveChoice->GetStringSelection();
}

void DiskAnalyzerPanel::BuildTreemap() {
    m_treemapRects.clear();
    if (!m_rootNode || m_rootNode->children.empty()) return;

    const int w = m_treemapPanel->GetSize().GetWidth();
    const int h = m_treemapPanel->GetSize().GetHeight();
    if (w <= 0 || h <= 0) return;

    Squarify(m_rootNode, 0, 0, w, h);
}

void DiskAnalyzerPanel::Squarify(std::shared_ptr<IceClean::Models::DiskNode> node,
                                 int x, int y, int w, int h) {
    if (!node || node->children.empty()) return;

    // 按大小降序排列子节点
    auto children = node->children;
    std::sort(children.begin(), children.end(),
              [](const auto& a, const auto& b) { return a->GetTotalSize() > b->GetTotalSize(); });

    uint64_t totalSize = node->GetTotalSize();
    if (totalSize == 0) return;

    bool horizontal = w >= h;
    int currentX = x;
    int currentY = y;
    int remainingW = w;
    int remainingH = h;

    for (const auto& child : children) {
        double fraction = static_cast<double>(child->GetTotalSize()) / static_cast<double>(totalSize);
        int rectW, rectH;

        if (horizontal) {
            rectW = static_cast<int>(remainingW * fraction);
            rectH = remainingH;
        } else {
            rectW = remainingW;
            rectH = static_cast<int>(remainingH * fraction);
        }

        if (rectW > 0 && rectH > 0) {
            TreemapRect rect;
            rect.node = child;
            rect.x = currentX;
            rect.y = currentY;
            rect.width = rectW;
            rect.height = rectH;
            rect.color = GetNodeColor(child);
            m_treemapRects.push_back(rect);

            // 递归处理子节点
            if (!child->children.empty()) {
                int padding = 2;
                Squarify(child, currentX + padding, currentY + padding,
                         rectW - padding * 2, rectH - padding * 2);
            }

            if (horizontal) {
                currentX += rectW;
                remainingW -= rectW;
            } else {
                currentY += rectH;
                remainingH -= rectH;
            }
        }
    }
}

void DiskAnalyzerPanel::LayoutRow(std::vector<std::shared_ptr<IceClean::Models::DiskNode>>& row,
                                  int x, int y, int w, int h, bool horizontal) {
    // 简化实现 - 在Squarify中直接处理
}

wxColour DiskAnalyzerPanel::GetNodeColor(const std::shared_ptr<IceClean::Models::DiskNode>& node) const {
    if (!node) return wxColour(0xCC, 0xCC, 0xCC);

    // 根据扩展名分配颜色
    wxString name = node->name.Lower();
    if (name.EndsWith(L".mp4") || name.EndsWith(L".mkv") || name.EndsWith(L".avi") ||
        name.EndsWith(L".mov") || name.EndsWith(L".wmv")) {
        return wxColour(0x4C, 0xAF, 0x50); // 绿色 - 视频
    }
    if (name.EndsWith(L".mp3") || name.EndsWith(L".flac") || name.EndsWith(L".wav") ||
        name.EndsWith(L".aac")) {
        return wxColour(0xFF, 0x98, 0x00); // 橙色 - 音频
    }
    if (name.EndsWith(L".jpg") || name.EndsWith(L".png") || name.EndsWith(L".gif") ||
        name.EndsWith(L".bmp") || name.EndsWith(L".svg")) {
        return wxColour(0xE9, 0x1E, 0x63); // 粉色 - 图片
    }
    if (name.EndsWith(L".doc") || name.EndsWith(L".docx") || name.EndsWith(L".pdf") ||
        name.EndsWith(L".txt") || name.EndsWith(L".xlsx")) {
        return wxColour(0x21, 0x96, 0xF3); // 蓝色 - 文档
    }
    if (name.EndsWith(L".zip") || name.EndsWith(L".rar") || name.EndsWith(L".7z") ||
        name.EndsWith(L".tar") || name.EndsWith(L".gz")) {
        return wxColour(0x9C, 0x27, 0xB0); // 紫色 - 压缩包
    }
    if (name.EndsWith(L".exe") || name.EndsWith(L".msi") || name.EndsWith(L".dll")) {
        return wxColour(0xFF, 0x57, 0x22); // 红橙 - 应用程序
    }

    // 目录颜色 - 基于哈希
    if (node->isDirectory) {
        size_t hash = std::hash<std::wstring>{}(node->fullPath);
        int r = 80 + (hash % 80);
        int g = 130 + ((hash >> 8) % 80);
        int b = 180 + ((hash >> 16) % 60);
        return wxColour(r, g, b);
    }

    return wxColour(0x90, 0xA4, 0xAE); // 默认灰蓝色
}

void DiskAnalyzerPanel::OnTreemapPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(m_treemapPanel);
    dc.SetBackground(wxBrush(wxColour(0xF5, 0xF5, 0xF5)));
    dc.Clear();

    for (size_t i = 0; i < m_treemapRects.size(); ++i) {
        const auto& rect = m_treemapRects[i];

        // 绘制矩形
        wxColour color = rect.color;
        if (static_cast<int>(i) == m_hoveredRect) {
            color = color.ChangeLightness(120); // 悬停高亮
        }

        dc.SetBrush(wxBrush(color));
        dc.SetPen(wxPen(wxColour(0xF5, 0xF5, 0xF5), 1));
        dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);

        // 如果矩形足够大，显示名称
        if (rect.width > 40 && rect.height > 20 && rect.node) {
            dc.SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
            dc.SetTextForeground(*wxWHITE);
            dc.SetBackgroundMode(wxTRANSPARENT);

            wxString label = rect.node->name;
            int textW, textH;
            dc.GetTextExtent(label, &textW, &textH);
            if (textW < rect.width - 4 && textH < rect.height - 4) {
                dc.DrawText(label, rect.x + 2, rect.y + 2);
            }
        }
    }
}

void DiskAnalyzerPanel::OnTreemapMouseMotion(wxMouseEvent& event) {
    int mx = event.GetX();
    int my = event.GetY();
    int oldHovered = m_hoveredRect;

    m_hoveredRect = -1;
    for (int i = static_cast<int>(m_treemapRects.size()) - 1; i >= 0; --i) {
        const auto& rect = m_treemapRects[i];
        if (mx >= rect.x && mx < rect.x + rect.width &&
            my >= rect.y && my < rect.y + rect.height) {
            m_hoveredRect = i;
            break;
        }
    }

    if (m_hoveredRect != oldHovered) {
        m_treemapPanel->Refresh();
    }

    // 更新状态栏
    if (m_hoveredRect >= 0 && m_hoveredRect < static_cast<int>(m_treemapRects.size())) {
        const auto& rect = m_treemapRects[m_hoveredRect];
        if (rect.node) {
            m_statusLabel->SetLabel(wxString::Format(L"%s  |  %s  |  %s",
                rect.node->name,
                rect.node->fullPath,
                IceClean::Utils::FormatUtil::FormatFileSize(rect.node->GetTotalSize())));
        }
    }
}

void DiskAnalyzerPanel::OnTreemapLeftClick(wxMouseEvent& event) {
    // 左键点击 - 可用于钻入子目录
}

void DiskAnalyzerPanel::OnTreemapRightClick(wxMouseEvent& event) {
    int mx = event.GetX();
    int my = event.GetY();

    int clickedRect = -1;
    for (int i = static_cast<int>(m_treemapRects.size()) - 1; i >= 0; --i) {
        const auto& rect = m_treemapRects[i];
        if (mx >= rect.x && mx < rect.x + rect.width &&
            my >= rect.y && my < rect.y + rect.height) {
            clickedRect = i;
            break;
        }
    }

    if (clickedRect < 0) return;

    const auto& rect = m_treemapRects[clickedRect];
    if (!rect.node) return;

    wxMenu contextMenu;
    contextMenu.Append(wxID_DELETE, L"删除");
    contextMenu.Append(wxID_FORWARD, L"迁移");
    contextMenu.Append(wxID_OPEN, L"打开位置");

    // 简单的右键菜单处理
    contextMenu.Bind(wxEVT_MENU, [this, &rect](wxCommandEvent& evt) {
        if (evt.GetId() == wxID_OPEN && rect.node) {
            // 打开文件位置
            wxString path = rect.node->fullPath;
            if (path.IsEmpty()) return;
            // 使用ShellExecute打开
            ShellExecuteW(nullptr, L"explore", path.wc_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
    }, wxID_OPEN);

    m_treemapPanel->PopupMenu(&contextMenu, event.GetPosition());
}

void DiskAnalyzerPanel::OnScanButton(wxCommandEvent& event) {
    m_scanButton->Disable();
    m_scanButton->SetLabel(L"扫描中...");

    // 发送扫描事件
    wxThreadEvent scanEvt(wxEVT_SCAN_PROGRESS);
    scanEvt.SetInt(3); // 3 = 磁盘分析扫描
    wxPostEvent(GetParent(), scanEvt);
}

void DiskAnalyzerPanel::OnDriveChoice(wxCommandEvent& event) {
    // 驱动器切换时清空矩形树图
    m_treemapRects.clear();
    m_rootNode.reset();
    m_treemapPanel->Refresh();
    m_statusLabel->SetLabel(L"选择驱动器并点击扫描开始分析");
}

void DiskAnalyzerPanel::OnTypeFilter(wxCommandEvent& event) {
    // 文件类型筛选变更时重新绘制
    BuildTreemap();
    m_treemapPanel->Refresh();
}

} // namespace IceClean::Gui
