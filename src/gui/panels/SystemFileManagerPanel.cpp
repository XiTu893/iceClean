#include "SystemFileManagerPanel.h"
#include "gui/controls/ThemeManager.h"
#include "gui/dialogs/UnifiedProgressDialog.h"
#include "gui/Events.h"
#include "core/utils/ProgressReporter.h"
#include <wx/gbsizer.h>
#include <thread>

namespace IceClean::Gui {

SystemFileManagerPanel::SystemFileManagerPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.background);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"系统文件管理");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* descLabel = new wxStaticText(this, wxID_ANY, L"管理占用 C 盘空间的关键系统文件，包含休眠文件、虚拟内存、Windows.old、回收站等。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(16);

    // ── 使用可滚动区域 ──
    auto* scrollWin = new wxScrolledWindow(this, wxID_ANY);
    scrollWin->SetBackgroundColour(colors.background);
    scrollWin->SetScrollRate(0, 10);
    auto* cardSizer = new wxBoxSizer(wxVERTICAL);
    cardSizer->AddSpacer(4);

    // ── 系统文件卡片：hiberfil.sys ──
    {
        auto* card = new wxPanel(scrollWin, wxID_ANY);
        card->SetBackgroundColour(colors.surface);
        auto* cardSizerInner = new wxBoxSizer(wxVERTICAL);
        cardSizerInner->AddSpacer(12);

        auto* headerRow = new wxBoxSizer(wxHORIZONTAL);
        auto* icon = new wxStaticText(card, wxID_ANY, L"💤");
        icon->SetFont(wxFont(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        headerRow->Add(icon, 0, wxLEFT, 16);
        headerRow->AddSpacer(12);

        auto* titleSizer = new wxBoxSizer(wxVERTICAL);
        auto* name = new wxStaticText(card, wxID_ANY, L"休眠文件 (hiberfil.sys)");
        name->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                             false, L"微软雅黑"));
        titleSizer->Add(name);
        titleSizer->AddSpacer(2);

        m_hibernationStatus = new wxStaticText(card, wxID_ANY, L"正在检测...");
        m_hibernationStatus->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                            false, L"微软雅黑"));
        m_hibernationStatus->SetForegroundColour(colors.textSecondary);
        titleSizer->Add(m_hibernationStatus);
        headerRow->Add(titleSizer, 1, wxEXPAND);
        cardSizerInner->Add(headerRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(8);

        auto* infoRow = new wxBoxSizer(wxHORIZONTAL);
        infoRow->AddSpacer(48);
        auto* infoText = new wxStaticText(card, wxID_ANY, L"当前大小: ");
        infoText->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
        infoRow->Add(infoText, 0, wxALIGN_CENTER_VERTICAL);
        m_hibernationSize = new wxStaticText(card, wxID_ANY, L"--");
        m_hibernationSize->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                          false, L"微软雅黑"));
        m_hibernationSize->SetForegroundColour(colors.accent);
        infoRow->Add(m_hibernationSize, 0, wxALIGN_CENTER_VERTICAL);
        infoRow->AddStretchSpacer();
        cardSizerInner->Add(infoRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(12);

        auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
        btnRow->AddSpacer(48);

        m_hibernationBtn = new wxButton(card, wxID_ANY, L"禁用休眠 (删除文件)");
        m_hibernationBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                         false, L"微软雅黑"));
        m_hibernationBtn->Bind(wxEVT_BUTTON, &SystemFileManagerPanel::OnDisableHibernation, this);
        btnRow->Add(m_hibernationBtn, 0, wxRIGHT, 8);

        m_hibernationSmallBtn = new wxButton(card, wxID_ANY, L"缩小 (仅保留快速启动)");
        m_hibernationSmallBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                              false, L"微软雅黑"));
        m_hibernationSmallBtn->Bind(wxEVT_BUTTON, &SystemFileManagerPanel::OnSetHibernationSmall, this);
        btnRow->Add(m_hibernationSmallBtn, 0);
        btnRow->AddStretchSpacer();
        cardSizerInner->Add(btnRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(12);
        card->SetSizer(cardSizerInner);
        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    // ── 系统文件卡片：pagefile.sys ──
    {
        auto* card = new wxPanel(scrollWin, wxID_ANY);
        card->SetBackgroundColour(colors.surface);
        auto* cardSizerInner = new wxBoxSizer(wxVERTICAL);
        cardSizerInner->AddSpacer(12);

        auto* headerRow = new wxBoxSizer(wxHORIZONTAL);
        auto* icon = new wxStaticText(card, wxID_ANY, L"💾");
        icon->SetFont(wxFont(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        headerRow->Add(icon, 0, wxLEFT, 16);
        headerRow->AddSpacer(12);

        auto* titleSizer = new wxBoxSizer(wxVERTICAL);
        auto* name = new wxStaticText(card, wxID_ANY, L"虚拟内存 (pagefile.sys)");
        name->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                             false, L"微软雅黑"));
        titleSizer->Add(name);
        titleSizer->AddSpacer(2);
        auto* desc = new wxStaticText(card, wxID_ANY, L"系统虚拟内存文件，建议由系统自动管理");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textSecondary);
        titleSizer->Add(desc);
        headerRow->Add(titleSizer, 1, wxEXPAND);
        cardSizerInner->Add(headerRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(8);

        auto* infoRow = new wxBoxSizer(wxHORIZONTAL);
        infoRow->AddSpacer(48);
        auto* infoText = new wxStaticText(card, wxID_ANY, L"当前大小: ");
        infoText->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
        infoRow->Add(infoText, 0, wxALIGN_CENTER_VERTICAL);
        m_pagefileSize = new wxStaticText(card, wxID_ANY, L"--");
        m_pagefileSize->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                       false, L"微软雅黑"));
        m_pagefileSize->SetForegroundColour(colors.accent);
        infoRow->Add(m_pagefileSize, 0, wxALIGN_CENTER_VERTICAL);
        infoRow->AddStretchSpacer();
        cardSizerInner->Add(infoRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(12);

        auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
        btnRow->AddSpacer(48);
        m_pagefileBtn = new wxButton(card, wxID_ANY, L"优化为推荐大小");
        m_pagefileBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"微软雅黑"));
        m_pagefileBtn->Bind(wxEVT_BUTTON, &SystemFileManagerPanel::OnOptimizePageFile, this);
        btnRow->Add(m_pagefileBtn, 0);
        btnRow->AddStretchSpacer();
        cardSizerInner->Add(btnRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(12);
        card->SetSizer(cardSizerInner);
        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    // ── 系统文件卡片：Windows.old ──
    {
        auto* card = new wxPanel(scrollWin, wxID_ANY);
        card->SetBackgroundColour(colors.surface);
        auto* cardSizerInner = new wxBoxSizer(wxVERTICAL);
        cardSizerInner->AddSpacer(12);

        auto* headerRow = new wxBoxSizer(wxHORIZONTAL);
        auto* icon = new wxStaticText(card, wxID_ANY, L"📁");
        icon->SetFont(wxFont(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        headerRow->Add(icon, 0, wxLEFT, 16);
        headerRow->AddSpacer(12);

        auto* titleSizer = new wxBoxSizer(wxVERTICAL);
        auto* name = new wxStaticText(card, wxID_ANY, L"旧 Windows 安装 (Windows.old)");
        name->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                             false, L"微软雅黑"));
        titleSizer->Add(name);
        titleSizer->AddSpacer(2);
        auto* desc = new wxStaticText(card, wxID_ANY, L"系统升级后保留的旧系统文件，安全删除后可释放大量空间");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textSecondary);
        titleSizer->Add(desc);
        headerRow->Add(titleSizer, 1, wxEXPAND);
        cardSizerInner->Add(headerRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(8);

        auto* infoRow = new wxBoxSizer(wxHORIZONTAL);
        infoRow->AddSpacer(48);
        auto* infoText = new wxStaticText(card, wxID_ANY, L"占用空间: ");
        infoText->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
        infoRow->Add(infoText, 0, wxALIGN_CENTER_VERTICAL);
        m_winOldSize = new wxStaticText(card, wxID_ANY, L"--");
        m_winOldSize->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                     false, L"微软雅黑"));
        m_winOldSize->SetForegroundColour(colors.accent);
        infoRow->Add(m_winOldSize, 0, wxALIGN_CENTER_VERTICAL);
        infoRow->AddStretchSpacer();
        cardSizerInner->Add(infoRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(12);

        auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
        btnRow->AddSpacer(48);
        m_winOldBtn = new wxButton(card, wxID_ANY, L"删除 Windows.old");
        m_winOldBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
        m_winOldBtn->Bind(wxEVT_BUTTON, &SystemFileManagerPanel::OnCleanWindowsOld, this);
        btnRow->Add(m_winOldBtn, 0);
        btnRow->AddStretchSpacer();
        cardSizerInner->Add(btnRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(12);
        card->SetSizer(cardSizerInner);
        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    // ── 系统文件卡片：回收站 ──
    {
        auto* card = new wxPanel(scrollWin, wxID_ANY);
        card->SetBackgroundColour(colors.surface);
        auto* cardSizerInner = new wxBoxSizer(wxVERTICAL);
        cardSizerInner->AddSpacer(12);

        auto* headerRow = new wxBoxSizer(wxHORIZONTAL);
        auto* icon = new wxStaticText(card, wxID_ANY, L"🗑️");
        icon->SetFont(wxFont(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        headerRow->Add(icon, 0, wxLEFT, 16);
        headerRow->AddSpacer(12);

        auto* titleSizer = new wxBoxSizer(wxVERTICAL);
        auto* name = new wxStaticText(card, wxID_ANY, L"回收站");
        name->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                             false, L"微软雅黑"));
        titleSizer->Add(name);
        titleSizer->AddSpacer(2);
        auto* desc = new wxStaticText(card, wxID_ANY, L"已删除文件的临时存储，清空后可释放磁盘空间");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textSecondary);
        titleSizer->Add(desc);
        headerRow->Add(titleSizer, 1, wxEXPAND);
        cardSizerInner->Add(headerRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(8);

        auto* infoRow = new wxBoxSizer(wxHORIZONTAL);
        infoRow->AddSpacer(48);
        auto* infoText = new wxStaticText(card, wxID_ANY, L"占用空间: ");
        infoText->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
        infoRow->Add(infoText, 0, wxALIGN_CENTER_VERTICAL);
        m_recycleBinSize = new wxStaticText(card, wxID_ANY, L"--");
        m_recycleBinSize->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                         false, L"微软雅黑"));
        m_recycleBinSize->SetForegroundColour(colors.accent);
        infoRow->Add(m_recycleBinSize, 0, wxALIGN_CENTER_VERTICAL);
        infoRow->AddStretchSpacer();
        cardSizerInner->Add(infoRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(12);

        auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
        btnRow->AddSpacer(48);
        m_recycleBinBtn = new wxButton(card, wxID_ANY, L"清空回收站");
        m_recycleBinBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                        false, L"微软雅黑"));
        m_recycleBinBtn->Bind(wxEVT_BUTTON, &SystemFileManagerPanel::OnEmptyRecycleBin, this);
        btnRow->Add(m_recycleBinBtn, 0);
        btnRow->AddStretchSpacer();
        cardSizerInner->Add(btnRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(12);
        card->SetSizer(cardSizerInner);
        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    // ── 系统文件卡片：WinSxS ──
    {
        auto* card = new wxPanel(scrollWin, wxID_ANY);
        card->SetBackgroundColour(colors.surface);
        auto* cardSizerInner = new wxBoxSizer(wxVERTICAL);
        cardSizerInner->AddSpacer(12);

        auto* headerRow = new wxBoxSizer(wxHORIZONTAL);
        auto* icon = new wxStaticText(card, wxID_ANY, L"🔄");
        icon->SetFont(wxFont(20, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        headerRow->Add(icon, 0, wxLEFT, 16);
        headerRow->AddSpacer(12);

        auto* titleSizer = new wxBoxSizer(wxVERTICAL);
        auto* name = new wxStaticText(card, wxID_ANY, L"Windows 更新缓存 (WinSxS)");
        name->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                             false, L"微软雅黑"));
        titleSizer->Add(name);
        titleSizer->AddSpacer(2);
        auto* desc = new wxStaticText(card, wxID_ANY, L"通过 DISM 工具清理 WinSxS 组件存储中的过期备份");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textSecondary);
        titleSizer->Add(desc);
        headerRow->Add(titleSizer, 1, wxEXPAND);
        cardSizerInner->Add(headerRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(8);

        auto* infoRow = new wxBoxSizer(wxHORIZONTAL);
        infoRow->AddSpacer(48);
        m_winSxSInfo = new wxStaticText(card, wxID_ANY, L"需管理员权限运行 DISM 清理");
        m_winSxSInfo->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
        m_winSxSInfo->SetForegroundColour(colors.textSecondary);
        infoRow->Add(m_winSxSInfo, 0, wxALIGN_CENTER_VERTICAL);
        infoRow->AddStretchSpacer();
        cardSizerInner->Add(infoRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(12);

        auto* btnRow = new wxBoxSizer(wxHORIZONTAL);
        btnRow->AddSpacer(48);
        m_winSxsBtn = new wxButton(card, wxID_ANY, L"清理 WinSxS (DISM)");
        m_winSxsBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
        m_winSxsBtn->Bind(wxEVT_BUTTON, &SystemFileManagerPanel::OnCleanWinSxS, this);
        btnRow->Add(m_winSxsBtn, 0);
        btnRow->AddStretchSpacer();
        cardSizerInner->Add(btnRow, 0, wxEXPAND);

        cardSizerInner->AddSpacer(12);
        card->SetSizer(cardSizerInner);
        cardSizer->Add(card, 0, wxLEFT | wxRIGHT | wxEXPAND, 20);
        cardSizer->AddSpacer(8);
    }

    cardSizer->AddSpacer(12);
    scrollWin->SetSizer(cardSizer);
    mainSizer->Add(scrollWin, 1, wxEXPAND);

    SetSizer(mainSizer);

    // 异步加载数据
    RefreshItems();
}

void SystemFileManagerPanel::RefreshItems() {
    std::thread([this]() {
        auto hibInfo = m_manager.GetHibernationFileInfo();
        auto pageInfo = m_manager.GetPageFileInfo();
        auto winOldInfo = m_manager.GetWindowsOldInfo();
        auto recycleInfo = m_manager.GetRecycleBinInfo();

        CallAfter([this, hibInfo, pageInfo, winOldInfo, recycleInfo]() {
            // 更新休眠信息
            m_hibernationSize->SetLabel(hibInfo.sizeDisplay);
            m_hibernationStatus->SetLabel(hibInfo.isActive ? L"状态: 已启用" : L"状态: 已禁用");

            // 更新虚拟内存信息
            m_pagefileSize->SetLabel(pageInfo.sizeDisplay);

            // 更新 Windows.old
            m_winOldSize->SetLabel(winOldInfo.sizeDisplay);
            m_winOldBtn->Enable(winOldInfo.sizeBytes > 0);

            // 更新回收站
            m_recycleBinSize->SetLabel(recycleInfo.sizeDisplay);
            m_recycleBinBtn->Enable(recycleInfo.sizeBytes > 0);
        });
    }).detach();
}

// ── 操作事件 ──

void SystemFileManagerPanel::OnDisableHibernation(wxCommandEvent& /*event*/) {
    wxMessageDialog confirm(this, L"确定要禁用休眠并删除 hiberfil.sys 吗？\n此操作将释放磁盘空间，但将无法使用休眠功能。",
                            L"确认禁用休眠", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
    if (confirm.ShowModal() != wxID_YES) return;

    auto* progressDlg = new UnifiedProgressDialog(this, L"正在禁用休眠");
    progressDlg->Show();

    std::thread([this, progressDlg]() {
        auto reporter = Core::Utils::ProgressReporter(
            [progressDlg](const Core::Utils::ProgressSnapshot& snap) {
                wxThreadEvent* event = new wxThreadEvent(wxEVT_OPERATION_PROGRESS_UPDATE);
                event->SetPayload(snap);
                wxQueueEvent(progressDlg->GetParent(), event);
            }
        );

        reporter.ReportProgress(10, L"正在执行命令...");

        auto result = m_manager.DisableHibernation();

        wxString summary = result.success
            ? wxString::Format(L"休眠已禁用，释放 %s", m_manager.FormatSize(result.freedBytes))
            : L"禁用休眠失败: " + result.message;

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_OPERATION_COMPLETE);
        completeEvt->SetInt(result.success ? 1 : 0);
        completeEvt->SetPayload(summary);
        wxQueueEvent(progressDlg->GetParent(), completeEvt);

        CallAfter([this]() { RefreshItems(); });
    }).detach();
}

void SystemFileManagerPanel::OnSetHibernationSmall(wxCommandEvent& /*event*/) {
    auto* progressDlg = new UnifiedProgressDialog(this, L"正在缩小休眠文件");
    progressDlg->Show();

    std::thread([this, progressDlg]() {
        auto result = m_manager.SetHibernationSize(40);

        wxString summary = result.success
            ? wxString::Format(L"休眠文件已缩小，释放 %s", m_manager.FormatSize(result.freedBytes))
            : L"缩小失败: " + result.message;

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_OPERATION_COMPLETE);
        completeEvt->SetInt(result.success ? 1 : 0);
        completeEvt->SetPayload(summary);
        wxQueueEvent(progressDlg->GetParent(), completeEvt);

        CallAfter([this]() { RefreshItems(); });
    }).detach();
}

void SystemFileManagerPanel::OnOptimizePageFile(wxCommandEvent& /*event*/) {
    wxMessageDialog confirm(this, L"将虚拟内存调整为系统推荐大小。\n此操作需要重启电脑后生效。", L"确认优化",
                            wxYES_NO | wxNO_DEFAULT | wxICON_INFORMATION);
    if (confirm.ShowModal() != wxID_YES) return;

    auto result = m_manager.OptimizePageFile();
    if (result.success) {
        wxMessageBox(L"虚拟内存已优化为推荐大小，重启后生效。", L"操作完成", wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(L"优化失败: " + result.message, L"错误", wxOK | wxICON_ERROR, this);
    }
}

void SystemFileManagerPanel::OnCleanWindowsOld(wxCommandEvent& /*event*/) {
    wxMessageDialog confirm(this, L"确定要删除 Windows.old 文件夹吗？\n此操作不可撤销！删除后将无法降级到旧版 Windows。",
                            L"确认删除", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
    if (confirm.ShowModal() != wxID_YES) return;

    auto* progressDlg = new UnifiedProgressDialog(this, L"正在删除 Windows.old");
    progressDlg->Show();

    std::thread([this, progressDlg]() {
        auto result = m_manager.CleanWindowsOld();

        wxString summary = result.success
            ? wxString::Format(L"Windows.old 已删除，释放 %s", m_manager.FormatSize(result.freedBytes))
            : L"删除失败: " + result.message;

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_OPERATION_COMPLETE);
        completeEvt->SetInt(result.success ? 1 : 0);
        completeEvt->SetPayload(summary);
        wxQueueEvent(progressDlg->GetParent(), completeEvt);

        CallAfter([this]() { RefreshItems(); });
    }).detach();
}

void SystemFileManagerPanel::OnEmptyRecycleBin(wxCommandEvent& /*event*/) {
    wxMessageDialog confirm(this, L"确定要清空回收站吗？\n清空后将无法恢复已删除的文件。",
                            L"确认清空", wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
    if (confirm.ShowModal() != wxID_YES) return;

    auto result = m_manager.EmptyRecycleBin();
    if (result.success) {
        wxString msg = wxString::Format(L"回收站已清空，释放 %s", m_manager.FormatSize(result.freedBytes));
        wxMessageBox(msg, L"操作完成", wxOK | wxICON_INFORMATION, this);
    } else {
        wxMessageBox(L"清空回收站失败: " + result.message, L"错误", wxOK | wxICON_ERROR, this);
    }
    RefreshItems();
}

void SystemFileManagerPanel::OnCleanWinSxS(wxCommandEvent& /*event*/) {
    wxMessageDialog confirm(this, L"将通过 DISM 工具清理 WinSxS 组件存储。\n此操作需要管理员权限，可能耗时数分钟。",
                            L"确认清理", wxYES_NO | wxNO_DEFAULT | wxICON_INFORMATION);
    if (confirm.ShowModal() != wxID_YES) return;

    auto* progressDlg = new UnifiedProgressDialog(this, L"正在清理 WinSxS");
    progressDlg->Show();

    std::thread([this, progressDlg]() {
        auto result = m_manager.CleanWinSxS();

        wxString summary = result.success
            ? wxString::Format(L"WinSxS 清理完成，释放 %s", m_manager.FormatSize(result.freedBytes))
            : L"清理失败: " + result.message;

        wxThreadEvent* completeEvt = new wxThreadEvent(wxEVT_OPERATION_COMPLETE);
        completeEvt->SetInt(result.success ? 1 : 0);
        completeEvt->SetPayload(summary);
        wxQueueEvent(progressDlg->GetParent(), completeEvt);

        CallAfter([this]() { RefreshItems(); });
    }).detach();
}

} // namespace IceClean::Gui
