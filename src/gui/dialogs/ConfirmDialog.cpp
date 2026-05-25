#include "ConfirmDialog.h"

namespace IceClean::Gui {

ConfirmDialog::ConfirmDialog(wxWindow* parent,
                             const wxString& title,
                             const wxString& description,
                             DangerLevel level,
                             const wxString& confirmText,
                             const wxString& cancelText)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(420, 220),
               wxDEFAULT_DIALOG_STYLE | wxCENTRE)
    , m_description(description)
    , m_level(level)
{
    CreateControls(confirmText, cancelText);
    CentreOnParent();
}

void ConfirmDialog::CreateControls(const wxString& confirmText, const wxString& cancelText) {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(20);

    // 图标 + 描述区域
    auto* contentSizer = new wxBoxSizer(wxHORIZONTAL);
    contentSizer->AddSpacer(20);

    // 警告图标 (使用文字符号代替)
    auto* iconText = new wxStaticText(this, wxID_ANY, L"⚠");
    iconText->SetFont(wxFont(28, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    iconText->SetForegroundColour(GetAccentColor());
    contentSizer->Add(iconText, 0, wxALIGN_TOP);
    contentSizer->AddSpacer(16);

    // 描述文字
    auto* descText = new wxStaticText(this, wxID_ANY, m_description);
    descText->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
    descText->Wrap(320);
    contentSizer->Add(descText, 1, wxEXPAND);

    mainSizer->Add(contentSizer, 1, wxEXPAND);
    mainSizer->AddSpacer(20);

    // 按钮区域
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();

    auto* cancelButton = new wxButton(this, wxID_CANCEL, cancelText);
    cancelButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    cancelButton->Bind(wxEVT_BUTTON, &ConfirmDialog::OnCancel, this);
    buttonSizer->Add(cancelButton, 0, wxRIGHT, 8);

    auto* confirmButton = new wxButton(this, wxID_OK, confirmText);
    confirmButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                  false, L"微软雅黑"));
    confirmButton->SetBackgroundColour(GetAccentColor());
    confirmButton->SetForegroundColour(*wxWHITE);
    confirmButton->Bind(wxEVT_BUTTON, &ConfirmDialog::OnConfirm, this);
    buttonSizer->Add(confirmButton, 0, wxRIGHT, 20);

    mainSizer->Add(buttonSizer, 0, wxEXPAND);
    mainSizer->AddSpacer(16);

    SetSizer(mainSizer);
}

wxColour ConfirmDialog::GetAccentColor() const {
    switch (m_level) {
        case DangerLevel::Caution:   return wxColour(0xFF, 0x8C, 0x00); // 橙色
        case DangerLevel::Dangerous: return wxColour(0xE8, 0x11, 0x23); // 红色
        default: return *wxBLACK;
    }
}

void ConfirmDialog::OnConfirm(wxCommandEvent& event) {
    EndModal(wxID_OK);
}

void ConfirmDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}

} // namespace IceClean::Gui
