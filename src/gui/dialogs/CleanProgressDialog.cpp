#include "CleanProgressDialog.h"
#include "gui/controls/ThemeManager.h"
#include <algorithm>

namespace IceClean::Gui {

CleanProgressDialog::CleanProgressDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, L"正在清理...", wxDefaultPosition, wxSize(420, 200),
               wxDEFAULT_DIALOG_STYLE & ~wxCLOSE_BOX & ~wxMINIMIZE_BOX & ~wxMAXIMIZE_BOX | wxCENTRE)
{
    // 关闭时自动销毁
    Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event) {
        Destroy();
    });
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(16);

    // 分类标签
    m_categoryLabel = new wxStaticText(this, wxID_ANY, L"正在清理: --");
    m_categoryLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                    false, L"微软雅黑"));
    mainSizer->Add(m_categoryLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(10);

    // 进度条
    m_progressGauge = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(380, 22));
    m_progressGauge->SetValue(0);
    mainSizer->Add(m_progressGauge, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(10);

    // 当前文件
    m_fileLabel = new wxStaticText(this, wxID_ANY, L"准备中...");
    m_fileLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    m_fileLabel->SetForegroundColour(ThemeManager::Instance().GetColors().textSecondary);
    mainSizer->Add(m_fileLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 已释放大小
    m_sizeLabel = new wxStaticText(this, wxID_ANY, L"已释放: 0 MB");
    m_sizeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    mainSizer->Add(m_sizeLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(16);

    // 取消按钮
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    m_cancelButton = new wxButton(this, wxID_ANY, L"取消");
    m_cancelButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_cancelButton->Bind(wxEVT_BUTTON, &CleanProgressDialog::OnCancel, this);
    buttonSizer->Add(m_cancelButton, 0, wxRIGHT, 20);
    mainSizer->Add(buttonSizer, 0, wxEXPAND);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);
}

void CleanProgressDialog::SetProgress(int percent) {
    m_progressGauge->SetValue(std::clamp(percent, 0, 100));
}

void CleanProgressDialog::SetCurrentFile(const wxString& fileName) {
    wxString display = fileName;
    if (display.length() > 55) {
        display = display.Left(26) + L"..." + display.Right(26);
    }
    m_fileLabel->SetLabel(display);
}

void CleanProgressDialog::SetCleanedSize(uint64_t bytes) {
    double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    m_sizeLabel->SetLabel(wxString::Format(L"已释放: %.2f MB", mb));
}

void CleanProgressDialog::SetCategory(const wxString& category) {
    m_categoryLabel->SetLabel(L"正在清理: " + category);
}

void CleanProgressDialog::SetFinished(uint64_t totalCleaned) {
    SetProgress(100);
    SetTitle(L"清理完成");

    double mb = static_cast<double>(totalCleaned) / (1024.0 * 1024.0);
    m_sizeLabel->SetLabel(wxString::Format(L"已释放: %.2f MB", mb));

    m_fileLabel->SetLabel(L"清理完成！");
    m_fileLabel->SetForegroundColour(ThemeManager::Instance().GetColors().success);

    m_cancelButton->SetLabel(L"关闭");
    m_cancelButton->Unbind(wxEVT_BUTTON, &CleanProgressDialog::OnCancel, this);
    m_cancelButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Close(); });
}

void CleanProgressDialog::OnCancel(wxCommandEvent& event) {
    m_cancelled = true;
    m_cancelButton->Disable();
    m_fileLabel->SetLabel(L"正在取消...");
}

} // namespace IceClean::Gui
