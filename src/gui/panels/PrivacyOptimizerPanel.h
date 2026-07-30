#pragma once
#include <wx/wx.h>
#include <wx/choice.h>
#include <wx/checklst.h>
#include "models/PrivacyItem.h"
#include "core/optimizer/PrivacyOptimizer.h"

namespace IceClean::Gui {

class PrivacyOptimizerPanel : public wxPanel {
public:
    PrivacyOptimizerPanel(wxWindow* parent);

    void RefreshItems();

private:
    void OnPresetSelected(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);

    void PopulateList();
    void UpdateApplyButtonState();

    Core::Optimizer::PrivacyOptimizer m_optimizer;
    std::vector<Models::PrivacyItem> m_items;

    wxChoice* m_presetChoice = nullptr;
    wxCheckListBox* m_itemList = nullptr;
    wxButton* m_applyButton = nullptr;
    wxStaticText* m_infoLabel = nullptr;
};

} // namespace IceClean::Gui
