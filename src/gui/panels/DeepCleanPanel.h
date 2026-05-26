#pragma once
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <vector>
#include "core/cleaner/RegistryCleaner.h"

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

    // 注册表清理标签页
    wxButton* m_registryScanButton = nullptr;
    wxButton* m_registryCleanButton = nullptr;
    wxCheckBox* m_registrySelectAllCheck = nullptr;
    wxListCtrl* m_registryList = nullptr;
    wxStaticText* m_registryStatusLabel = nullptr;
    std::vector<IceClean::Core::Cleaner::RegistryInvalidItem> m_registryItems;

    void CreateControls();
    void CreateSystemCleanTab(wxWindow* parent);
    void CreateRegistryCleanTab(wxWindow* parent);
    void CreatePrivacyCleanTab(wxWindow* parent);

    void OnCleanButton(wxCommandEvent& event);
    void OnRegistryScan(wxCommandEvent& event);
    void OnRegistryClean(wxCommandEvent& event);
    void OnRegistrySelectAll(wxCommandEvent& event);

    wxString GetTypeString(IceClean::Core::Cleaner::RegistryInvalidItem::Type type) const;

    wxDECLARE_EVENT_TABLE();
};

} // namespace IceClean::Gui
