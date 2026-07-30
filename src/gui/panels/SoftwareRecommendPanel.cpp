#include "SoftwareRecommendPanel.h"
#include "gui/controls/ThemeManager.h"
#include "core/safety/SoftwareRecommendDB.h"
#include "core/safety/SoftwareRecommendFetcher.h"
#include <wx/hyperlink.h>
#include <wx/clipbrd.h>

namespace IceClean::Gui {

wxBEGIN_EVENT_TABLE(SoftwareRecommendPanel, wxPanel)
    EVT_BUTTON(wxID_HIGHEST + 1000, SoftwareRecommendPanel::OnRefreshFromNetwork)
wxEND_EVENT_TABLE()

// ── 构造函数 ──

SoftwareRecommendPanel::SoftwareRecommendPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    const auto& colors = ThemeManager::Instance().GetColors();
    SetBackgroundColour(colors.background);

    CreateControls();
    LoadFromDB();
}

// ── 创建控件 ──

void SoftwareRecommendPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // ── 顶部标题栏 ──
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* titleLabel = new wxStaticText(this, wxID_ANY, L"软件推荐");
    titleLabel->SetFont(ThemeManager::GetTitleFont());
    titleLabel->SetForegroundColour(colors.textPrimary);
    headerSizer->Add(titleLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    m_updateTimeLabel = new wxStaticText(this, wxID_ANY, L"");
    m_updateTimeLabel->SetFont(ThemeManager::GetSmallFont());
    m_updateTimeLabel->SetForegroundColour(colors.textSecondary);
    headerSizer->Add(m_updateTimeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);

    headerSizer->AddStretchSpacer();

    m_refreshButton = new wxButton(this, wxID_HIGHEST + 1000, L"刷新数据",
                                    wxDefaultPosition, wxSize(100, 32));
    m_refreshButton->SetName("btn_primary_refresh");
    m_refreshButton->SetFont(ThemeManager::GetSmallButtonFont());
    headerSizer->Add(m_refreshButton, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(headerSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 24);

    // ── 分类标签栏 ──
    m_categoryBar = new wxPanel(this, wxID_ANY);
    m_categoryBar->SetBackgroundColour(colors.background);
    m_categorySizer = new wxBoxSizer(wxHORIZONTAL);
    m_categoryBar->SetSizer(m_categorySizer);

    mainSizer->Add(m_categoryBar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 16);

    // ── 分隔线 ──
    auto* divider = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    divider->SetBackgroundColour(colors.divider);
    mainSizer->Add(divider, 0, wxEXPAND | wxLEFT | wxRIGHT, 24);

    // ── 内容滚动区域 ──
    m_contentScroller = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                              wxVSCROLL);
    m_contentScroller->SetBackgroundColour(colors.background);
    m_contentScroller->SetScrollRate(0, 10);

    auto* contentSizer = new wxBoxSizer(wxVERTICAL);
    m_contentScroller->SetSizer(contentSizer);

    mainSizer->Add(m_contentScroller, 1, wxEXPAND | wxALL, 0);

    // ── 状态标签 ──
    m_statusLabel = new wxStaticText(this, wxID_ANY, L"");
    m_statusLabel->SetFont(ThemeManager::GetSmallFont());
    m_statusLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(m_statusLabel, 0, wxALIGN_CENTER | wxBOTTOM, 12);

    SetSizer(mainSizer);

    // 注册主题变更回调
    ThemeManager::Instance().RegisterChangeCallback([this](const ThemeColors& newColors) {
        SetBackgroundColour(newColors.background);
        m_categoryBar->SetBackgroundColour(newColors.background);
        m_contentScroller->SetBackgroundColour(newColors.background);
        Refresh();
    });
}

// ── 从数据库加载 ──

void SoftwareRecommendPanel::LoadFromDB() {
    auto& db = IceClean::Core::Safety::SoftwareRecommendDB::Instance();

    if (!db.IsInitialized()) {
        if (!db.Initialize()) {
            m_statusLabel->SetLabel(L"数据库初始化失败");
            return;
        }
    }

    m_data = db.LoadRecommendData();

    if (m_data.categories.empty()) {
        m_statusLabel->SetLabel(L"暂无推荐数据，正在从网络获取...");

        // 首次加载，尝试从网络获取
        auto& fetcher = IceClean::Core::Safety::SoftwareRecommendFetcher::Instance();
        fetcher.FetchAsync([this](bool success, const IceClean::Models::RecommendData& data) {
            CallAfter([this, success, data]() {
                if (success) {
                    auto& db2 = IceClean::Core::Safety::SoftwareRecommendDB::Instance();
                    db2.SaveRecommendData(data);
                    m_data = data;
                    RefreshList();
                    m_statusLabel->SetLabel(L"数据获取成功");
                } else {
                    m_statusLabel->SetLabel(L"网络获取失败，请稍后重试");
                }
            });
        });
        return;
    }

    // 更新时间显示
    auto lastUpdate = db.GetLastUpdateTime();
    if (lastUpdate != std::chrono::system_clock::time_point{}) {
        auto timeT = std::chrono::system_clock::to_time_t(lastUpdate);
        struct tm tmBuf {};
        localtime_s(&tmBuf, &timeT);
        wchar_t timeStr[64] = {};
        wcsftime(timeStr, 64, L"更新于 %Y-%m-%d %H:%M", &tmBuf);
        m_updateTimeLabel->SetLabel(timeStr);
    }

    RefreshList();

    // 检查是否需要自动更新
    if (db.NeedsUpdate()) {
        auto& fetcher = IceClean::Core::Safety::SoftwareRecommendFetcher::Instance();
        fetcher.FetchAsync([this](bool success, const IceClean::Models::RecommendData& data) {
            CallAfter([this, success, data]() {
                if (success) {
                    auto& db2 = IceClean::Core::Safety::SoftwareRecommendDB::Instance();
                    db2.SaveRecommendData(data);
                    m_data = data;
                    RefreshList();
                    m_statusLabel->SetLabel(L"数据已更新");
                }
                // 失败不重试
            });
        });
    }
}

// ── 刷新列表 ──

void SoftwareRecommendPanel::RefreshList() {
    const auto& colors = ThemeManager::Instance().GetColors();

    // 清空分类按钮
    for (auto* btn : m_categoryButtons) {
        m_categorySizer->Detach(btn);
        btn->Destroy();
    }
    m_categoryButtons.clear();

    // 添加"全部"按钮
    auto* allBtn = new wxButton(m_categoryBar, wxID_ANY, L"全部");
    allBtn->SetFont(ThemeManager::GetSmallButtonFont());
    allBtn->SetMinSize(wxSize(60, 30));
    allBtn->SetName("btn_category_all");
    if (m_selectedCategoryIndex == 0) {
        allBtn->SetBackgroundColour(colors.accent);
        allBtn->SetForegroundColour(*wxWHITE);
    } else {
        allBtn->SetBackgroundColour(colors.surface);
        allBtn->SetForegroundColour(colors.textPrimary);
    }
    allBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        m_selectedCategoryIndex = 0;
        RefreshList();
    });
    m_categorySizer->Add(allBtn, 0, wxRIGHT, 6);
    m_categoryButtons.push_back(allBtn);

    // 添加各分类按钮
    for (int i = 0; i < static_cast<int>(m_data.categories.size()); ++i) {
        const auto& cat = m_data.categories[i];
        auto* btn = new wxButton(m_categoryBar, wxID_ANY, cat.name);
        btn->SetFont(ThemeManager::GetSmallButtonFont());
        btn->SetMinSize(wxSize(70, 30));
        btn->SetName("btn_category");

        if (m_selectedCategoryIndex == i + 1) {
            btn->SetBackgroundColour(colors.accent);
            btn->SetForegroundColour(*wxWHITE);
        } else {
            btn->SetBackgroundColour(colors.surface);
            btn->SetForegroundColour(colors.textPrimary);
        }

        btn->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent&) {
            m_selectedCategoryIndex = i + 1;
            RefreshList();
        });

        m_categorySizer->Add(btn, 0, wxRIGHT, 6);
        m_categoryButtons.push_back(btn);
    }

    m_categoryBar->Layout();

    // 清空内容区域
    auto* contentSizer = m_contentScroller->GetSizer();
    contentSizer->Clear(true);

    // 获取要显示的软件
    std::vector<IceClean::Models::RecommendedSoftware> displaySoftware;
    if (m_selectedCategoryIndex == 0) {
        // 全部
        displaySoftware = m_data.software;
    } else {
        int catIdx = m_selectedCategoryIndex - 1;
        if (catIdx < static_cast<int>(m_data.categories.size())) {
            const auto& catId = m_data.categories[catIdx].id;
            for (const auto& sw : m_data.software) {
                if (sw.categoryId == catId) {
                    displaySoftware.push_back(sw);
                }
            }
        }
    }

    // 创建软件卡片网格
    auto* gridSizer = new wxGridSizer(2, 16, 12);

    for (const auto& sw : displaySoftware) {
        auto* card = CreateSoftwareCard(m_contentScroller, sw);
        gridSizer->Add(card, 0, wxEXPAND);
    }

    contentSizer->Add(gridSizer, 0, wxEXPAND | wxALL, 24);
    contentSizer->Layout();
    m_contentScroller->FitInside();

    // 更新状态
    m_statusLabel->SetLabel(
        wxString::Format(L"共 %d 款软件", static_cast<int>(displaySoftware.size())));
}

// ── 创建软件卡片 ──

wxPanel* SoftwareRecommendPanel::CreateSoftwareCard(
    wxWindow* parent,
    const IceClean::Models::RecommendedSoftware& software) {

    const auto& colors = ThemeManager::Instance().GetColors();

    auto* card = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 120));
    card->SetName("card");
    card->SetBackgroundColour(colors.surface);
    card->SetMinSize(wxSize(300, 120));
    card->SetMaxSize(wxSize(500, 120));

    auto* cardSizer = new wxBoxSizer(wxHORIZONTAL);

    // ── 左侧：图标区域 ──
    auto* iconPanel = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(56, 56));
    iconPanel->SetBackgroundColour(colors.accent);
    // 绘制软件首字母作为图标
    auto* iconLabel = new wxStaticText(iconPanel, wxID_ANY,
        software.name.substr(0, 1));
    iconLabel->SetFont(wxFont(20, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    iconLabel->SetForegroundColour(*wxWHITE);
    iconLabel->SetBackgroundColour(colors.accent);

    auto* iconSizer = new wxBoxSizer(wxHORIZONTAL);
    iconSizer->AddStretchSpacer();
    iconSizer->Add(iconLabel, 0, wxALIGN_CENTER);
    iconSizer->AddStretchSpacer();
    auto* iconVSizer = new wxBoxSizer(wxVERTICAL);
    iconVSizer->AddStretchSpacer();
    iconVSizer->Add(iconSizer, 0, wxALIGN_CENTER);
    iconVSizer->AddStretchSpacer();
    iconPanel->SetSizer(iconVSizer);

    cardSizer->Add(iconPanel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    // ── 中间：软件信息 ──
    auto* infoSizer = new wxBoxSizer(wxVERTICAL);

    // 软件名称 + 推荐标签
    auto* nameRow = new wxBoxSizer(wxHORIZONTAL);
    auto* nameLabel = new wxStaticText(card, wxID_ANY, software.name);
    nameLabel->SetFont(ThemeManager::GetSubtitleFont());
    nameLabel->SetForegroundColour(colors.textPrimary);
    nameRow->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    if (software.isRecommended) {
        auto* recLabel = new wxStaticText(card, wxID_ANY, L"推荐");
        recLabel->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                                 wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
        recLabel->SetForegroundColour(*wxWHITE);
        recLabel->SetBackgroundColour(colors.accent);
        recLabel->SetMinSize(wxSize(32, 16));
        nameRow->Add(recLabel, 0, wxALIGN_CENTER_VERTICAL);
    }
    infoSizer->Add(nameRow, 0, wxBOTTOM, 4);

    // 描述
    auto* descLabel = new wxStaticText(card, wxID_ANY, software.description);
    descLabel->SetFont(ThemeManager::GetSmallFont());
    descLabel->SetForegroundColour(colors.textSecondary);
    descLabel->SetMaxSize(wxSize(250, -1));
    infoSizer->Add(descLabel, 0, wxBOTTOM, 4);

    // 大小 + 标签
    wxString sizeInfo = wxString::Format(L"%d MB", software.sizeMb);
    if (!software.tags.empty()) {
        sizeInfo += L"  |  ";
        for (size_t i = 0; i < software.tags.size() && i < 3; ++i) {
            if (i > 0) sizeInfo += L" ";
            sizeInfo += software.tags[i];
        }
    }
    auto* sizeLabel = new wxStaticText(card, wxID_ANY, sizeInfo);
    sizeLabel->SetFont(ThemeManager::GetSmallFont());
    sizeLabel->SetForegroundColour(colors.textDisabled);
    infoSizer->Add(sizeLabel, 0);

    cardSizer->Add(infoSizer, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    // ── 右侧：操作按钮 ──
    auto* btnSizer = new wxBoxSizer(wxVERTICAL);

    auto* downloadBtn = new wxButton(card, wxID_ANY, L"下载",
                                      wxDefaultPosition, wxSize(64, 28));
    downloadBtn->SetName("btn_primary_download");
    downloadBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                                wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    downloadBtn->Bind(wxEVT_BUTTON, [software](wxCommandEvent&) {
        if (!software.downloadUrl.empty()) {
            wxLaunchDefaultBrowser(software.downloadUrl);
        }
    });
    btnSizer->Add(downloadBtn, 0, wxBOTTOM, 4);

    auto* visitBtn = new wxButton(card, wxID_ANY, L"官网",
                                   wxDefaultPosition, wxSize(64, 24));
    visitBtn->SetName("btn_visit");
    visitBtn->SetFont(wxFont(8, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                             wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    visitBtn->Bind(wxEVT_BUTTON, [software](wxCommandEvent&) {
        if (!software.officialUrl.empty()) {
            wxLaunchDefaultBrowser(software.officialUrl);
        }
    });
    btnSizer->Add(visitBtn, 0);

    cardSizer->Add(btnSizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    card->SetSizer(cardSizer);
    return card;
}

// ── 事件处理 ──

void SoftwareRecommendPanel::OnRefreshFromNetwork(wxCommandEvent& /*event*/) {
    m_statusLabel->SetLabel(L"正在获取最新数据...");
    m_refreshButton->Enable(false);

    auto& fetcher = IceClean::Core::Safety::SoftwareRecommendFetcher::Instance();
    fetcher.FetchAsync([this](bool success, const IceClean::Models::RecommendData& data) {
        CallAfter([this, success, data]() {
            m_refreshButton->Enable(true);
            if (success) {
                auto& db = IceClean::Core::Safety::SoftwareRecommendDB::Instance();
                db.SaveRecommendData(data);
                m_data = data;
                RefreshList();
                m_statusLabel->SetLabel(L"数据已更新");

                // 更新时间显示
                auto lastUpdate = db.GetLastUpdateTime();
                if (lastUpdate != std::chrono::system_clock::time_point{}) {
                    auto timeT = std::chrono::system_clock::to_time_t(lastUpdate);
                    struct tm tmBuf {};
                    localtime_s(&tmBuf, &timeT);
                    wchar_t timeStr[64] = {};
                    wcsftime(timeStr, 64, L"更新于 %Y-%m-%d %H:%M", &tmBuf);
                    m_updateTimeLabel->SetLabel(timeStr);
                }
            } else {
                m_statusLabel->SetLabel(L"获取失败，请稍后重试");
            }
        });
    });
}

void SoftwareRecommendPanel::OnCategorySelected(wxCommandEvent& event) {
    m_selectedCategoryIndex = event.GetInt();
    RefreshList();
}

void SoftwareRecommendPanel::OnDownloadClick(wxCommandEvent& /*event*/) {
    // 下载按钮的具体操作在绑定时已处理（打开浏览器）
}

void SoftwareRecommendPanel::OnVisitClick(wxCommandEvent& /*event*/) {
    // 官网按钮的具体操作在绑定时已处理（打开浏览器）
}

} // namespace IceClean::Gui
