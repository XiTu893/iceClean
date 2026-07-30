#include "PrivacyOptimizerPanel.h"
#include "gui/controls/ThemeManager.h"
#include "core/utils/ProgressReporter.h"
#include "gui/Events.h"
#include "gui/dialogs/UnifiedProgressDialog.h"
#include <algorithm>

namespace IceClean::Gui {

PrivacyOptimizerPanel::PrivacyOptimizerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.background);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"隐私策略设置");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* descLabel = new wxStaticText(this, wxID_ANY, L"管理 Windows 隐私设置，控制数据收集与权限。更严格的设置提供更强隐私保护。");
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
    auto presets = m_optimizer.GetPresets();
    for (const auto& p : presets) {
        m_presetChoice->Append(p.name);
    }
    if (m_presetChoice->GetCount() > 0) {
        m_presetChoice->SetSelection(0);
    }
    m_presetChoice->Bind(wxEVT_CHOICE, &PrivacyOptimizerPanel::OnPresetSelected, this);
    presetRow->Add(m_presetChoice, 0, wxEXPAND);
    presetRow->AddStretchSpacer();

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
    selectAllBtn->Bind(wxEVT_BUTTON, &PrivacyOptimizerPanel::OnSelectAll, this);
    buttonRow->Add(selectAllBtn, 0, wxRIGHT, 8);

    buttonRow->AddStretchSpacer();

    m_applyButton = new wxButton(this, wxID_ANY, L"应用所选 (0)");
    m_applyButton->SetBackgroundColour(colors.accent);
    m_applyButton->SetForegroundColour(*wxWHITE);
    m_applyButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
    m_applyButton->Disable();
    m_applyButton->Bind(wxEVT_BUTTON, &PrivacyOptimizerPanel::OnApply, this);
    buttonRow->Add(m_applyButton, 0, wxLEFT, 0);

    mainSizer->Add(buttonRow, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);

    RefreshItems();
}

void PrivacyOptimizerPanel::RefreshItems() {
    m_optimizer.RefreshCurrentValues();
    m_items = m_optimizer.GetItems();
    PopulateList();
}

void PrivacyOptimizerPanel::OnPresetSelected(wxCommandEvent& /*event*/) {
    int sel = m_presetChoice->GetSelection();
    if (sel < 0) return;
    auto presets = m_optimizer.GetPresets();
    if (sel >= static_cast<int>(presets.size())) return;

    const auto& preset = presets[sel];
    for (size_t i = 0; i < m_items.size(); ++i) {
        bool inPreset = std::find(preset.itemIds.begin(), preset.itemIds.end(), m_items[i].id)
                        != preset.itemIds.end();
        m_items[i].isApplied = false; // reset applied state for UI display
        m_itemList->Check(i, inPreset);
    }
    UpdateApplyButtonState();
}

void PrivacyOptimizerPanel::OnSelectAll(wxCommandEvent& /*event*/) {
    for (size_t i = 0; i < m_items.size(); ++i) {
        m_itemList->Check(i, true);
    }
    UpdateApplyButtonState();
}

void PrivacyOptimizerPanel::OnApply(wxCommandEvent& /*event*/) {
    std::vector<Models::PrivacyItem> selected;
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_itemList->IsChecked(i)) {
            selected.push_back(m_items[i]);
        }
    }
    if (selected.empty()) return;

    wxString msg = wxString::Format(L"确定要应用 %zu 项隐私策略设置吗？\n部分设置需要管理员权限。",
                                     selected.size());
    wxMessageDialog confirm(this, msg, L"确认设置", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    if (confirm.ShowModal() != wxID_YES) return;

    auto* progressDlg = new UnifiedProgressDialog(this, L"正在应用隐私策略");
    progressDlg->Show();

    std::thread([this, selected, progressDlg]() {
        auto reporter = Core::Utils::ProgressReporter(
            [progressDlg](const Core::Utils::ProgressSnapshot& snap) {
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
            reporter.SetStage(L"正在设置: " + selected[i].name);
            reporter.Advance();

            if (m_optimizer.ApplyItem(selected[i])) {
                successCount++;
            }
        }

        bool allSuccess = (successCount == total);
        wxString summary = wxString::Format(L"设置完成: %d / %d 项成功", successCount, total);

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_OPERATION_COMPLETE);
        completeEvt->SetInt(allSuccess ? 1 : 0);
        completeEvt->SetPayload(summary);
        wxQueueEvent(progressDlg->GetParent(), completeEvt);
    }).detach();
}

void PrivacyOptimizerPanel::PopulateList() {
    m_itemList->Clear();

    // 按类别分组
    auto addGroup = [&](const std::wstring& catName, Models::PrivacyCategory cat) {
        bool headerAdded = false;
        for (size_t i = 0; i < m_items.size(); ++i) {
            if (m_items[i].category != cat) continue;

            if (!headerAdded) {
                m_itemList->Append(L"── " + catName + L" ──");
                m_itemList->Check(m_itemList->GetCount() - 1, false);
                m_itemList->Enable(m_itemList->GetCount() - 1, false);
                headerAdded = true;
            }

            wxString display;
            switch (m_items[i].safetyLevel) {
                case Models::SafetyLevel::Safe:     display = L"🟢 "; break;
                case Models::SafetyLevel::Caution:  display = L"🟡 "; break;
                case Models::SafetyLevel::Warning:  display = L"🔴 "; break;
            }
            display += m_items[i].name;
            if (!m_items[i].description.empty()) {
                display += L" - " + m_items[i].description;
            }

            m_itemList->Append(display);
            m_itemList->Check(m_itemList->GetCount() - 1, false);
        }
    };

    addGroup(L"遥测与诊断", Models::PrivacyCategory::Telemetry);
    addGroup(L"Cortana 与搜索", Models::PrivacyCategory::Cortana);
    addGroup(L"广告与个性化", Models::PrivacyCategory::Advertising);
    addGroup(L"位置信息", Models::PrivacyCategory::Location);
    addGroup(L"摄像头", Models::PrivacyCategory::Camera);
    addGroup(L"麦克风", Models::PrivacyCategory::Microphone);
    addGroup(L"通知", Models::PrivacyCategory::Notifications);
    addGroup(L"语音识别", Models::PrivacyCategory::Speech);
    addGroup(L"搜索权限", Models::PrivacyCategory::Search);
    addGroup(L"Windows 更新", Models::PrivacyCategory::Updates);
    addGroup(L"OneDrive", Models::PrivacyCategory::OneDrive);
    addGroup(L"活动历史记录", Models::PrivacyCategory::ActivityHistory);
    addGroup(L"传递优化", Models::PrivacyCategory::DeliveryOptimization);
    addGroup(L"游戏 DVR", Models::PrivacyCategory::GameDVR);
    addGroup(L"应用权限", Models::PrivacyCategory::AppPermissions);

    m_itemList->Bind(wxEVT_CHECKLISTBOX, [this](wxCommandEvent&) {
        UpdateApplyButtonState();
    });

    UpdateApplyButtonState();
}

void PrivacyOptimizerPanel::UpdateApplyButtonState() {
    int selected = 0;
    for (size_t i = 0; i < m_itemList->GetCount(); ++i) {
        if (m_itemList->IsChecked(i)) selected++;
    }
    m_infoLabel->SetLabel(wxString::Format(L"共 %zu 项，已选 %d 项", m_items.size(), selected));
    m_applyButton->SetLabel(wxString::Format(L"应用所选 (%d)", selected));
    m_applyButton->Enable(selected > 0);
}

} // namespace IceClean::Gui
