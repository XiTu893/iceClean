#pragma once
#include <wx/wx.h>

namespace IceClean::Gui {

// 确认对话框 - 用于危险操作的二次确认
class ConfirmDialog : public wxDialog {
public:
    enum class DangerLevel {
        Caution,   // 橙色 - 谨慎操作
        Dangerous  // 红色 - 危险操作
    };

    ConfirmDialog(wxWindow* parent,
                  const wxString& title,
                  const wxString& description,
                  DangerLevel level = DangerLevel::Caution,
                  const wxString& confirmText = L"确认",
                  const wxString& cancelText = L"取消");

    wxString GetDescription() const { return m_description; }

private:
    wxString m_description;
    DangerLevel m_level;

    void CreateControls(const wxString& confirmText, const wxString& cancelText);
    wxColour GetAccentColor() const;

    void OnConfirm(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
};

} // namespace IceClean::Gui
