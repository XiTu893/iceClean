#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/choice.h>
#include <wx/checklst.h>
#include "models/DebloatItem.h"
#include "core/optimizer/WindowsDebloater.h"

namespace IceClean::Gui {

class WindowsDebloaterPanel : public wxPanel {
public:
    WindowsDebloaterPanel(wxWindow* parent);

    void RefreshItems();

private:
    void OnPresetSelected(wxCommandEvent& event);
    void OnSelectAll(wxCommandEvent& event);
    void OnDeselectAll(wxCommandEvent& event);
    void OnSelectRecommended(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);

    void PopulateList();
    void UpdateApplyButtonState();

    Core::Optimizer::WindowsDebloater m_debloater;
    std::vector<Models::DebloatItem> m_items;

    wxChoice* m_presetChoice = nullptr;
    wxCheckListBox* m_itemList = nullptr;
    wxButton* m_applyButton = nullptr;
    wxStaticText* m_infoLabel = nullptr;
};

} // namespace IceClean::Gui
