#pragma once
#include <wx/wx.h>
#include <vector>

namespace IceClean::Gui {

// 深度清理面板
class DeepCleanPanel : public wxPanel {
public:
    DeepCleanPanel(wxWindow* parent, wxWindowID id = wxID_ANY);

    // 获取选中的深度清理项ID列表
    std::vector<wxString> GetSelectedIds() const;

private:
    wxNotebook* m_notebook = nullptr;
    wxButton* m_cleanButton = nullptr;

    // 系统清理标签页
    struct SystemCleanItem {
        wxCheckBox* checkbox = nullptr;
        wxString id;          // 项标识
        wxString description; // 描述
        bool isDangerous = false;
    };
    std::vector<SystemCleanItem> m_systemItems;

    // 隐私清理标签页
    struct PrivacyCleanItem {
        wxCheckBox* checkbox = nullptr;
        wxString id;
        wxString description;
    };
    std::vector<PrivacyCleanItem> m_privacyItems;

    void CreateControls();
    void CreateSystemCleanTab(wxWindow* parent);
    void CreateRegistryCleanTab(wxWindow* parent);
    void CreatePrivacyCleanTab(wxWindow* parent);

    void OnCleanButton(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
