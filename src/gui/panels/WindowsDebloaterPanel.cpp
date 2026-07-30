#include "WindowsDebloaterPanel.h"
#include "gui/controls/ThemeManager.h"
#include "core/utils/ProgressReporter.h"
#include "gui/Events.h"
#include "gui/dialogs/UnifiedProgressDialog.h"
#include <algorithm>

namespace IceClean::Gui {

WindowsDebloaterPanel::WindowsDebloaterPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.background);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // ── 标题区域 ──
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"Windows 组件精简");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* descLabel = new wxStaticText(this, wxID_ANY, L"禁用不必要的 Windows 组件和功能，提升系统性能。已选项将以管理员权限执行。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // ── 预设选择 ──
    auto* presetRow = new wxBoxSizer(wxHORIZONTAL);
    auto* presetLabel = new wxStaticText(this, wxID_ANY, L"预设方案:");
    presetLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    presetRow->Add(presetLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    m_presetChoice = new wxChoice(this, wxID_ANY);
    m_presetChoice->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    auto presets = m_debloater.GetPresets();
    for (const auto& p : presets) {
        m_presetChoice->Append(p.name);
    }
    if (m_presetChoice->GetCount() > 0) {
        m_presetChoice->SetSelection(0);
    }
    m_presetChoice->Bind(wxEVT_CHOICE, &WindowsDebloaterPanel::OnPresetSelected, this);
    presetRow->Add(m_presetChoice, 0, wxEXPAND);

    presetRow->AddStretchSpacer();

    // 信息标签
    m_infoLabel = new wxStaticText(this, wxID_ANY, L"共 0 项，已选 0 项");
    m_infoLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    m_infoLabel->SetForegroundColour(colors.textSecondary);
    presetRow->Add(m_infoLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    mainSizer->Add(presetRow, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // ── 项目列表 ──
    m_itemList = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 300));
    m_itemList->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    mainSizer->Add(m_itemList, 1, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(8);

    // ── 操作按钮行 ──
    auto* buttonRow = new wxBoxSizer(wxHORIZONTAL);

    auto* selectAllBtn = new wxButton(this, wxID_ANY, L"全选");
    selectAllBtn->Bind(wxEVT_BUTTON, &WindowsDebloaterPanel::OnSelectAll, this);
    buttonRow->Add(selectAllBtn, 0, wxRIGHT, 8);

    auto* deselectAllBtn = new wxButton(this, wxID_ANY, L"反选");
    deselectAllBtn->Bind(wxEVT_BUTTON, &WindowsDebloaterPanel::OnDeselectAll, this);
    buttonRow->Add(deselectAllBtn, 0, wxRIGHT, 8);

    auto* recommendedBtn = new wxButton(this, wxID_ANY, L"仅推荐");
    recommendedBtn->Bind(wxEVT_BUTTON, &WindowsDebloaterPanel::OnSelectRecommended, this);
    buttonRow->Add(recommendedBtn, 0, wxRIGHT, 8);

    buttonRow->AddStretchSpacer();

    m_applyButton = new wxButton(this, wxID_ANY, L"应用所选 (0)");
    m_applyButton->SetBackgroundColour(colors.accent);
    m_applyButton->SetForegroundColour(*wxWHITE);
    m_applyButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
    m_applyButton->Disable();
    m_applyButton->Bind(wxEVT_BUTTON, &WindowsDebloaterPanel::OnApply, this);
    buttonRow->Add(m_applyButton, 0, wxLEFT, 0);

    mainSizer->Add(buttonRow, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);

    // 填充数据
    RefreshItems();
}

void WindowsDebloaterPanel::RefreshItems() {
    m_items = m_debloater.GetDebloatItems();
    PopulateList();
}

void WindowsDebloaterPanel::OnPresetSelected(wxCommandEvent& /*event*/) {
    int sel = m_presetChoice->GetSelection();
    if (sel < 0) return;
    auto presets = m_debloater.GetPresets();
    if (sel >= static_cast<int>(presets.size())) return;

    const auto& preset = presets[sel];
    for (size_t i = 0; i < m_items.size(); ++i) {
        bool inPreset = std::find(preset.itemIds.begin(), preset.itemIds.end(), m_items[i].id)
                        != preset.itemIds.end();
        m_items[i].isSelected = inPreset;
        m_itemList->Check(i, inPreset);
    }

    UpdateApplyButtonState();
}

void WindowsDebloaterPanel::OnSelectAll(wxCommandEvent& /*event*/) {
    for (size_t i = 0; i < m_items.size(); ++i) {
        m_items[i].isSelected = true;
        m_itemList->Check(i, true);
    }
    UpdateApplyButtonState();
}

void WindowsDebloaterPanel::OnDeselectAll(wxCommandEvent& /*event*/) {
    for (size_t i = 0; i < m_items.size(); ++i) {
        m_items[i].isSelected = false;
        m_itemList->Check(i, false);
    }
    UpdateApplyButtonState();
}

void WindowsDebloaterPanel::OnSelectRecommended(wxCommandEvent& /*event*/) {
    for (size_t i = 0; i < m_items.size(); ++i) {
        m_items[i].isSelected = m_items[i].isRecommended;
        m_itemList->Check(i, m_items[i].isRecommended);
    }
    UpdateApplyButtonState();
}

void WindowsDebloaterPanel::OnApply(wxCommandEvent& /*event*/) {
    // 收集选中的项
    std::vector<Models::DebloatItem> selected;
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].isSelected) {
            selected.push_back(m_items[i]);
        }
    }
    if (selected.empty()) return;

    // 显示确认对话框
    wxString msg = wxString::Format(L"确定要应用 %zu 项系统精简操作吗？\n部分操作需要管理员权限。\n建议在操作前创建还原点。",
                                     selected.size());
    wxMessageDialog confirm(this, msg, L"确认精简", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    if (confirm.ShowModal() != wxID_YES) return;

    // 显示统一进度对话框
    auto* progressDlg = new UnifiedProgressDialog(this, L"正在精简 Windows 组件");
    progressDlg->Show();

    // 后台线程执行
    std::thread([this, selected, progressDlg]() {
        auto reporter = Core::Utils::ProgressReporter(
            [progressDlg](const Core::Utils::ProgressSnapshot& snap) {
                UnifiedProgressData data;
                data.percent = snap.percent;
                data.stage = snap.stage;
                data.detail = snap.detail;
                data.processedItems = snap.processedItems;
                data.totalItems = snap.totalItems;
                data.processedBytes = snap.processedBytes;
                data.speedBytesPerSec = snap.speedBytesPerSec;

                wxThreadEvent* event = new wxThreadEvent(wxEVT_OPERATION_PROGRESS_UPDATE);
                event->SetPayload(snap);
                wxQueueEvent(progressDlg->GetParent(), event);
            }
        );

        int total = static_cast<int>(selected.size());
        reporter.SetTotalSteps(total);

        int successCount = 0;
        for (int i = 0; i < total; ++i) {
            if (reporter.IsCancelled()) break;

            reporter.SetStage(L"正在精简: " + selected[i].name);
            reporter.Advance();

            if (m_debloater.ApplyItem(selected[i])) {
                successCount++;
            }
        }

        bool allSuccess = (successCount == total);
        wxString summary = wxString::Format(L"精简完成: %d / %d 项成功", successCount, total);

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_OPERATION_COMPLETE);
        completeEvt->SetInt(allSuccess ? 1 : 0);
        completeEvt->SetPayload(summary);
        wxQueueEvent(progressDlg->GetParent(), completeEvt);
    }).detach();
}

void WindowsDebloaterPanel::PopulateList() {
    m_itemList->Clear();

    // 按类别分组
    std::vector<std::pair<std::wstring, std::vector<size_t>>> groups;
    auto addToGroup = [&](const std::wstring& catName, const std::wstring& targetCat) {
        std::vector<size_t> indices;
        for (size_t i = 0; i < m_items.size(); ++i) {
            bool matches = false;
            switch (m_items[i].category) {
                case Models::DebloatCategory::AppxPackage: matches = (targetCat == L"Appx"); break;
                case Models::DebloatCategory::SystemComponent: matches = (targetCat == L"Component"); break;
                case Models::DebloatCategory::Telemetry: matches = (targetCat == L"Telemetry"); break;
                case Models::DebloatCategory::Service: matches = (targetCat == L"Service"); break;
                case Models::DebloatCategory::ScheduledTask: matches = (targetCat == L"Task"); break;
                case Models::DebloatCategory::ContextMenu: matches = (targetCat == L"Menu"); break;
                case Models::DebloatCategory::RegistryTweak: matches = (targetCat == L"Registry"); break;
            }
            if (matches) indices.push_back(i);
        }
        if (!indices.empty()) {
            groups.emplace_back(catName, indices);
        }
    };

    addToGroup(L"── Appx 应用包 ──", L"Appx");
    addToGroup(L"── 系统组件 ──", L"Component");
    addToGroup(L"── 遥测与数据收集 ──", L"Telemetry");
    addToGroup(L"── 服务 ──", L"Service");
    addToGroup(L"── 计划任务 ──", L"Task");
    addToGroup(L"── 上下文菜单 ──", L"Menu");
    addToGroup(L"── 注册表优化 ──", L"Registry");

    for (const auto& [groupName, indices] : groups) {
        m_itemList->Append(groupName);
        m_itemList->Check(m_itemList->GetCount() - 1, false);
        m_itemList->Enable(m_itemList->GetCount() - 1, false);

        for (size_t idx : indices) {
            std::wstring display = m_items[idx].name;
            if (!m_items[idx].description.empty()) {
                display += L" - " + m_items[idx].description;
            }
            m_itemList->Append(display);
            m_itemList->Check(m_itemList->GetCount() - 1, m_items[idx].isSelected);
        }
    }

    // 绑定 Check 事件
    m_itemList->Bind(wxEVT_CHECKLISTBOX, [this](wxCommandEvent& event) {
        int sel = event.GetInt();
        // 跳过分组标题行
        int itemIdx = 0;
        for (size_t i = 0; i < m_items.size(); ++i) {
            if (itemIdx == sel) {
                m_items[i].isSelected = m_itemList->IsChecked(sel);
                break;
            }
            itemIdx++;
            // 跳过分组标题行
            for (const auto& [_, indices] : {
                 std::pair<std::wstring, std::vector<size_t>>{L"", {}},
                 std::pair<std::wstring, std::vector<size_t>>{L"", {}},
                 std::pair<std::wstring, std::vector<size_t>>{L"", {}},
                 std::pair<std::wstring, std::vector<size_t>>{L"", {}},
                 std::pair<std::wstring, std::vector<size_t>>{L"", {}},
                 std::pair<std::wstring, std::vector<size_t>>{L"", {}},
                 std::pair<std::wstring, std::vector<size_t>>{L"", {}}
            }) {
                (void)_;
                itemIdx++;
            }
        }
        UpdateApplyButtonState();
    });

    UpdateApplyButtonState();
}

void WindowsDebloaterPanel::UpdateApplyButtonState() {
    int selected = 0;
    for (const auto& item : m_items) {
        if (item.isSelected) selected++;
    }
    m_infoLabel->SetLabel(wxString::Format(L"共 %zu 项，已选 %d 项", m_items.size(), selected));
    m_applyButton->SetLabel(wxString::Format(L"应用所选 (%d)", selected));

    if (selected > 0) {
        m_applyButton->Enable();
    } else {
        m_applyButton->Disable();
    }
}

} // namespace IceClean::Gui
