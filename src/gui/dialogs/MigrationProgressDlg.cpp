#include "MigrationProgressDlg.h"
#include "utils/FormatUtil.h"

namespace IceClean::Gui {

MigrationProgressDlg::MigrationProgressDlg(wxWindow* parent, const wxString& title,
                                           const wxPoint& pos, const wxSize& size)
    : wxDialog(parent, wxID_ANY, title, pos, size,
               wxDEFAULT_DIALOG_STYLE & ~wxCLOSE_BOX | wxCENTRE)
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(16);

    // 进度条
    m_progressBar = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(440, 24));
    m_progressBar->SetValue(0);
    mainSizer->Add(m_progressBar, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // 当前文件
    m_fileLabel = new wxStaticText(this, wxID_ANY, L"准备迁移...");
    m_fileLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    m_fileLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    mainSizer->Add(m_fileLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 统计信息
    m_statsLabel = new wxStaticText(this, wxID_ANY, L"0/0 项  |  0 B / 0 B");
    m_statsLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    mainSizer->Add(m_statsLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 速度和剩余时间
    auto* infoSizer = new wxBoxSizer(wxHORIZONTAL);
    m_speedLabel = new wxStaticText(this, wxID_ANY, L"速度: --");
    m_speedLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    m_speedLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    infoSizer->Add(m_speedLabel, 1);

    m_timeLabel = new wxStaticText(this, wxID_ANY, L"剩余时间: --");
    m_timeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    m_timeLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    infoSizer->Add(m_timeLabel, 0);
    mainSizer->Add(infoSizer, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
    mainSizer->AddSpacer(16);

    // 取消按钮
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    m_cancelButton = new wxButton(this, wxID_ANY, L"取消");
    m_cancelButton->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_cancelButton->Bind(wxEVT_BUTTON, &MigrationProgressDlg::OnCancel, this);
    buttonSizer->Add(m_cancelButton, 0, wxRIGHT, 20);
    mainSizer->Add(buttonSizer, 0, wxEXPAND);
    mainSizer->AddSpacer(12);

    SetSizer(mainSizer);

    Bind(wxEVT_CLOSE_WINDOW, &MigrationProgressDlg::OnClose, this);
}

void MigrationProgressDlg::SetCurrentFile(const wxString& file) {
    wxString display = file;
    if (display.length() > 60) {
        display = display.Left(28) + L"..." + display.Right(28);
    }
    m_fileLabel->SetLabel(display);
}

void MigrationProgressDlg::SetProgress(int percent) {
    m_progressBar->SetValue(wxClamp(percent, 0, 100));
}

void MigrationProgressDlg::SetSpeed(const wxString& speed) {
    m_speedLabel->SetLabel(L"速度: " + speed);
}

void MigrationProgressDlg::SetRemainingTime(const wxString& time) {
    m_timeLabel->SetLabel(L"剩余时间: " + time);
}

void MigrationProgressDlg::SetStats(int currentItem, int totalItems,
                                    uint64_t migratedSize, uint64_t totalSize) {
    using namespace IceClean::Utils;
    wxString stats = wxString::Format(L"%d/%d 项  |  %s / %s",
        currentItem, totalItems,
        FormatUtil::FormatFileSize(migratedSize),
        FormatUtil::FormatFileSize(totalSize));
    m_statsLabel->SetLabel(stats);
}

void MigrationProgressDlg::SetCompleted(bool success, const wxString& message) {
    m_progressBar->SetValue(100);
    m_cancelButton->SetLabel(L"关闭");
    m_cancelButton->Unbind(wxEVT_BUTTON, &MigrationProgressDlg::OnCancel, this);
    m_cancelButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_OK); });

    if (success) {
        m_fileLabel->SetLabel(message.IsEmpty() ? L"迁移完成！" : message);
        m_fileLabel->SetForegroundColour(wxColour(0x10, 0x7C, 0x10));
    } else {
        m_fileLabel->SetLabel(message.IsEmpty() ? L"迁移失败" : message);
        m_fileLabel->SetForegroundColour(wxColour(0xE8, 0x11, 0x23));
    }
    m_speedLabel->SetLabel(L"速度: --");
    m_timeLabel->SetLabel(L"剩余时间: --");
}

void MigrationProgressDlg::OnCancel(wxCommandEvent& event) {
    m_cancelled = true;
    m_cancelButton->Disable();
    m_fileLabel->SetLabel(L"正在取消...");
}

void MigrationProgressDlg::OnClose(wxCloseEvent& event) {
    m_cancelled = true;
    event.Skip();
}

} // namespace IceClean::Gui
