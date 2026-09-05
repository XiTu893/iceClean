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

    // ── 分隔线 ──
    auto* divider = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
    divider->SetBackgroundColour(colors.divider);
    mainSizer->Add(divider, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 16);

    // ── 分类 Tab 页：全部 + 各分类，每页单列软件列表 ──
    m_categoryNotebook = new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        wxNB_TOP | wxBORDER_NONE);
    m_categoryNotebook->SetFont(ThemeManager::GetSmallFont());
    mainSizer->Add(m_categoryNotebook, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

    // ── 状态标签 ──
    m_statusLabel = new wxStaticText(this, wxID_ANY, L"");
    m_statusLabel->SetFont(ThemeManager::GetSmallFont());
    m_statusLabel->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(m_statusLabel, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 8);

    SetSizer(mainSizer);

    // 注册主题变更回调
    ThemeManager::Instance().RegisterChangeCallback([this](const ThemeColors& newColors) {
        SetBackgroundColour(newColors.background);
        m_categoryNotebook->Refresh();
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

    // 离线首启兜底：本地无数据时导入内置精选列表（7-Zip / FileZilla / RustDesk 等）
    db.EnsureSeedLoaded();

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
                    m_statusLabel->SetLabel(L"网络不可用 · 已使用本地列表");
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

    m_categoryNotebook->Freeze();
    m_categoryNotebook->DeleteAllPages();

    // 单个 Tab 页工厂：title + 过滤后的软件列表
    auto addTab = [&](const wxString& title,
                      const std::vector<IceClean::Models::RecommendedSoftware>& items) {
        auto* page = new wxPanel(m_categoryNotebook, wxID_ANY);
        page->SetBackgroundColour(colors.background);

        auto* pageSizer = new wxBoxSizer(wxVERTICAL);
        auto* scroll = new wxScrolledWindow(page, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                            wxVSCROLL | wxBORDER_NONE);
        scroll->SetBackgroundColour(colors.background);
        scroll->SetScrollRate(0, 10);

        auto* listSizer = new wxBoxSizer(wxVERTICAL);
        for (const auto& sw : items) {
            listSizer->Add(CreateSoftwareCard(scroll, sw), 0, wxEXPAND | wxBOTTOM, 10);
        }
        listSizer->AddStretchSpacer();
        scroll->SetSizer(listSizer);
        scroll->FitInside();

        pageSizer->Add(scroll, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
        page->SetSizer(pageSizer);
        m_categoryNotebook->AddPage(page, title, false);
    };

    // 全部
    addTab(L"全部", m_data.software);

    // 各分类
    for (const auto& cat : m_data.categories) {
        std::vector<IceClean::Models::RecommendedSoftware> filtered;
        for (const auto& sw : m_data.software) {
            if (sw.categoryId == cat.id) filtered.push_back(sw);
        }
        addTab(cat.name, filtered);
    }

    m_categoryNotebook->Thaw();

    m_statusLabel->SetLabel(
        wxString::Format(L"共 %d 款软件 · %d 个分类",
                         static_cast<int>(m_data.software.size()),
                         static_cast<int>(m_data.categories.size())));
}

// ── 创建单列行式卡片 ──

wxPanel* SoftwareRecommendPanel::CreateSoftwareCard(
    wxWindow* parent,
    const IceClean::Models::RecommendedSoftware& software) {

    const auto& colors = ThemeManager::Instance().GetColors();

    // 单列行式卡片：横向铺满，[图标 | 名称/描述/元信息 | 操作按钮]
    auto* card = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    card->SetName("card");
    card->SetBackgroundColour(colors.surface);
    card->SetMinSize(wxSize(-1, 76));

    auto* cardSizer = new wxBoxSizer(wxHORIZONTAL);

    // ── 左侧：图标（软件首字符）──
    auto* iconPanel = new wxPanel(card, wxID_ANY, wxDefaultPosition, wxSize(48, 48));
    iconPanel->SetBackgroundColour(colors.accent);
    auto* iconLabel = new wxStaticText(iconPanel, wxID_ANY,
        wxString(software.name).Left(1).Upper());
    iconLabel->SetFont(wxFont(18, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                              wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    iconLabel->SetForegroundColour(*wxWHITE);
    iconLabel->SetBackgroundColour(colors.accent);

    auto* iconVSizer = new wxBoxSizer(wxVERTICAL);
    iconVSizer->AddStretchSpacer();
    iconVSizer->Add(iconLabel, 0, wxALIGN_CENTER_HORIZONTAL);
    iconVSizer->AddStretchSpacer();
    iconPanel->SetSizer(iconVSizer);

    cardSizer->Add(iconPanel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 14);

    // ── 中间：信息区 ──
    auto* infoSizer = new wxBoxSizer(wxVERTICAL);

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
        nameRow->Add(recLabel, 0, wxALIGN_CENTER_VERTICAL);
    }
    if (!software.version.empty()) {
        auto* verLabel = new wxStaticText(card, wxID_ANY,
                                          L"v" + software.version);
        verLabel->SetFont(ThemeManager::GetSmallFont());
        verLabel->SetForegroundColour(colors.textDisabled);
        nameRow->Add(verLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    }
    infoSizer->Add(nameRow, 0, wxBOTTOM, 3);

    // 描述（限宽换行，行式卡片内不无限拉长）
    auto* descLabel = new wxStaticText(card, wxID_ANY, software.description);
    descLabel->SetFont(ThemeManager::GetSmallFont());
    descLabel->SetForegroundColour(colors.textSecondary);
    descLabel->Wrap(560);
    infoSizer->Add(descLabel, 0, wxBOTTOM, 4);

    // 元信息：大小 + 标签
    wxString metaInfo = software.sizeMb > 0
        ? wxString::Format(L"%d MB", software.sizeMb) : wxString(L"");
    for (size_t i = 0; i < software.tags.size() && i < 3; ++i) {
        if (!metaInfo.empty()) metaInfo += L"  ·  ";
        metaInfo += software.tags[i];
    }
    if (!metaInfo.empty()) {
        auto* metaLabel = new wxStaticText(card, wxID_ANY, metaInfo);
        metaLabel->SetFont(ThemeManager::GetSmallFont());
        metaLabel->SetForegroundColour(colors.textDisabled);
        infoSizer->Add(metaLabel, 0);
    }

    cardSizer->Add(infoSizer, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    // ── 右侧：操作按钮 ──
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* visitBtn = new wxButton(card, wxID_ANY, L"官网",
                                   wxDefaultPosition, wxSize(64, 30));
    visitBtn->SetName("btn_visit");
    visitBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                             wxFONTWEIGHT_NORMAL, false, L"微软雅黑"));
    visitBtn->Bind(wxEVT_BUTTON, [software](wxCommandEvent&) {
        if (!software.officialUrl.empty()) {
            wxLaunchDefaultBrowser(software.officialUrl);
        }
    });
    btnSizer->Add(visitBtn, 0, wxRIGHT, 8);

    auto* downloadBtn = new wxButton(card, wxID_ANY, L"下载",
                                      wxDefaultPosition, wxSize(72, 30));
    downloadBtn->SetName("btn_primary_download");
    downloadBtn->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL,
                                wxFONTWEIGHT_BOLD, false, L"微软雅黑"));
    downloadBtn->Bind(wxEVT_BUTTON, [software](wxCommandEvent&) {
        if (!software.downloadUrl.empty()) {
            wxLaunchDefaultBrowser(software.downloadUrl);
        }
    });
    btnSizer->Add(downloadBtn, 0, wxRIGHT, 16);

    cardSizer->Add(btnSizer, 0, wxALIGN_CENTER_VERTICAL);

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

void SoftwareRecommendPanel::OnDownloadClick(wxCommandEvent& /*event*/) {
    // 下载按钮的具体操作在绑定时已处理（打开浏览器）
}

void SoftwareRecommendPanel::OnVisitClick(wxCommandEvent& /*event*/) {
    // 官网按钮的具体操作在绑定时已处理（打开浏览器）
}

} // namespace IceClean::Gui
