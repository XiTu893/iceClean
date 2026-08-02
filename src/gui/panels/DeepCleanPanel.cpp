#include "DeepCleanPanel.h"
#include "gui/controls/ThemeManager.h"
#include "gui/controls/SafetyBadge.h"
#include "gui/dialogs/ConfirmDialog.h"
#include "gui/Events.h"
#include "core/cleaner/RegistryCleaner.h"
#include "utils/FileUtil.h"
#include "utils/FormatUtil.h"
#include <thread>
#include <chrono>
#include <algorithm>
#include <shlobj.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(DeepCleanPanel, wxPanel)
wxEND_EVENT_TABLE()

DeepCleanPanel::DeepCleanPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();
}

void DeepCleanPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(12);

    // 标题
    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"深度清理");
    titleLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    titleLabel->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(titleLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 提示信息
    auto* tipLabel = new wxStaticText(this, wxID_ANY,
        L"深度清理功能涉及系统核心文件，操作前将自动创建系统还原点。");
    tipLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
    tipLabel->SetForegroundColour(colors.warning);
    mainSizer->Add(tipLabel, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 标签页
    m_notebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxNB_TOP | wxBORDER_NONE);
    m_notebook->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                               false, L"微软雅黑"));
    m_notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [this](wxBookCtrlEvent& event) {
        // 注册表tab和软件专清tab有独立按钮，隐藏底部"开始清理"按钮
        int page = m_notebook->GetSelection();
        if (page == 1 || page == 3) {  // 注册表清理 或 软件专清
            m_cleanButton->Hide();
        } else {
            m_cleanButton->Show();
            // 更新按钮文本
            if (page == 0) {
                m_cleanButton->SetLabel(L"开始系统清理");
            } else {
                m_cleanButton->SetLabel(L"开始隐私清理");
            }
        }
        Layout();
        event.Skip();
    });

    // 系统清理标签页
    auto* systemTab = new wxPanel(m_notebook);
    systemTab->SetBackgroundColour(colors.background);
    CreateSystemCleanTab(systemTab);
    m_notebook->AddPage(systemTab, L"系统清理");

    // 注册表清理标签页
    auto* registryTab = new wxPanel(m_notebook);
    registryTab->SetBackgroundColour(colors.background);
    CreateRegistryCleanTab(registryTab);
    m_notebook->AddPage(registryTab, L"注册表清理");

    // 隐私清理标签页
    auto* privacyTab = new wxPanel(m_notebook);
    privacyTab->SetBackgroundColour(colors.background);
    CreatePrivacyCleanTab(privacyTab);
    m_notebook->AddPage(privacyTab, L"隐私清理");

    // 软件专清标签页
    auto* softwareTab = new wxPanel(m_notebook);
    softwareTab->SetBackgroundColour(colors.background);
    CreateSoftwareCacheTab(softwareTab);
    m_notebook->AddPage(softwareTab, L"软件专清");

    mainSizer->Add(m_notebook, 1, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // 底部按钮
    auto* bottomSizer = new wxBoxSizer(wxHORIZONTAL);
    bottomSizer->AddStretchSpacer();

    m_cleanButton = new wxButton(this, wxID_ANY, L"开始系统清理", wxDefaultPosition, wxSize(160, 40));
    m_cleanButton->SetName("btn_primary_clean");
    m_cleanButton->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
    m_cleanButton->SetBackgroundColour(colors.accent);
    m_cleanButton->SetForegroundColour(*wxWHITE);
    m_cleanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnCleanButton, this);
    bottomSizer->Add(m_cleanButton, 0, wxRIGHT, 20);

    mainSizer->Add(bottomSizer, 0, wxEXPAND | wxBOTTOM, 12);

    SetSizer(mainSizer);
}

void DeepCleanPanel::CreateSystemCleanTab(wxWindow* parent) {
    const auto& colors = ThemeManager::Instance().GetColors();
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
        desc->SetForegroundColour(colors.textDisabled);
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
        desc->SetForegroundColour(colors.textDisabled);
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
        desc->SetForegroundColour(colors.textDisabled);
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
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_systemItems.push_back({check, L"hibernation",
            L"关闭休眠功能并删除休眠文件", true});
    }

    sizer->AddStretchSpacer();
    parent->SetSizer(sizer);
}

void DeepCleanPanel::CreateRegistryCleanTab(wxWindow* parent) {
    const auto& colors = ThemeManager::Instance().GetColors();
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
    m_registryStatusLabel->SetForegroundColour(colors.textDisabled);
    topSizer->Add(m_registryStatusLabel, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(topSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // 列表控件
    m_registryList = new wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     wxLC_REPORT | wxBORDER_SIMPLE);
    m_registryList->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));

    // 创建复选框图片列表（0=未勾选, 1=已勾选）
    auto* imgList = new wxImageList(16, 16, true, 2);
    wxBitmap uncheckedBmp(16, 16);
    wxBitmap checkedBmp(16, 16);
    {
        wxMemoryDC dc;
        // 未勾选
        dc.SelectObject(uncheckedBmp);
        dc.SetBackground(colors.background);
        dc.Clear();
        dc.SetPen(wxPen(colors.border, 1));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(2, 2, 12, 12);
        dc.SelectObject(wxNullBitmap);
        // 已勾选
        dc.SelectObject(checkedBmp);
        dc.SetBackground(colors.background);
        dc.Clear();
        dc.SetPen(wxPen(colors.accent, 1));
        dc.SetBrush(wxBrush(colors.accent));
        dc.DrawRectangle(2, 2, 12, 12);
        dc.SetPen(wxPen(*wxWHITE, 2));
        dc.DrawLine(4, 8, 7, 11);
        dc.DrawLine(7, 11, 12, 4);
        dc.SelectObject(wxNullBitmap);
    }
    imgList->Add(uncheckedBmp);
    imgList->Add(checkedBmp);
    m_registryList->AssignImageList(imgList, wxIMAGE_LIST_SMALL);

    // 添加列（第一列为复选框列）
    m_registryList->AppendColumn(L" ", wxLIST_FORMAT_CENTER, 30);
    m_registryList->AppendColumn(L"类型", wxLIST_FORMAT_LEFT, 110);
    m_registryList->AppendColumn(L"注册表路径", wxLIST_FORMAT_LEFT, 280);
    m_registryList->AppendColumn(L"描述", wxLIST_FORMAT_LEFT, 180);

    // 点击行时切换复选框（使用左键按下事件，单击即可触发）
    m_registryList->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& mouseEvent) {
        int flags = wxLIST_HITTEST_ONITEM;
        long idx = m_registryList->HitTest(mouseEvent.GetPosition(), flags);
        if (idx >= 0 && idx < static_cast<long>(m_registryChecked.size())) {
            // 切换复选框状态
            m_registryChecked[idx] = !m_registryChecked[idx];
            m_registryList->SetItemImage(idx, m_registryChecked[idx] ? 1 : 0);

            // 更新全选复选框状态
            bool allChecked = true;
            for (size_t i = 0; i < m_registryChecked.size(); ++i) {
                if (!m_registryChecked[i]) { allChecked = false; break; }
            }
            m_registrySelectAllCheck->SetValue(allChecked);
        }
        mouseEvent.Skip();
    });

    sizer->Add(m_registryList, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(8);

    parent->SetSizer(sizer);
}

void DeepCleanPanel::CreatePrivacyCleanTab(wxWindow* parent) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 浏览器Cookies
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器Cookies");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Chrome/Edge/Firefox/Brave/Vivaldi/Opera的Cookies数据。清理后需要重新登录网站。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
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
            L"清理浏览器浏览历史记录，包括下载历史。清理后无法恢复访问记录。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
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
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"formData",
            L"清理浏览器表单自动填充数据"});
    }

    // 浏览器缓存
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器缓存");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理浏览器缓存文件，可释放磁盘空间。清理后网页首次加载可能稍慢。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"browserCache",
            L"清理浏览器缓存文件"});
    }

    // 浏览器会话数据
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器会话数据");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理浏览器保存的标签页和会话恢复数据。清理后浏览器无法恢复上次打开的标签页。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"session",
            L"清理浏览器会话数据"});
    }

    // 浏览器保存的密码
    {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* check = new wxCheckBox(parent, wxID_ANY, L"浏览器保存的密码");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* badge = new SafetyBadge(parent);
        badge->SetSafetyRating(IceClean::Models::SafetyRating::Dangerous);
        itemSizer->Add(badge, 0, wxALIGN_CENTER_VERTICAL);

        sizer->Add(itemSizer, 0, wxLEFT | wxRIGHT, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"⚠ 清理后所有保存的网站密码将被删除，需要重新输入所有密码。此操作不可恢复！");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.danger);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"passwords",
            L"清理浏览器保存的密码", true});
    }

    // 最近文档记录
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"最近文档记录");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Windows最近打开的文档、文件夹快捷方式和跳转列表。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"recentDocs",
            L"清理最近文档记录"});
    }

    // 剪贴板历史
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"剪贴板历史");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Windows剪贴板历史记录（Win+V）。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"clipboardHistory",
            L"清理剪贴板历史记录"});
    }

    // Office最近文件
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"Office最近文件记录");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Microsoft Office(Word/Excel/PowerPoint)的最近打开文件记录。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"officeRecent",
            L"清理Office最近文件记录"});
    }

    // 压缩软件历史
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"压缩软件历史记录");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理WinRAR/7-Zip等压缩软件的历史记录和临时解压文件。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"archiveHistory",
            L"清理压缩软件历史记录"});
    }

    // 下载器历史
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"下载器历史记录");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理迅雷/IDM等下载工具的下载历史记录。不会删除已下载的文件。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"downloadHistory",
            L"清理下载器历史记录"});
    }

    // 缩略图缓存
    {
        auto* check = new wxCheckBox(parent, wxID_ANY, L"缩略图缓存");
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        sizer->Add(check, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        auto* desc = new wxStaticText(parent, wxID_ANY,
            L"清理Windows资源管理器的缩略图缓存。清理后首次打开文件夹时缩略图会重新生成。");
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        sizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_privacyItems.push_back({check, L"thumbnailCache",
            L"清理缩略图缓存"});
    }

    sizer->AddStretchSpacer();
    parent->SetSizer(sizer);
}

void DeepCleanPanel::OnCleanButton(wxCommandEvent& event) {
    int currentPage = m_notebook->GetSelection();

    // 收集当前tab的勾选项
    auto selectedIds = GetSelectedIds();
    if (selectedIds.empty()) {
        wxString tabName = (currentPage == 0) ? L"系统清理" : L"隐私清理";
        wxMessageBox(wxString::Format(L"请在\"%s\"标签页中至少勾选一项。", tabName.wx_str()),
                     L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

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
            m_registryChecked.assign(m_registryItems.size(), false);
            m_registryList->DeleteAllItems();

            for (int i = 0; i < static_cast<int>(m_registryItems.size()); ++i) {
                const auto& item = m_registryItems[i];
                long idx = m_registryList->InsertItem(i, L" ", 0);  // 0=未勾选图片
                m_registryList->SetItem(idx, 1, GetTypeString(item.type));
                m_registryList->SetItem(idx, 2, item.keyPath);
                m_registryList->SetItem(idx, 3, item.description);
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
    // 收集勾选的项
    std::vector<IceClean::Core::Cleaner::RegistryInvalidItem> selectedItems;
    for (int i = 0; i < static_cast<int>(m_registryItems.size()); ++i) {
        if (i < static_cast<int>(m_registryChecked.size()) && m_registryChecked[i]) {
            selectedItems.push_back(m_registryItems[i]);
        }
    }

    if (selectedItems.empty()) {
        wxMessageBox(L"请至少勾选一项要清理的注册表项。", L"IceClean", wxOK | wxICON_WARNING, this);
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
    m_registryStatusLabel->SetLabelText(L"正在备份注册表...");

    // 生成自动备份路径
    std::wstring backupDir;
    wchar_t appDataPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appDataPath))) {
        backupDir = std::wstring(appDataPath) + L"\\IceClean\\registry_backup";
        CreateDirectoryW(backupDir.c_str(), nullptr);
    }

    // 生成带时间戳的备份文件名
    std::wstring backupPath;
    if (!backupDir.empty()) {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        struct tm tmBuf {};
        localtime_s(&tmBuf, &timeT);
        wchar_t timeStr[32] = {};
        wcsftime(timeStr, 32, L"%Y%m%d_%H%M%S", &tmBuf);
        backupPath = backupDir + L"\\reg_backup_" + timeStr + L".reg";
    }

    // 清理旧备份（保留最近5次）
    if (!backupDir.empty()) {
        WIN32_FIND_DATAW findData;
        std::wstring searchPattern = backupDir + L"\\reg_backup_*.reg";
        std::vector<std::wstring> backupFiles;
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    backupFiles.push_back(backupDir + L"\\" + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        // 按名称排序（时间戳格式保证排序=时间排序）
        std::sort(backupFiles.begin(), backupFiles.end());
        // 删除旧备份，保留最近5个
        while (backupFiles.size() > 5) {
            DeleteFileW(backupFiles.front().c_str());
            backupFiles.erase(backupFiles.begin());
        }
    }

    m_registryStatusLabel->SetLabelText(L"正在清理...");

    // 在后台线程执行清理
    std::thread([this, selectedItems = std::move(selectedItems), backupPath]() mutable {
        IceClean::Core::Cleaner::RegistryCleaner cleaner;
        auto result = cleaner.Clean(selectedItems, backupPath, nullptr);

        CallAfter([this, result = std::move(result)]() mutable {
            m_registryStatusLabel->SetLabelText(
                wxString::Format(L"清理完成，成功 %d 项，失败 %d 项", result.cleanedFileCount, result.failedFileCount));
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
        m_registryChecked[i] = select;
        m_registryList->SetItemImage(i, select ? 1 : 0);
    }
}

wxString DeepCleanPanel::GetTypeString(IceClean::Core::Cleaner::RegistryInvalidItem::Type type) const {
    using Type = IceClean::Core::Cleaner::RegistryInvalidItem::Type;
    switch (type) {
        case Type::InvalidUninstall:  return L"无效卸载信息";
        case Type::InvalidStartup:    return L"无效启动项";
        case Type::InvalidFileAssoc:  return L"无效文件关联";
        case Type::InvalidSharedDLL:  return L"无效共享DLL";
        case Type::InvalidFont:       return L"无效字体引用";
        case Type::InvalidHelpFile:   return L"无效帮助文件";
        case Type::InvalidAppPath:    return L"无效应用路径";
        case Type::InvalidCOM:        return L"无效COM组件";
        case Type::InvalidMUI:        return L"无效MUI缓存";
        case Type::InvalidEnvVar:     return L"无效环境变量";
        case Type::InvalidTrayNotify: return L"无效托盘缓存";
        case Type::InvalidSound:      return L"无效声音关联";
        default:                      return L"其他";
    }
}

void DeepCleanPanel::CreateSoftwareCacheTab(wxWindow* parent) {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddSpacer(8);

    // 顶部按钮区域
    auto* topSizer = new wxBoxSizer(wxHORIZONTAL);

    m_softwareScanButton = new wxButton(parent, wxID_ANY, L"扫描缓存",
                                         wxDefaultPosition, wxSize(120, 36));
    m_softwareScanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                          false, L"微软雅黑"));
    m_softwareScanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnSoftwareScan, this);
    topSizer->Add(m_softwareScanButton, 0, wxRIGHT, 12);

    m_softwareCleanButton = new wxButton(parent, wxID_ANY, L"清理选中",
                                          wxDefaultPosition, wxSize(120, 36));
    m_softwareCleanButton->SetName("btn_primary_software_clean");
    m_softwareCleanButton->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_softwareCleanButton->SetBackgroundColour(colors.accent);
    m_softwareCleanButton->SetForegroundColour(*wxWHITE);
    m_softwareCleanButton->Enable(false);
    m_softwareCleanButton->Bind(wxEVT_BUTTON, &DeepCleanPanel::OnSoftwareClean, this);
    topSizer->Add(m_softwareCleanButton, 0, wxRIGHT, 12);

    topSizer->AddStretchSpacer();

    m_softwareStatusLabel = new wxStaticText(parent, wxID_ANY, L"点击\"扫描缓存\"检测常用软件缓存");
    m_softwareStatusLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                           false, L"微软雅黑"));
    m_softwareStatusLabel->SetForegroundColour(colors.textDisabled);
    topSizer->Add(m_softwareStatusLabel, 0, wxALIGN_CENTER_VERTICAL);

    sizer->Add(topSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // 软件列表区域（使用wxScrolledWindow）
    auto* scrollWin = new wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                            wxVSCROLL | wxBORDER_NONE);
    scrollWin->SetBackgroundColour(colors.background);
    scrollWin->SetScrollRate(0, 10);

    auto* listSizer = new wxBoxSizer(wxVERTICAL);

    // 定义软件缓存项
    struct SoftwareDef {
        const wchar_t* name;
        const wchar_t* path;
        const wchar_t* desc;
    };

    const SoftwareDef softwareDefs[] = {
        { L"微信缓存", L"%USERPROFILE%\\Documents\\WeChat Files", L"微信聊天图片、视频、文件缓存" },
        { L"QQ缓存", L"%USERPROFILE%\\Documents\\Tencent Files", L"QQ聊天图片、视频缓存" },
        { L"迅雷缓存", L"%USERPROFILE%\\AppData\\Local\\Thunder Network", L"迅雷下载缓存和临时文件" },
        { L"爱奇艺缓存", L"%USERPROFILE%\\AppData\\Local\\Qiyi", L"爱奇艺视频缓存" },
        { L"腾讯视频缓存", L"%USERPROFILE%\\AppData\\Local\\Tencent\\QLive", L"腾讯视频缓存" },
        { L"优酷缓存", L"%USERPROFILE%\\AppData\\Local\\Youku", L"优酷视频缓存" },
        { L"哔哩哔哩缓存", L"%USERPROFILE%\\AppData\\Local\\bilibili", L"哔哩哔哩视频缓存" },
        { L"WPS缓存", L"%USERPROFILE%\\AppData\\Local\\Kingsoft", L"WPS云端缓存和临时文件" },
        { L"钉钉缓存", L"%USERPROFILE%\\AppData\\Local\\DingTalk", L"钉钉缓存文件" },
        { L"有道云笔记缓存", L"%USERPROFILE%\\AppData\\Local\\Youdao", L"有道云笔记和词典缓存" },
    };

    for (const auto& def : softwareDefs) {
        auto* itemSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* check = new wxCheckBox(scrollWin, wxID_ANY, def.name);
        check->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        itemSizer->Add(check, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        auto* sizeLabel = new wxStaticText(scrollWin, wxID_ANY, L"");
        sizeLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
        sizeLabel->SetForegroundColour(colors.accent);
        sizeLabel->SetMinSize(wxSize(80, -1));
        itemSizer->Add(sizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

        listSizer->Add(itemSizer, 0, wxLEFT | wxRIGHT | wxTOP, 12);

        auto* desc = new wxStaticText(scrollWin, wxID_ANY, def.desc);
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textDisabled);
        desc->Wrap(550);
        listSizer->Add(desc, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        m_softwareCacheItems.push_back({check, def.name, def.path, sizeLabel, 0});
    }

    listSizer->AddStretchSpacer();
    scrollWin->SetSizer(listSizer);
    scrollWin->FitInside();

    sizer->Add(scrollWin, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
    sizer->AddSpacer(8);

    parent->SetSizer(sizer);
}

void DeepCleanPanel::OnSoftwareScan(wxCommandEvent& event) {
    m_softwareScanButton->Enable(false);
    m_softwareStatusLabel->SetLabelText(L"正在扫描...");

    // 重置大小标签
    for (auto& item : m_softwareCacheItems) {
        item.cacheSize = 0;
        item.sizeLabel->SetLabelText(L"");
    }

    std::thread([this]() {
        auto result = m_softwareCacheScanner.Scan();

        CallAfter([this, result = std::move(result)]() mutable {
            if (IsBeingDeleted()) return;

            m_softwareScanResult = std::move(result);

            // 按路径前缀统计每个软件的缓存大小
            for (const auto& scanItem : m_softwareScanResult.items) {
                const auto& path = scanItem.path;
                for (auto& cacheItem : m_softwareCacheItems) {
                    auto expandedPath = Utils::Win32Util::ExpandEnvVars(cacheItem.cachePath);
                    // 检查扫描到的文件是否属于该软件的缓存路径
                    if (path.find(expandedPath) == 0) {
                        cacheItem.cacheSize += scanItem.size;
                    }
                }
            }

            // 更新UI
            uint64_t totalSize = 0;
            int foundCount = 0;
            for (auto& item : m_softwareCacheItems) {
                if (item.cacheSize > 0) {
                    item.sizeLabel->SetLabelText(
                        wxString::Format(L"%s", Utils::FormatUtil::FormatFileSize(item.cacheSize)));
                    item.checkbox->SetValue(true);
                    totalSize += item.cacheSize;
                    foundCount++;
                } else {
                    item.sizeLabel->SetLabelText(L"0 B");
                    item.checkbox->SetValue(false);
                    item.checkbox->Enable(false);
                }
            }

            m_softwareStatusLabel->SetLabelText(
                wxString::Format(L"共发现 %d 个软件缓存，总计 %s",
                    foundCount, Utils::FormatUtil::FormatFileSize(totalSize)));
            m_softwareScanButton->Enable(true);
            m_softwareCleanButton->Enable(foundCount > 0);
        });
    }).detach();
}

void DeepCleanPanel::OnSoftwareClean(wxCommandEvent& event) {
    // 收集选中的缓存项
    std::vector<std::wstring> selectedPaths;
    uint64_t totalSize = 0;
    for (const auto& item : m_softwareCacheItems) {
        if (item.checkbox->IsEnabled() && item.checkbox->GetValue()) {
            selectedPaths.push_back(Utils::Win32Util::ExpandEnvVars(item.cachePath));
            totalSize += item.cacheSize;
        }
    }

    if (selectedPaths.empty()) {
        wxMessageBox(L"请至少勾选一项要清理的软件缓存。", L"IceClean", wxOK | wxICON_WARNING, this);
        return;
    }

    ConfirmDialog dlg(this, L"确认软件缓存清理",
        wxString::Format(L"即将清理 %d 个软件的缓存文件，预计释放 %s。\n\n"
                          L"清理后部分软件可能需要重新加载数据，确认继续？",
                          static_cast<int>(selectedPaths.size()),
                          Utils::FormatUtil::FormatFileSize(totalSize)),
        ConfirmDialog::DangerLevel::Caution, L"确认清理", L"取消");

    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    m_softwareCleanButton->Enable(false);
    m_softwareStatusLabel->SetLabelText(L"正在清理...");

    std::thread([this, selectedPaths = std::move(selectedPaths)]() mutable {
        int cleanedCount = 0;
        uint64_t cleanedSize = 0;

        for (const auto& path : selectedPaths) {
            if (IceClean::Utils::FileUtil::Exists(path)) {
                // 递归删除目录内容（保留目录本身）
                WIN32_FIND_DATAW findData;
                std::wstring searchPath = path + L"\\*";
                HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        std::wstring itemName(findData.cFileName);
                        if (itemName == L"." || itemName == L"..") continue;

                        std::wstring itemPath = path + L"\\" + itemName;
                        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            // 删除子目录
                            IceClean::Utils::FileUtil::DeleteFolder(itemPath);
                        } else {
                            // 删除文件
                            IceClean::Utils::FileUtil::DeleteFilePermanently(itemPath);
                        }
                    } while (FindNextFileW(hFind, &findData));
                    FindClose(hFind);
                }
                cleanedCount++;
            }
        }

        CallAfter([this, cleanedCount, cleanedSize]() {
            if (IsBeingDeleted()) return;

            m_softwareStatusLabel->SetLabelText(
                wxString::Format(L"清理完成，已处理 %d 个软件缓存", cleanedCount));
            m_softwareCleanButton->Enable(false);

            // 重新扫描以更新大小
            wxCommandEvent scanEvt(wxEVT_BUTTON, m_softwareScanButton->GetId());
            m_softwareScanButton->GetEventHandler()->AddPendingEvent(scanEvt);
        });
    }).detach();
}

} // namespace IceClean::Gui
