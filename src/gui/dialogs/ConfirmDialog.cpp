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
    , m_confirmText(confirmText)
    , m_level(level)
{
    CreateControls(confirmText, cancelText);
    CentreOnParent();

    // Dangerous 级别启用3秒倒计时
    if (m_level == DangerLevel::Dangerous) {
        m_countdown = 3;
        m_confirmButton->Enable(false);
        m_confirmButton->SetLabel(wxString::Format(L"%s (%d)", m_confirmText, m_countdown));

        m_countdownTimer = new wxTimer(this);
        Bind(wxEVT_TIMER, &ConfirmDialog::OnCountdown, this, m_countdownTimer->GetId());
        m_countdownTimer->Start(1000);
    }
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

    m_confirmButton = new wxButton(this, wxID_OK, confirmText);
    m_confirmButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
    m_confirmButton->SetBackgroundColour(GetAccentColor());
    m_confirmButton->SetForegroundColour(*wxWHITE);
    m_confirmButton->Bind(wxEVT_BUTTON, &ConfirmDialog::OnConfirm, this);
    buttonSizer->Add(m_confirmButton, 0, wxRIGHT, 20);

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
    if (m_countdownTimer) {
        m_countdownTimer->Stop();
        delete m_countdownTimer;
        m_countdownTimer = nullptr;
    }
    EndModal(wxID_OK);
}

void ConfirmDialog::OnCancel(wxCommandEvent& event) {
    if (m_countdownTimer) {
        m_countdownTimer->Stop();
        delete m_countdownTimer;
        m_countdownTimer = nullptr;
    }
    EndModal(wxID_CANCEL);
}

void ConfirmDialog::OnCountdown(wxTimerEvent& /*event*/) {
    m_countdown--;
    if (m_countdown <= 0) {
        // 倒计时结束，启用确认按钮
        m_countdownTimer->Stop();
        delete m_countdownTimer;
        m_countdownTimer = nullptr;
        m_confirmButton->Enable(true);
        m_confirmButton->SetLabel(m_confirmText);
    } else {
        m_confirmButton->SetLabel(wxString::Format(L"%s (%d)", m_confirmText, m_countdown));
    }
}

} // namespace IceClean::Gui
