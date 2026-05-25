#include "MigrationPanel.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/dialogs/MigrationProgressDlg.h"
#include "gui/Events.h"
#include "utils/FormatUtil.h"

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(MigrationPanel, wxPanel)
wxEND_EVENT_TABLE()

MigrationPanel::MigrationPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(*wxWHITE);
    CreateControls();
}

void MigrationPanel::CreateControls() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"智能迁移");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* descLabel = new wxStaticText(this, wxID_ANY,
        L"将C盘大文件迁移到其他分区，通过Junction链接让程序无感知运行。");
    descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    descLabel->SetForegroundColour(wxColour(0x66, 0x66, 0x66));
    mainSizer->Add(descLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // 扫描按钮
    m_scanButton = new wxButton(this, wxID_ANY, L"扫描大文件", wxDefaultPosition, wxSize(140, 36));
    m_scanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                 false, L"微软雅黑"));
    m_scanButton->SetBackgroundColour(wxColour(0x00, 0x78, 0xD4));
    m_scanButton->SetForegroundColour(*wxWHITE);
    m_scanButton->Bind(wxEVT_BUTTON, &MigrationPanel::OnScanButton, this);
    mainSizer->Add(m_scanButton, 0, wxLEFT, 20);
    mainSizer->AddSpacer(12);

    // 文件列表
    m_fileList = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_NONE);
    m_fileList->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));

    m_fileList->AppendColumn(L"✓", wxLIST_FORMAT_LEFT, 30);     // 选择框
    m_fileList->AppendColumn(L"名称", wxLIST_FORMAT_LEFT, 180);  // 名称
    m_fileList->AppendColumn(L"路径", wxLIST_FORMAT_LEFT, 250);  // 路径
    m_fileList->AppendColumn(L"大小", wxLIST_FORMAT_LEFT, 100);  // 大小
    m_fileList->AppendColumn(L"类型", wxLIST_FORMAT_LEFT, 100);  // 类型
    m_fileList->AppendColumn(L"迁移建议", wxLIST_FORMAT_LEFT, 90); // 迁移建议

    mainSizer->Add(m_fileList, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // 底部操作栏
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);

    // 目标驱动器选择
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

    m_migrateButton = new wxButton(this, wxID_ANY, L"开始迁移", wxDefaultPosition, wxSize(140, 40));
    m_migrateButton->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                    false, L"微软雅黑"));
    m_migrateButton->SetBackgroundColour(wxColour(0x00, 0x78, 0xD4));
    m_migrateButton->SetForegroundColour(*wxWHITE);
    m_migrateButton->Bind(wxEVT_BUTTON, &MigrationPanel::OnMigrateButton, this);
    bottomSizer->Add(m_migrateButton, 0, wxRIGHT, 20);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxBOTTOM, 12);

    SetSizer(mainSizer);
}

void MigrationPanel::PopulateDriveList() {
    m_targetDriveChoice->Clear();

    // 枚举可用驱动器
    DWORD drives = GetLogicalDrives();
    for (int i = 0; i < 26; ++i) {
        if (drives & (1 << i)) {
            wchar_t root[] = {static_cast<wchar_t>(L'A' + i), L':', L'\\', L'\0'};
            UINT type = GetDriveTypeW(root);
            if (type == DRIVE_FIXED) {
                // 跳过C盘
                if (root[0] == L'C') continue;

                // 获取可用空间
                ULARGE_INTEGER freeBytes;
                if (GetDiskFreeSpaceExW(root, &freeBytes, nullptr, nullptr)) {
                    wxString label = wxString::Format(L"%c: (%s 可用)",
                        L'A' + i,
                        IceClean::Utils::FormatUtil::FormatFileSize(freeBytes.QuadPart));
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
    m_items = items;
    m_fileList->DeleteAllItems();

    using namespace IceClean::Models;
    using namespace IceClean::Utils;

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];
        long idx = m_fileList->InsertItem(static_cast<long>(i), item.selected ? L"✓" : L"");

        m_fileList->SetItem(idx, 1, item.name);
        m_fileList->SetItem(idx, 2, item.sourcePath);
        m_fileList->SetItem(idx, 3, FormatUtil::FormatFileSize(item.size));

        // 类型
        wxString typeStr;
        switch (item.type) {
            case MigrationType::SteamGame:    typeStr = L"Steam游戏"; break;
            case MigrationType::UserFolder:   typeStr = L"用户文件夹"; break;
            case MigrationType::WeChatCache:  typeStr = L"微信缓存"; break;
            case MigrationType::QQCache:      typeStr = L"QQ缓存"; break;
            case MigrationType::CustomFolder: typeStr = L"自定义文件夹"; break;
            case MigrationType::LargeSoftware: typeStr = L"大型软件"; break;
        }
        m_fileList->SetItem(idx, 4, typeStr);

        // 迁移建议
        wxString adviceStr;
        switch (item.advice) {
            case MigrationAdvice::Recommended:    adviceStr = L"推荐迁移"; break;
            case MigrationAdvice::Possible:       adviceStr = L"可以迁移"; break;
            case MigrationAdvice::NotRecommended: adviceStr = L"不建议"; break;
        }
        m_fileList->SetItem(idx, 5, adviceStr);
    }
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
    return label.Left(2); // "D:" 等
}

void MigrationPanel::OnScanButton(wxCommandEvent& event) {
    m_scanButton->Disable();
    m_scanButton->SetLabel(L"扫描中...");

    // 发送扫描事件，由MainWindow处理
    wxThreadEvent scanEvt(wxEVT_SCAN_PROGRESS);
    scanEvt.SetInt(1); // 1 = 迁移扫描
    wxPostEvent(GetParent(), scanEvt);
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

    // 确认对话框
    wxString desc = wxString::Format(L"即将将 %d 个项目迁移到 %s\n\n"
        L"迁移后将创建Junction链接，程序可正常运行。\n"
        L"确定继续？",
        static_cast<int>(selectedItems.size()), targetDrive);

    ConfirmDialog dlg(this, L"确认迁移", desc,
                      ConfirmDialog::DangerLevel::Caution, L"确认迁移", L"取消");

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    // 显示迁移进度对话框
    MigrationProgressDlg progressDlg(this);
    progressDlg.Show();

    // 发送迁移事件
    wxThreadEvent migrateEvt(wxEVT_MIGRATE_PROGRESS);
    migrateEvt.SetInt(0);
    wxPostEvent(GetParent(), migrateEvt);
}

void MigrationPanel::OnItemChecked(wxListEvent& event) {
    long idx = event.GetIndex();
    if (idx >= 0 && static_cast<size_t>(idx) < m_items.size()) {
        m_items[idx].selected = !m_items[idx].selected;
        m_fileList->SetItemText(idx, 0, m_items[idx].selected ? L"✓" : L"");
    }
}

} // namespace IceClean::Gui
