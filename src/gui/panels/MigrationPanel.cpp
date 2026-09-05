#include "MigrationPanel.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/dialogs/MigrationProgressDlg.h"
#include "gui/Events.h"
#include "gui/controls/ThemeManager.h"
#include "App.h"
#include "utils/FormatUtil.h"
#include "utils/JunctionPoint.h"
#include "utils/FileUtil.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(MigrationPanel, wxPanel)
EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, MigrationPanel::OnNotebookPageChanged)
wxEND_EVENT_TABLE()

namespace {
constexpr int ID_TAB_PROGRAM = 1001;
constexpr int ID_TAB_FOLDER = 1002;
}

MigrationPanel::MigrationPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    Bind(wxEVT_MIGRATION_SCAN_PROGRESS, &MigrationPanel::OnMigrationScanProgress, this);
    CreateControls();
}

void MigrationPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"智能迁移");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    // 描述
    auto* descLabel = new wxStaticText(this, wxID_ANY,
        L"将C盘大文件迁移到其他分区，通过Junction链接让程序无感知运行。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // ═══ 扫描控制栏 ═══
    auto* scanSizer = new wxBoxSizer(wxHORIZONTAL);
    scanSizer->AddSpacer(20);

    auto* thresholdLabel = new wxStaticText(this, wxID_ANY, L"阈值:");
    thresholdLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
    scanSizer->Add(thresholdLabel, 0, wxALIGN_CENTER_VERTICAL);

    m_thresholdChoice = new wxChoice(this, wxID_ANY);
    PopulateThresholdList();
    m_thresholdChoice->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                       false, L"微软雅黑"));
    m_thresholdChoice->Bind(wxEVT_CHOICE, &MigrationPanel::OnThresholdChanged, this);
    scanSizer->Add(m_thresholdChoice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);

    scanSizer->AddSpacer(8);

    m_scanButton = new wxButton(this, wxID_ANY, L"开始扫描", wxDefaultPosition, wxSize(140, 36));
    m_scanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    m_scanButton->SetBackgroundColour(colors.accent);
    m_scanButton->SetForegroundColour(*wxWHITE);
    m_scanButton->Bind(wxEVT_BUTTON, &MigrationPanel::OnScanButton, this);
    scanSizer->Add(m_scanButton, 0);

    m_stopButton = new wxButton(this, wxID_ANY, L"停止", wxDefaultPosition, wxSize(80, 36));
    m_stopButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                  false, L"微软雅黑"));
    m_stopButton->SetBackgroundColour(colors.danger);
    m_stopButton->SetForegroundColour(*wxWHITE);
    m_stopButton->Bind(wxEVT_BUTTON, &MigrationPanel::OnStopButton, this);
    m_stopButton->Hide();
    scanSizer->Add(m_stopButton, 0, wxLEFT, 8);

    m_statusLabel = new wxStaticText(this, wxID_ANY, L"");
    m_statusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    m_statusLabel->SetForegroundColour(colors.textSecondary);
    scanSizer->Add(m_statusLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

    mainSizer->Add(scanSizer, 0);
    mainSizer->AddSpacer(8);

    // 扫描状态面板
    m_scanInfoPanel = new IceClean::Gui::ScanInfoPanel(this, wxID_ANY);
    m_scanInfoPanel->SetState(ScanInfoPanelState::Normal);
    m_scanInfoPanel->SetMinSize(wxSize(300, 48));
    mainSizer->Add(m_scanInfoPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    m_currentPathLabel = new wxStaticText(this, wxID_ANY, L"");
    m_currentPathLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"Consolas"));
    m_currentPathLabel->SetForegroundColour(colors.textSecondary);
    m_currentPathLabel->SetLabelText(L"");
    mainSizer->Add(m_currentPathLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // ═══ wxNotebook 标签页（与启动优化一致）═══
    m_notebook = new wxNotebook(this, wxID_ANY);
    m_notebook->SetBackgroundColour(colors.surface);

    CreateProgramTab();
    CreateFolderTab();

    m_notebook->SetSelection(0);  // 默认显示应用迁移 Tab
    m_currentScanType = 1;

    mainSizer->Add(m_notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 展开面板
    m_expandPanel = new wxPanel(this, wxID_ANY);
    m_expandPanel->Hide();
    auto expandSizer = new wxBoxSizer(wxVERTICAL);
    m_expandTitle = new wxStaticText(m_expandPanel, wxID_ANY, L"可迁移子目录：");
    m_expandTitle->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                   false, L"微软雅黑"));
    m_expandContent = new wxStaticText(m_expandPanel, wxID_ANY, L"");
    m_expandContent->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                      false, L"Consolas"));
    m_expandContent->SetForegroundColour(colors.textSecondary);
    expandSizer->Add(m_expandTitle, 0, wxLEFT, 8);
    expandSizer->Add(m_expandContent, 0, wxLEFT, 8);
    m_expandPanel->SetSizer(expandSizer);
    mainSizer->Add(m_expandPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // 底部操作栏
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* driveLabel = new wxStaticText(this, wxID_ANY, L"目标驱动器:");
    driveLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    bottomSizer->Add(driveLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 20);

    m_targetDriveChoice = new wxChoice(this, wxID_ANY);
    m_targetDriveChoice->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                        false, L"微软雅黑"));
    PopulateDriveList();
    bottomSizer->Add(m_targetDriveChoice, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);

    bottomSizer->AddStretchSpacer();

    m_deleteButton = new wxButton(this, wxID_ANY, L"删除", wxDefaultPosition, wxSize(100, 40));
    m_deleteButton->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                    false, L"微软雅黑"));
    m_deleteButton->SetBackgroundColour(colors.danger);
    m_deleteButton->SetForegroundColour(*wxWHITE);
    m_deleteButton->Bind(wxEVT_BUTTON, &MigrationPanel::OnDeleteButton, this);
    bottomSizer->Add(m_deleteButton, 0, wxRIGHT, 8);

    m_migrateButton = new wxButton(this, wxID_ANY, L"开始迁移", wxDefaultPosition, wxSize(140, 40));
    m_migrateButton->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                    false, L"微软雅黑"));
    m_migrateButton->SetBackgroundColour(colors.accent);
    m_migrateButton->SetForegroundColour(*wxWHITE);
    m_migrateButton->Bind(wxEVT_BUTTON, &MigrationPanel::OnMigrateButton, this);
    bottomSizer->Add(m_migrateButton, 0, wxRIGHT, 20);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxBOTTOM, 12);
    SetSizer(mainSizer);

    // 应用迁移 Tab 不需要阈值下拉框
    m_thresholdChoice->Enable(false);
}

void MigrationPanel::CreateProgramTab() {
    const auto& colors = ThemeManager::Instance().GetColors();
    m_programPage = new wxPanel(m_notebook, wxID_ANY);
    m_programPage->SetBackgroundColour(colors.surface);
    auto* programSizer = new wxBoxSizer(wxVERTICAL);

    m_headerCheckbox = new wxCheckBox(m_programPage, wxID_ANY, L"全选", wxDefaultPosition, wxDefaultSize,
                                      wxCHK_3STATE);
    m_headerCheckbox->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                     false, L"微软雅黑"));
    m_headerCheckbox->Bind(wxEVT_CHECKBOX, &MigrationPanel::OnHeaderCheckbox, this);
    m_headerCheckbox->Hide();
    programSizer->Add(m_headerCheckbox, 0, wxALL, 4);

    m_fileList = new wxListCtrl(m_programPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_fileList->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    m_fileList->EnableCheckBoxes(true);
    m_fileList->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MigrationPanel::OnItemActivated, this);
    m_fileList->Bind(wxEVT_LIST_ITEM_CHECKED, &MigrationPanel::OnListItemChecked, this);
    m_fileList->Bind(wxEVT_LIST_ITEM_UNCHECKED, &MigrationPanel::OnListItemChecked, this);

    SetupListColumnsForType(2);
    programSizer->Add(m_fileList, 1, wxEXPAND | wxALL, 4);
    m_programPage->SetSizer(programSizer);
    m_notebook->AddPage(m_programPage, L"应用迁移");
}

void MigrationPanel::CreateFolderTab() {
    const auto& colors = ThemeManager::Instance().GetColors();
    m_folderPage = new wxPanel(m_notebook, wxID_ANY);
    m_folderPage->SetBackgroundColour(colors.surface);
    auto* folderSizer = new wxBoxSizer(wxVERTICAL);

    m_fileList = new wxListCtrl(m_folderPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_fileList->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    m_fileList->EnableCheckBoxes(true);
    m_fileList->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MigrationPanel::OnItemActivated, this);
    m_fileList->Bind(wxEVT_LIST_ITEM_CHECKED, &MigrationPanel::OnListItemChecked, this);
    m_fileList->Bind(wxEVT_LIST_ITEM_UNCHECKED, &MigrationPanel::OnListItemChecked, this);

    SetupListColumnsForType(1);
    folderSizer->Add(m_fileList, 1, wxEXPAND | wxALL, 4);
    m_folderPage->SetSizer(folderSizer);
    m_notebook->AddPage(m_folderPage, L"大文件夹");
}

void MigrationPanel::OnNotebookPageChanged(wxNotebookEvent& event) {
    int sel = m_notebook->GetSelection();
    m_currentScanType = (sel == 0) ? 1 : 2;  // 0=应用迁移(1), 1=大文件夹(2)

    m_thresholdChoice->Enable(sel == 1);  // 大文件夹 Tab 才需要阈值

    // 重建当前 Tab 的列表内容
    m_items.clear();
    HideExpandPanel();

    if (sel == 0) {
        SetupListColumnsForType(2);
    } else {
        SetupListColumnsForType(1);
    }
    RefreshItemList();
    Layout();
}

void MigrationPanel::SetupListColumnsForType(int scanType) {
    if (!m_fileList) return;
    m_fileList->DeleteAllColumns();
    if (scanType == 1) {
        // 大文件夹 Tab
        m_fileList->AppendColumn(L"名称", wxLIST_FORMAT_LEFT, 180);
        m_fileList->AppendColumn(L"路径", wxLIST_FORMAT_LEFT, 250);
        m_fileList->AppendColumn(L"大小", wxLIST_FORMAT_LEFT, 100);
        m_fileList->AppendColumn(L"类型", wxLIST_FORMAT_LEFT, 100);
        m_fileList->AppendColumn(L"迁移建议", wxLIST_FORMAT_LEFT, 90);
        m_fileList->AppendColumn(L"状态", wxLIST_FORMAT_LEFT, 80);
    } else {
        // 应用迁移 Tab
        m_fileList->AppendColumn(L"程序名称", wxLIST_FORMAT_LEFT, 160);
        m_fileList->AppendColumn(L"大小", wxLIST_FORMAT_LEFT, 90);
        m_fileList->AppendColumn(L"安装路径", wxLIST_FORMAT_LEFT, 280);
        m_fileList->AppendColumn(L"发行商", wxLIST_FORMAT_LEFT, 120);
        m_fileList->AppendColumn(L"安全级别", wxLIST_FORMAT_LEFT, 100);
        m_fileList->AppendColumn(L"状态", wxLIST_FORMAT_LEFT, 80);
    }
}

void MigrationPanel::RefreshItemList() {
    using namespace IceClean::Models;
    using namespace IceClean::Utils;

    if (!m_fileList) return;
    m_fileList->DeleteAllItems();

    for (size_t i = 0; i < m_items.size(); ++i) {
        const auto& item = m_items[i];
        long idx = m_fileList->InsertItem(static_cast<long>(i), L"");

        if (m_currentScanType == 1) {
            m_fileList->SetItem(idx, 0, item.name);
            m_fileList->SetItem(idx, 1, item.sourcePath);
            m_fileList->SetItem(idx, 2, FormatUtil::FormatFileSize(item.size));

            wxString typeStr;
            switch (item.type) {
                case MigrationType::SteamGame:    typeStr = L"Steam游戏"; break;
                case MigrationType::UserFolder:   typeStr = L"用户文件夹"; break;
                case MigrationType::WeChatCache:  typeStr = L"微信缓存"; break;
                case MigrationType::QQCache:      typeStr = L"QQ缓存"; break;
                case MigrationType::CustomFolder: typeStr = L"自定义文件夹"; break;
                case MigrationType::LargeSoftware: typeStr = L"大型软件"; break;
                case MigrationType::InstalledProgram: typeStr = L"已安装应用"; break;
                default: typeStr = L"其他"; break;
            }
            m_fileList->SetItem(idx, 3, typeStr);

            wxString adviceStr;
            switch (item.advice) {
                case MigrationAdvice::Recommended:    adviceStr = L"推荐迁移"; break;
                case MigrationAdvice::Possible:       adviceStr = L"可以迁移"; break;
                case MigrationAdvice::NotRecommended: adviceStr = L"不建议"; break;
                default: adviceStr = L""; break;
            }
            m_fileList->SetItem(idx, 4, adviceStr);

            bool isJunction = JunctionPoint::IsJunction(item.sourcePath);
            if (item.migrated || isJunction) {
                m_fileList->SetItem(idx, 5, L"已迁移");
            } else {
                m_fileList->SetItem(idx, 5, L"可迁移");
            }
        } else {
            m_fileList->SetItem(idx, 0, item.name);
            m_fileList->SetItem(idx, 1, FormatUtil::FormatFileSize(item.size));
            m_fileList->SetItem(idx, 2, item.sourcePath);
            m_fileList->SetItem(idx, 3, item.publisher);

            wxString safetyStr;
            switch (item.programSafety) {
                case ProgramSafetyLevel::Safe:      safetyStr = L"[安全]"; break;
                case ProgramSafetyLevel::Caution:   safetyStr = L"[谨慎]"; break;
                case ProgramSafetyLevel::Dangerous: safetyStr = L"[禁止]"; break;
                default: safetyStr = L""; break;
            }
            m_fileList->SetItem(idx, 4, safetyStr);

            if (item.isRunning) {
                m_fileList->SetItem(idx, 5, L"运行中");
            } else if (JunctionPoint::IsJunction(item.sourcePath)) {
                m_fileList->SetItem(idx, 5, L"已迁移");
            } else {
                m_fileList->SetItem(idx, 5, L"空闲");
            }
        }

        m_fileList->CheckItem(idx, item.selected);
    }

    UpdateHeaderCheckboxState();
}

void MigrationPanel::PopulateThresholdList() {
    m_thresholdChoice->Clear();
    m_thresholdChoice->Append(L"50 MB");
    m_thresholdChoice->Append(L"100 MB");
    m_thresholdChoice->Append(L"200 MB");
    m_thresholdChoice->Append(L"500 MB");
    m_thresholdChoice->Append(L"1000 MB");
    m_thresholdChoice->SetSelection(1);
    m_currentThresholdMB = 100;
}

void MigrationPanel::PopulateDriveList() {
    m_targetDriveChoice->Clear();
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (drives & (1 << i)) {
            wchar_t root[] = {static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0'};
            UINT type = GetDriveTypeW(root);
            if (type == DRIVE_FIXED) {
                if (root[0] == L'C') continue;
                ULARGE_INTEGER freeBytes;
                if (GetDiskFreeSpaceExW(root, &freeBytes, nullptr, nullptr)) {
                    wxString driveLetterStr(1, static_cast<wchar_t>(L'A' + i));
                    wxString label = wxString::Format(L"%s: (%s 可用)",
                        driveLetterStr,
                        IceClean::Utils::FormatUtil::FormatFileSize(freeBytes.QuadPart).c_str());
                    m_targetDriveChoice->Append(label);
                }
            }
        }
    }
    if (m_targetDriveChoice->GetCount() > 0) {
        m_targetDriveChoice->SetSelection(0);
    }
}

void MigrationPanel::SetMigrationItems(const std::vector<IceClean::Models::MigrationItem>& items) {
    m_scanButton->Show();
    m_stopButton->Hide();
    m_stopButton->Enable();
    m_stopButton->SetLabel(L"停止");
    Layout();

    if (m_scanInfoPanel) {
        m_scanInfoPanel->SetState(ScanInfoPanelState::Normal);
        m_scanInfoPanel->SetStatusText(L"扫描完成");
        m_scanInfoPanel->SetProcessingItem(L"");
    }
    if (m_currentPathLabel) m_currentPathLabel->SetLabelText(L"");
    m_items = items;
    HideExpandPanel();
    RefreshItemList();

    m_statusLabel->SetLabel(wxString::Format(L"扫描完成，找到 %d 个可迁移项",
        static_cast<int>(items.size())));
}

void MigrationPanel::SetProgramItems(const std::vector<IceClean::Models::MigrationItem>& items) {
    m_scanButton->Show();
    m_stopButton->Hide();
    m_stopButton->Enable();
    m_stopButton->SetLabel(L"停止");
    Layout();

    if (m_scanInfoPanel) {
        m_scanInfoPanel->SetState(ScanInfoPanelState::Normal);
        m_scanInfoPanel->SetStatusText(L"扫描完成");
        m_scanInfoPanel->SetProcessingItem(L"");
    }
    if (m_currentPathLabel) m_currentPathLabel->SetLabelText(L"");
    m_items = items;
    HideExpandPanel();
    RefreshItemList();

    m_statusLabel->SetLabel(wxString::Format(L"扫描完成，找到 %d 个可迁移应用",
        static_cast<int>(items.size())));
}

void MigrationPanel::UpdateScanProgress(const wxString& phase, const wxString& currentPath,
                                        int foundCount) {
    if (!m_scanInfoPanel) return;
    m_scanInfoPanel->SetState(ScanInfoPanelState::Processing);
    wxString status = phase;
    if (foundCount > 0) {
        status += wxString::Format(L" · 已发现 %d 项", foundCount);
    }
    m_scanInfoPanel->SetStatusText(status);
    if (!currentPath.empty()) {
        m_scanInfoPanel->SetProcessingItem(currentPath);
    }
    if (m_currentPathLabel) {
        if (currentPath.empty()) {
            m_currentPathLabel->SetLabelText(L"");
        } else {
            m_currentPathLabel->SetLabelText(wxString::Format(L"  %s", currentPath));
        }
    }
}

void MigrationPanel::OnMigrationScanProgress(wxThreadEvent& event) {
    if (IsBeingDeleted()) return;
    auto info = event.GetPayload<MigrationScanProgressInfo>();
    UpdateScanProgress(info.phase, info.path, info.foundCount);
}

std::vector<IceClean::Models::MigrationItem> MigrationPanel::GetSelectedItems() const {
    std::vector<IceClean::Models::MigrationItem> selected;
    for (size_t i = 0; i < m_items.size(); ++i) {
        if (m_items[i].selected) {
            selected.push_back(m_items[i]);
        }
    }
    return selected;
}

wxString MigrationPanel::GetTargetDrive() const {
    int sel = m_targetDriveChoice->GetSelection();
    if (sel == wxNOT_FOUND) return L"";
    wxString label = m_targetDriveChoice->GetStringSelection();
    return label.Left(2);
}

void MigrationPanel::OnScanButton(wxCommandEvent& event) {
    m_scanButton->Hide();
    m_stopButton->Show();
    m_stopButton->Enable();
    m_stopButton->SetLabel(L"停止");
    Layout();
    m_statusLabel->SetLabel(L"");
    HideExpandPanel();

    if (m_scanInfoPanel) {
        m_scanInfoPanel->SetState(ScanInfoPanelState::Processing);
        m_scanInfoPanel->SetStatusText(m_currentScanType == 2 ? L"正在扫描大文件夹..." : L"正在扫描已安装应用...");
    }
    if (m_currentPathLabel) m_currentPathLabel->SetLabelText(L"  准备开始扫描...");

    wxThreadEvent scanEvt(wxEVT_MIGRATION_SCAN_REQUEST);
    scanEvt.SetInt(m_currentScanType == 1 ? 0 : m_currentThresholdMB);
    wxPostEvent(GetParent(), scanEvt);
}

void MigrationPanel::OnThresholdChanged(wxCommandEvent& event) {
    int sel = m_thresholdChoice->GetSelection();
    switch (sel) {
        case 0: m_currentThresholdMB = 50;   break;
        case 1: m_currentThresholdMB = 100;  break;
        case 2: m_currentThresholdMB = 200;  break;
        case 3: m_currentThresholdMB = 500;  break;
        case 4: m_currentThresholdMB = 1000; break;
        default: m_currentThresholdMB = 100; break;
    }
}

void MigrationPanel::OnStopButton(wxCommandEvent& event) {
    m_stopButton->Disable();
    m_stopButton->SetLabel(L"正在停止...");
    m_statusLabel->SetLabel(L"正在停止扫描...");
    if (m_currentPathLabel) m_currentPathLabel->SetLabelText(L"  正在停止...");

    wxThreadEvent stopEvt(wxEVT_SCAN_STOP);
    wxPostEvent(GetParent(), stopEvt);
}

void MigrationPanel::OnMigrateButton(wxCommandEvent& event) {
    auto selectedItems = GetSelectedItems();
    if (selectedItems.empty()) {
        wxMessageBox(L"请先选择要迁移的项目", L"提示", wxOK | wxICON_INFORMATION);
        return;
    }

    wxString targetDrive = GetTargetDrive();
    if (targetDrive.IsEmpty()) {
        wxMessageBox(L"请选择目标驱动器", L"提示", wxOK | wxICON_INFORMATION);
        return;
    }

    // 应用迁移时检查是否有"禁止"安全级别的项
    if (m_currentScanType == 1) {
        for (const auto& it : selectedItems) {
            if (it.programSafety == IceClean::Models::ProgramSafetyLevel::Dangerous) {
                wxMessageBox(L"已选择的列表中包含禁止迁移的应用（系统组件），请取消勾选",
                             L"提示", wxOK | wxICON_WARNING);
                return;
            }
        }
    }

    wxString desc = wxString::Format(L"即将将 %d 个项目迁移到 %s\n\n"
        L"迁移后将创建Junction链接，程序可正常运行。\n"
        L"确定继续？",
        static_cast<int>(selectedItems.size()), targetDrive.wx_str());

    ConfirmDialog dlg(this, L"确认迁移", desc,
                      ConfirmDialog::DangerLevel::Caution, L"确认迁移", L"取消");

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    MigrationProgressDlg progressDlg(this);
    progressDlg.Show();

    wxThreadEvent migrateEvt(wxEVT_MIGRATE_PROGRESS);
    migrateEvt.SetInt(0);
    wxPostEvent(GetParent(), migrateEvt);
}

void MigrationPanel::OnDeleteButton(wxCommandEvent& event) {
    auto selectedItems = GetSelectedItems();
    if (selectedItems.empty()) {
        wxMessageBox(L"请先选择要删除的项目", L"提示", wxOK | wxICON_INFORMATION);
        return;
    }

    wxString desc = wxString::Format(
        L"即将永久删除 %d 个项目：\n\n", static_cast<int>(selectedItems.size()));
    size_t showCount = std::min<size_t>(selectedItems.size(), 5);
    uint64_t totalSize = 0;
    for (size_t i = 0; i < showCount; ++i) {
        desc += wxString::Format(L"  • %s\n", selectedItems[i].name.c_str());
        totalSize += selectedItems[i].size;
    }
    if (selectedItems.size() > showCount) {
        desc += wxString::Format(L"  ... 还有 %d 个项目\n",
            static_cast<int>(selectedItems.size() - showCount));
    }
    for (const auto& it : selectedItems) totalSize += it.size;
    desc += wxString::Format(L"\n总大小约 %s\n\n此操作不可撤销，确定删除？",
        IceClean::Utils::FormatUtil::FormatFileSize(totalSize).c_str());

    ConfirmDialog dlg(this, L"确认删除", desc,
                      ConfirmDialog::DangerLevel::Dangerous, L"确认删除", L"取消");

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    int deletedCount = 0;
    uint64_t freedSize = 0;
    for (const auto& it : selectedItems) {
        if (IceClean::Utils::JunctionPoint::IsJunction(it.sourcePath)) continue;
        if (IceClean::Utils::FileUtil::DeleteFolder(it.sourcePath)) {
            deletedCount++;
            freedSize += it.size;
        }
    }

    m_statusLabel->SetLabel(wxString::Format(L"已删除 %d 个项目，释放 %s",
        deletedCount, IceClean::Utils::FormatUtil::FormatFileSize(freedSize).c_str()));

    std::vector<IceClean::Models::MigrationItem> remaining;
    for (const auto& it : m_items) {
        bool wasDeleted = false;
        for (const auto& sel : selectedItems) {
            if (it.sourcePath == sel.sourcePath) {
                wasDeleted = true;
                break;
            }
        }
        if (!wasDeleted) {
            remaining.push_back(it);
        }
    }
    SetMigrationItems(remaining);
}

void MigrationPanel::OnItemActivated(wxListEvent& event) {
    long idx = event.GetIndex();
    if (idx < 0 || static_cast<size_t>(idx) >= m_items.size()) return;

    if (m_expandedIndex == idx) {
        HideExpandPanel();
    } else {
        ShowExpandPanel(static_cast<int>(idx));
    }
}

void MigrationPanel::ShowExpandPanel(int itemIndex) {
    m_expandedIndex = itemIndex;

    const auto& item = m_items[itemIndex];
    m_expandTitle->SetLabelText(wxString::Format(L"▶ %s 的可迁移子目录（≥ %d MB）：",
        item.name, m_currentThresholdMB));
    m_expandContent->SetLabelText(L"正在扫描...");

    Layout();
    m_expandPanel->Show();

    std::wstring parentPath = item.sourcePath;
    int thresholdBytes = m_currentThresholdMB * 1024 * 1024;

    wxTheApp->CallAfter([this, parentPath, thresholdBytes]() {
        std::wstring content = ScanLargeSubDirs(parentPath, thresholdBytes);
        if (m_expandedIndex < 0) return;
        m_expandContent->SetLabelText(content.empty()
            ? wxString(L"（无 ≥ 阈值的大子目录）")
            : wxString(content));
    });
}

void MigrationPanel::HideExpandPanel() {
    m_expandedIndex = -1;
    m_expandPanel->Hide();
    Layout();
}

void MigrationPanel::OnHeaderCheckbox(wxCommandEvent& event) {
    size_t total = m_items.size();
    size_t selected = 0;
    for (const auto& it : m_items) {
        if (it.selected) ++selected;
    }

    bool newChecked;
    if (selected == total) {
        newChecked = false;
    } else {
        newChecked = true;
    }

    for (auto& it : m_items) {
        if (!IceClean::Utils::JunctionPoint::IsJunction(it.sourcePath)) {
            it.selected = newChecked;
        }
    }
    for (size_t i = 0; i < m_items.size(); ++i) {
        m_fileList->CheckItem(static_cast<long>(i), m_items[i].selected);
    }
    UpdateHeaderCheckboxState();
}

void MigrationPanel::OnListItemChecked(wxListEvent& event) {
    long idx = event.GetIndex();
    if (idx >= 0 && static_cast<size_t>(idx) < m_items.size()) {
        m_items[static_cast<size_t>(idx)].selected = m_fileList->IsItemChecked(idx);
    }
    UpdateHeaderCheckboxState();
}

void MigrationPanel::UpdateHeaderCheckboxState() {
    if (m_items.empty()) {
        m_headerCheckbox->Set3StateValue(wxCHK_UNCHECKED);
        m_headerCheckbox->Hide();
        return;
    }

    m_headerCheckbox->Show();

    size_t total = m_items.size();
    size_t selected = 0;
    for (const auto& it : m_items) {
        if (it.selected) ++selected;
    }

    if (selected == 0) {
        m_headerCheckbox->Set3StateValue(wxCHK_UNCHECKED);
    } else if (selected == total) {
        m_headerCheckbox->Set3StateValue(wxCHK_CHECKED);
    } else {
        m_headerCheckbox->Set3StateValue(wxCHK_UNDETERMINED);
    }
}

std::wstring MigrationPanel::ScanLargeSubDirs(const std::wstring& parentPath, int thresholdBytes) {
    std::wstring result;
    WIN32_FIND_DATAW fd;
    std::wstring searchPath = parentPath + L"\\*";
    HANDLE h = FindFirstFileW(searchPath.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return result;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;

        std::wstring childPath = parentPath + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            uint64_t childSize = IceClean::Utils::FileUtil::GetFolderSize(childPath);
            if (childSize >= static_cast<uint64_t>(thresholdBytes)) {
                wxString sizeStr = IceClean::Utils::FormatUtil::FormatFileSize(childSize).c_str();
                result += L"  " + std::wstring(fd.cFileName) + L" (" + sizeStr.ToStdWstring() + L")\n";
            }
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    if (!result.empty() && result.back() == L'\n') result.pop_back();
    return result;
}

} // namespace IceClean::Gui
