#include "DeepCleanPanel.h"
#include "gui/controls/SafetyBadge.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/Events.h"
#include "core/cleaner/RegistryCleaner.h"
#include <thread>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(DeepCleanPanel, wxPanel)
wxEND_EVENT_TABLE()

DeepCleanPanel::DeepCleanPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(*wxWHITE);
    CreateControls();
}

void DeepCleanPanel::CreateControls() {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"深度清理");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(wxColour(0x33, 0x33, 0x33));
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 提示信息
    auto* tipLabel = new wxStaticText(this, wxID_ANY,
        L"深度清理功能涉及系统核心文件，操作前将自动创建系统还原点。");
    tipLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
    tipLabel->SetForegroundColour(wxColour(0xFF, 0x8C, 0x00));
    mainSizer->Add(tipLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 标签页
    m_notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxNB_TOP | wxBORDER_NONE);
    m_notebook->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));

    // 系统清理标签页
    auto* systemTab = new wxPanel(m_notebook);
    systemTab->SetBackgroundColour(*wxWHITE);
    CreateSystemCleanTab(systemTab);
    m_notebook->AddPage(systemTab, L"系统清理");

    // 注册表清理标签页
    auto* registryTab = new wxPanel(m_notebook);
    registryTab->SetBackgroundColour(*wxWHITE);
    CreateRegistryCleanTab(registryTab);
    m_notebook->AddPage(registryTab, L"注册表清理");

    // 隐私清理标签页
    auto* privacyTab = new wxPanel(m_notebook);
    privacyTab->SetBackgroundColour(*wxWHITE);
    CreatePrivacyCleanTab(privacyTab);
    m_notebook->AddPage(privacyTab, L"隐私清理");

    mainSizer->Add(m_notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // 底部按钮
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    bottomSizer->AddStretchSpacer();

    m_cleanButton = new wxButton(this, wxID_ANY, L"开始清理", wxDefaultPosition, wxSize(140, 40));
    m_cleanButton->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
    m_cleanButton->SetBackgroundColour(wxColour(0x00, 0x78, 0xD4));
    m_cleanButton->SetForegroundColour(*wxWHITE);
    m_cleanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnCleanButton, this);
    bottomSizer->Add(m_cleanButton, 0, wxRIGHT, 20);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxBOTTOM, 12);

    SetSizer(mainSizer);
}

void DeepCleanPanel::CreateSystemCleanTab(wxWindow* parent) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // WinSxS组件清理
    {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* check = new wxCheckBox(parent, wxID_ANY, L"WinSxS组件清理");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(parent);
        badge->SetSafetyRating(IceClean::Models::SafetyRating::Caution);
        itemSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);

        sizer->Add(itemSizer, 0, wxLEFT | wxRIGHT, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理WinSxS文件夹中的被取代组件，可释放2-10GB空间。需要管理员权限。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_systemItems.push_back({check, L"winSxS",
            L"清理WinSxS文件夹中的被取代组件", false});
    }

    // CompactOS压缩
    {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* check = new wxCheckBox(parent, wxID_ANY, L"CompactOS压缩");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(parent);
        badge->SetSafetyRating(IceClean::Models::SafetyRating::Safe);
        itemSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);

        sizer->Add(itemSizer, 0, wxLEFT | wxRIGHT, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"压缩Windows系统文件，可释放2-5GB空间。不影响系统运行，但可能略微增加CPU使用率。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_systemItems.push_back({check, L"compactOS",
            L"压缩Windows系统文件", false});
    }

    // 旧Windows安装
    {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* check = new wxCheckBox(parent, wxID_ANY, L"删除旧Windows安装");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(parent);
        badge->SetSafetyRating(IceClean::Models::SafetyRating::Caution);
        itemSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);

        sizer->Add(itemSizer, 0, wxLEFT | wxRIGHT, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"删除Windows.old、$Windows.~BT、$Windows.~WS等旧安装文件。"
            L"删除后将无法回退到旧版本Windows，可释放10-50GB空间。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_systemItems.push_back({check, L"oldWindows",
            L"删除旧Windows安装文件", true});
    }

    // 休眠文件清理
    {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* check = new wxCheckBox(parent, wxID_ANY, L"关闭休眠功能");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(parent);
        badge->SetSafetyRating(IceClean::Models::SafetyRating::Caution);
        itemSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);

        sizer->Add(itemSizer, 0, wxLEFT | wxRIGHT, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"删除hiberfil.sys休眠文件，可释放4-32GB空间。关闭后无法使用休眠功能，快速启动也会受影响。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_systemItems.push_back({check, L"hibernation",
            L"关闭休眠功能并删除休眠文件", true});
    }

    sizer->AddStretchSpacer();
    parent->SetSizer(sizer);
}

void DeepCleanPanel::CreateRegistryCleanTab(wxWindow* parent) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 顶部按钮区域
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);

    m_registryScanButton = new wxButton(parent, wxID_ANY, L"扫描注册表",
                                         wxDefaultPosition, wxSize(120, 36));
    m_registryScanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
    m_registryScanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnRegistryScan, this);
    topSizer->Add(m_registryScanButton, 0, wxRIGHT, 12);

    m_registryCleanButton = new wxButton(parent, wxID_ANY, L"清理选中项",
                                          wxDefaultPosition, wxSize(120, 36));
    m_registryCleanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_registryCleanButton->Enable(false);
    m_registryCleanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnRegistryClean, this);
    topSizer->Add(m_registryCleanButton, 0, wxRIGHT, 12);

    m_registrySelectAllCheck = new wxCheckBox(parent, wxID_ANY, L"全选");
    m_registrySelectAllCheck->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                              false, L"微软雅黑"));
    m_registrySelectAllCheck->Enable(false);
    m_registrySelectAllCheck->Bind(wxEVT_CHECKBOX, &DeepCleanPanel::OnRegistrySelectAll, this);
    topSizer->Add(m_registrySelectAllCheck, 0, wxALIGN_CENTER_VERTICAL);

    topSizer->AddStretchSpacer();

    m_registryStatusLabel = new wxStaticText(parent, wxID_ANY, L"");
    m_registryStatusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_registryStatusLabel->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
    topSizer->Add(m_registryStatusLabel, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(topSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // 列表控件
    m_registryList = new wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxLC_REPORT | wxBORDER_SIMPLE);
    m_registryList->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));

    // 添加列
    m_registryList->AppendColumn(L"类型", wxLIST_FORMAT_LEFT, 120);
    m_registryList->AppendColumn(L"注册表路径", wxLIST_FORMAT_LEFT, 300);
    m_registryList->AppendColumn(L"描述", wxLIST_FORMAT_LEFT, 200);

    sizer->Add(m_registryList, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(8);

    parent->SetSizer(sizer);
}

void DeepCleanPanel::CreatePrivacyCleanTab(wxWindow* parent) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 浏览器Cookies
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器Cookies");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Chrome/Edge/Firefox的Cookies数据。清理后需要重新登录网站。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"cookies",
            L"清理浏览器Cookies数据"});
    }

    // 浏览器历史记录
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器历史记录");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理浏览器浏览历史记录。清理后无法恢复访问记录。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"history",
            L"清理浏览器浏览历史记录"});
    }

    // 表单自动填充数据
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"表单自动填充数据");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理浏览器保存的表单自动填充数据，包括用户名、地址等。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(wxColour(0x99, 0x99, 0x99));
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"formData",
            L"清理浏览器表单自动填充数据"});
    }

    sizer->AddStretchSpacer();
    parent->SetSizer(sizer);
}

void DeepCleanPanel::OnCleanButton(wxCommandEvent& event) {
    // 检查是否有危险操作
    bool hasDangerous = false;
    wxString dangerousItems;

    for (const auto& item : m_systemItems) {
        if (item.checkbox->GetValue() && item.isDangerous) {
            hasDangerous = true;
            dangerousItems += L"• " + item.checkbox->GetLabel() + L"\n";
        }
    }

    if (hasDangerous) {
        ConfirmDialog dlg(this, L"确认操作",
            L"以下操作可能影响系统功能，确认继续？\n\n" + dangerousItems +
            L"\n操作前将自动创建系统还原点。",
            ConfirmDialog::DangerLevel::Caution, L"确认清理", L"取消");

        if (dlg.ShowModal() != wxID_OK) {
            return;
        }
    }

    // 发送深度清理事件，携带选中项ID
    auto selectedIds = GetSelectedIds();
    if (selectedIds.empty()) {
        wxMessageBox(L"请至少选择一项清理内容。", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    wxThreadEvent cleanEvt(wxEVT_CLEAN_PROGRESS);
    cleanEvt.SetInt(1);  // 1=深度清理
    cleanEvt.SetPayload(selectedIds);
    wxPostEvent(GetParent(), cleanEvt);
}

std::vector<wxString> DeepCleanPanel::GetSelectedIds() const {
    std::vector<wxString> ids;
    for (const auto& item : m_systemItems) {
        if (item.checkbox->GetValue()) {
            ids.push_back(item.id);
        }
    }
    for (const auto& item : m_privacyItems) {
        if (item.checkbox->GetValue()) {
            ids.push_back(item.id);
        }
    }
    return ids;
}

void DeepCleanPanel::OnRegistryScan(wxCommandEvent& event) {
    m_registryScanButton->Enable(false);
    m_registryStatusLabel->SetLabelText(L"正在扫描...");

    // 在后台线程执行扫描
    std::thread([this]() {
        IceClean::Core::Cleaner::RegistryCleaner cleaner;
        auto items = cleaner.ScanInvalidItems();

        // 回到主线程更新UI
        CallAfter([this, items = std::move(items)]() mutable {
            m_registryItems = std::move(items);
            m_registryList->DeleteAllItems();

            for (int i = 0; i < static_cast<int>(m_registryItems.size()); ++i) {
                const auto& item = m_registryItems[i];
                long idx = m_registryList->InsertItem(i, GetTypeString(item.type));
                m_registryList->SetItem(idx, 1, item.keyPath);
                m_registryList->SetItem(idx, 2, item.description);
            }

            m_registryStatusLabel->SetLabelText(
                wxString::Format(L"共发现 %d 个无效注册表项", static_cast<int>(m_registryItems.size())));
            m_registryScanButton->Enable(true);
            m_registryCleanButton->Enable(!m_registryItems.empty());
            m_registrySelectAllCheck->Enable(!m_registryItems.empty());
        });
    }).detach();
}

void DeepCleanPanel::OnRegistryClean(wxCommandEvent& event) {
    // 收集选中的项（使用选中行而非checkbox，wx3.3不支持wxLC_CHECKBOX）
    std::vector<IceClean::Core::Cleaner::RegistryInvalidItem> selectedItems;
    for (int i = 0; i < static_cast<int>(m_registryItems.size()); ++i) {
        if (m_registryList->GetItemState(i, wxLIST_STATE_SELECTED) & wxLIST_STATE_SELECTED) {
            selectedItems.push_back(m_registryItems[i]);
        }
    }

    if (selectedItems.empty()) {
        wxMessageBox(L"请至少选择一项要清理的注册表项。\n(点击行可选中)", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    ConfirmDialog dlg(this, L"确认注册表清理",
        wxString::Format(L"即将清理 %d 个无效注册表项。\n\n清理前将自动备份注册表，确认继续？",
                          static_cast<int>(selectedItems.size())),
        ConfirmDialog::DangerLevel::Caution, L"确认清理", L"取消");

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    m_registryCleanButton->Enable(false);
    m_registryStatusLabel->SetLabelText(L"正在清理...");

    // 在后台线程执行清理
    std::thread([this, selectedItems = std::move(selectedItems)]() mutable {
        IceClean::Core::Cleaner::RegistryCleaner cleaner;
        auto result = cleaner.Clean(selectedItems, L"", nullptr);

        CallAfter([this, result = std::move(result)]() mutable {
            m_registryStatusLabel->SetLabelText(
                wxString::Format(L"清理完成，共处理 %d 项", result.cleanedFileCount));
            m_registryCleanButton->Enable(true);

            // 触发重新扫描
            wxCommandEvent scanEvt(wxEVT_BUTTON, m_registryScanButton->GetId());
            m_registryScanButton->GetEventHandler()->AddPendingEvent(scanEvt);
        });
    }).detach();
}

void DeepCleanPanel::OnRegistrySelectAll(wxCommandEvent& event) {
    bool select = m_registrySelectAllCheck->GetValue();
    for (int i = 0; i < m_registryList->GetItemCount(); ++i) {
        if (select) {
            m_registryList->SetItemState(i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        } else {
            m_registryList->SetItemState(i, 0, wxLIST_STATE_SELECTED);
        }
    }
}

wxString DeepCleanPanel::GetTypeString(IceClean::Core::Cleaner::RegistryInvalidItem::Type type) const {
    switch (type) {
        case IceClean::Core::Cleaner::RegistryInvalidItem::Type::InvalidUninstall:
            return L"无效卸载信息";
        case IceClean::Core::Cleaner::RegistryInvalidItem::Type::InvalidStartup:
            return L"无效启动项";
        case IceClean::Core::Cleaner::RegistryInvalidItem::Type::InvalidFileAssoc:
            return L"无效文件关联";
        default:
            return L"其他";
    }
}

} // namespace IceClean::Gui
