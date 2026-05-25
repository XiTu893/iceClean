#include "ScanResultPanel.h"
#include "gui/controls/SafetyBadge.h"
#include "gui/Events.h"
#include "utils/FormatUtil.h"

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(ScanResultPanel, wxPanel)
wxEND_EVENT_TABLE()

ScanResultPanel::ScanResultPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(*wxWHITE);
    CreateControls();
}

void ScanResultPanel::CreateControls() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // ── 顶部信息栏 ──
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"扫描结果");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    topSizer->Add(titleLabel, 0, wxALIGN_CENTER_VERTICAL);

    topSizer->AddStretchSpacer();

    m_durationLabel = new wxStaticText(this, wxID_ANY, L"扫描耗时: --");
    m_durationLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
    m_durationLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    topSizer->Add(m_durationLabel, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(topSizer, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // ── 分类列表 (可滚动区域) ──
    m_categoryScroller = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                               wxVSCROLL | wxBORDER_NONE);
    m_categoryScroller->SetBackgroundColour(*wxWHITE);
    m_categoryScroller->SetScrollRate(0, 10);

    m_categorySizer = new wxBoxSizer(wxVERTICAL);
    m_categoryScroller->SetSizer(m_categorySizer);

    mainSizer->Add(m_categoryScroller, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // ── 底部操作栏 ──
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    m_totalSizeLabel = new wxStaticText(this, wxID_ANY, L"可释放: 0 B");
    m_totalSizeLabel->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                     false, L"微软雅黑"));
    m_totalSizeLabel->SetForegroundColour(wxColour(0x00, 0x78, 0xD4));
    bottomSizer->Add(m_totalSizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 20);

    bottomSizer->AddStretchSpacer();

    m_cleanButton = new wxButton(this, wxID_ANY, L"一键清理", wxDefaultPosition, wxSize(140, 40));
    m_cleanButton->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
    m_cleanButton->SetBackgroundColour(wxColour(0x00, 0x78, 0xD4));
    m_cleanButton->SetForegroundColour(*wxWHITE);
    m_cleanButton->Bind(wxEVT_BUTTON, &ScanResultPanel::OnCleanButton, this);
    bottomSizer->Add(m_cleanButton, 0, wxRIGHT, 20);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 8);

    SetSizer(mainSizer);
}

void ScanResultPanel::SetScanResult(const IceClean::Models::ScanResult& result) {
    m_result = result;

    // 更新扫描耗时
    using namespace IceClean::Utils;
    m_durationLabel->SetLabel(L"扫描耗时: " + FormatUtil::FormatDuration(result.scanDurationMs));

    BuildCategoryList();
    UpdateTotalSize();
}

void ScanResultPanel::BuildCategoryList() {
    // 清空现有控件
    m_categoryUIs.clear();
    m_categorySizer->Clear(true);

    for (size_t i = 0; i < m_result.categories.size(); ++i) {
        auto& cat = m_result.categories[i];
        CategoryUI ui;

        // ── 分类头部 (可点击展开/折叠) ──
        ui.headerPanel = new wxPanel(m_categoryScroller, wxID_ANY);
        ui.headerPanel->SetBackgroundColour(*wxWHITE);
        ui.headerPanel->SetCursor(wxCURSOR_HAND);

        auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);
        headerSizer->AddSpacer(8);

        // 展开/折叠箭头
        auto* arrowLabel = new wxStaticText(ui.headerPanel, wxID_ANY, L"▶");
        arrowLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        arrowLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
        headerSizer->Add(arrowLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        // 分类名称
        auto* nameLabel = new wxStaticText(ui.headerPanel, wxID_ANY, cat.name);
        nameLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
        nameLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
        headerSizer->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        // 安全等级标识
        auto* badge = new SafetyBadge(ui.headerPanel, wxID_ANY);
        badge->SetSafetyRating(cat.safety);
        headerSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        headerSizer->AddStretchSpacer();

        // 大小标签
        ui.sizeLabel = new wxStaticText(ui.headerPanel, wxID_ANY,
            IceClean::Utils::FormatUtil::FormatFileSize(cat.totalSize));
        ui.sizeLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
        ui.sizeLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
        headerSizer->Add(ui.sizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        // 分类复选框
        ui.categoryCheck = new wxCheckBox(ui.headerPanel, wxID_ANY, wxEmptyString);
        // 安全项默认选中，谨慎项默认不选
        ui.categoryCheck->SetValue(cat.safety == IceClean::Models::SafetyRating::Safe);
        ui.categoryCheck->Bind(wxEVT_CHECKBOX, &ScanResultPanel::OnCategoryCheck, this);
        headerSizer->Add(ui.categoryCheck, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        ui.headerPanel->SetSizer(headerSizer);
        ui.headerPanel->Bind(wxEVT_LEFT_DOWN, &ScanResultPanel::OnCategoryHeaderClick, this);

        m_categorySizer->Add(ui.headerPanel, 0, wxEXPAND | wxBOTTOM, 1);

        // ── 分类详情 (默认隐藏) ──
        ui.detailPanel = new wxPanel(m_categoryScroller, wxID_ANY);
        ui.detailPanel->SetBackgroundColour(wxColour(0xFA, 0xFA, 0xFA));
        ui.detailPanel->Hide();

        auto* detailSizer = new wxBoxSizer(wxVERTICAL);
        detailSizer->AddSpacer(4);

        // 分类描述
        if (!cat.description.IsEmpty()) {
            auto* descLabel = new wxStaticText(ui.detailPanel, wxID_ANY, cat.description);
            descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
            descLabel->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
            descLabel->Wrap(600);
            detailSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 36);
            detailSizer->AddSpacer(4);
        }

        // 文件项列表
        for (size_t j = 0; j < cat.items.size(); ++j) {
            const auto& item = cat.items[j];
            auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
            itemSizer->AddSpacer(36);

            auto* itemCheck = new wxCheckBox(ui.detailPanel, wxID_ANY, wxEmptyString);
            itemCheck->SetValue(cat.safety == IceClean::Models::SafetyRating::Safe);
            itemCheck->Bind(wxEVT_CHECKBOX, &ScanResultPanel::OnItemCheck, this);
            ui.itemChecks.push_back(itemCheck);
            itemSizer->Add(itemCheck, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

            auto* pathLabel = new wxStaticText(ui.detailPanel, wxID_ANY, item.path);
            pathLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
            pathLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
            itemSizer->Add(pathLabel, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

            auto* sizeLabel = new wxStaticText(ui.detailPanel, wxID_ANY,
                IceClean::Utils::FormatUtil::FormatFileSize(item.size));
            sizeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
            sizeLabel->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
            itemSizer->Add(sizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 16);

            detailSizer->Add(itemSizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 2);
        }

        detailSizer->AddSpacer(4);
        ui.detailPanel->SetSizer(detailSizer);

        m_categorySizer->Add(ui.detailPanel, 0, wxEXPAND | wxBOTTOM, 4);

        // 同步模型中的展开状态
        ui.expanded = cat.expanded;
        if (ui.expanded) {
            ui.detailPanel->Show();
        }

        m_categoryUIs.push_back(std::move(ui));
    }

    m_categoryScroller->FitInside();
}

void ScanResultPanel::UpdateTotalSize() {
    uint64_t selectedSize = GetSelectedSize();
    using namespace IceClean::Utils;
    m_totalSizeLabel->SetLabel(L"可释放: " + FormatUtil::FormatFileSize(selectedSize));
}

std::vector<std::wstring> ScanResultPanel::GetSelectedPaths() const {
    std::vector<std::wstring> paths;
    for (size_t i = 0; i < m_result.categories.size() && i < m_categoryUIs.size(); ++i) {
        const auto& cat = m_result.categories[i];
        const auto& ui = m_categoryUIs[i];

        if (!ui.categoryCheck->GetValue()) continue;

        for (size_t j = 0; j < cat.items.size() && j < ui.itemChecks.size(); ++j) {
            if (ui.itemChecks[j]->GetValue()) {
                paths.push_back(cat.items[j].path);
            }
        }
    }
    return paths;
}

uint64_t ScanResultPanel::GetSelectedSize() const {
    uint64_t total = 0;
    for (size_t i = 0; i < m_result.categories.size() && i < m_categoryUIs.size(); ++i) {
        const auto& cat = m_result.categories[i];
        const auto& ui = m_categoryUIs[i];

        if (!ui.categoryCheck->GetValue()) continue;

        for (size_t j = 0; j < cat.items.size() && j < ui.itemChecks.size(); ++j) {
            if (ui.itemChecks[j]->GetValue()) {
                total += cat.items[j].size;
            }
        }
    }
    return total;
}

void ScanResultPanel::OnCategoryHeaderClick(wxMouseEvent& event) {
    // 找到点击的分类
    wxWindow* clickedPanel = static_cast<wxWindow*>(event.GetEventObject());
    // 可能点击的是子控件，向上查找headerPanel
    while (clickedPanel && clickedPanel != m_categoryScroller) {
        for (size_t i = 0; i < m_categoryUIs.size(); ++i) {
            if (m_categoryUIs[i].headerPanel == clickedPanel) {
                auto& ui = m_categoryUIs[i];
                ui.expanded = !ui.expanded;
                if (ui.expanded) {
                    ui.detailPanel->Show();
                } else {
                    ui.detailPanel->Hide();
                }
                m_categoryScroller->FitInside();
                return;
            }
        }
        clickedPanel = clickedPanel->GetParent();
    }
    event.Skip();
}

void ScanResultPanel::OnCategoryCheck(wxCommandEvent& event) {
    // 找到对应的分类，同步所有子项的选中状态
    wxCheckBox* check = static_cast<wxCheckBox*>(event.GetEventObject());
    bool checked = check->GetValue();

    for (auto& ui : m_categoryUIs) {
        if (ui.categoryCheck == check) {
            for (auto* itemCheck : ui.itemChecks) {
                itemCheck->SetValue(checked);
            }
            break;
        }
    }
    UpdateTotalSize();
}

void ScanResultPanel::OnItemCheck(wxCommandEvent& event) {
    UpdateTotalSize();
}

void ScanResultPanel::OnCleanButton(wxCommandEvent& event) {
    // 发送清理开始事件
    wxThreadEvent cleanEvt(wxEVT_CLEAN_PROGRESS);
    cleanEvt.SetInt(0);
    wxPostEvent(GetParent(), cleanEvt);
}

} // namespace IceClean::Gui
