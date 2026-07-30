#include "AboutPanel.h"
#include "gui/controls/ThemeManager.h"
#include <wx/image.h>
#include <wx/hyperlink.h>
#include <wx/mstream.h>
#include <windows.h>
#include "resource.h"

namespace IceClean::Gui {

AboutPanel::AboutPanel(wxWindow* parent, wxWindowID id)
    : wxPanel(parent, id)
{
    SetBackgroundColour(ThemeManager::Instance().GetColors().background);
    CreateControls();
}

wxImage AboutPanel::LoadResourceImage(int resourceId) {
    wxImage img;
    HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(resourceId), RT_RCDATA);
    if (hRes) {
        HGLOBAL hData = LoadResource(nullptr, hRes);
        if (hData) {
            DWORD size = SizeofResource(nullptr, hRes);
            void* data = LockResource(hData);
            if (data) {
                wxMemoryInputStream stream(data, size);
                img.LoadFile(stream, wxBITMAP_TYPE_JPEG);
            }
        }
    }
    return img;
}

void AboutPanel::CreateControls() {
    const auto& colors = ThemeManager::Instance().GetColors();
    auto* outerSizer = new wxBoxSizer(wxVERTICAL);

    m_scroller = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      wxVSCROLL | wxBORDER_NONE);
    m_scroller->SetBackgroundColour(colors.background);
    m_scroller->SetScrollRate(0, 10);

    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->AddSpacer(20);

    // ══════════════════════════════════════════
    // 顶部 Hero 区域：Logo + 应用名 + 版本
    // ══════════════════════════════════════════
    auto* heroPanel = new wxPanel(m_scroller, wxID_ANY);
    heroPanel->SetBackgroundColour(colors.surface);
    auto* heroSizer = new wxBoxSizer(wxVERTICAL);
    heroSizer->AddSpacer(10);

    // Logo
    wxImage logoImg = LoadResourceImage(XITU_LOGO);
    if (logoImg.IsOk()) {
        logoImg.Rescale(96, 96, wxIMAGE_QUALITY_HIGH);
        auto* logoBmp = new wxStaticBitmap(heroPanel, wxID_ANY, wxBitmap(logoImg));
        heroSizer->Add(logoBmp, 0, wxALIGN_CENTER_HORIZONTAL);
    }
    heroSizer->AddSpacer(12);

    // 应用名
    auto* nameLabel = new wxStaticText(heroPanel, wxID_ANY, L"IceClean");
    nameLabel->SetFont(wxFont(28, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    nameLabel->SetForegroundColour(colors.accent);
    heroSizer->Add(nameLabel, 0, wxALIGN_CENTER_HORIZONTAL);

    heroSizer->AddSpacer(4);

    // 副标题
    auto* subtitleLabel = new wxStaticText(heroPanel, wxID_ANY,
        L"智能C盘清理与迁移工具");
    subtitleLabel->SetFont(wxFont(13, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
    subtitleLabel->SetForegroundColour(colors.textSecondary);
    heroSizer->Add(subtitleLabel, 0, wxALIGN_CENTER_HORIZONTAL);

    heroSizer->AddSpacer(8);

    // 版本号标签（胶囊样式）
    auto* versionSizer = new wxBoxSizer(wxHORIZONTAL);
    versionSizer->AddStretchSpacer();

    auto* versionPanel = new wxPanel(heroPanel, wxID_ANY);
    versionPanel->SetBackgroundColour(wxColour(
        (colors.accent.Red() + colors.surface.Red()) / 2,
        (colors.accent.Green() + colors.surface.Green()) / 2,
        (colors.accent.Blue() + colors.surface.Blue()) / 2));
    versionPanel->SetMinSize(wxSize(-1, 28));
    auto* versionInnerSizer = new wxBoxSizer(wxHORIZONTAL);
    versionInnerSizer->AddSpacer(12);

    auto* versionDot = new wxStaticText(versionPanel, wxID_ANY, L"\u25CF");
    versionDot->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    versionDot->SetForegroundColour(colors.success);
    versionInnerSizer->Add(versionDot, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);

    auto* versionLabel = new wxStaticText(versionPanel, wxID_ANY, L"v1.0.0 Stable");
    versionLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
    versionLabel->SetForegroundColour(colors.accent);
    versionInnerSizer->Add(versionLabel, 0, wxALIGN_CENTER_VERTICAL);
    versionInnerSizer->AddSpacer(12);

    versionPanel->SetSizer(versionInnerSizer);
    versionSizer->Add(versionPanel, 0);
    versionSizer->AddStretchSpacer();
    heroSizer->Add(versionSizer, 0, wxEXPAND);

    heroSizer->AddSpacer(12);

    // GitHub 链接
    auto* githubLink = new wxHyperlinkCtrl(heroPanel, wxID_ANY,
        L"GitHub: XiTu893/iceClean",
        L"https://github.com/XiTu893/iceClean");
    githubLink->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                false, L"微软雅黑"));
    githubLink->SetNormalColour(colors.accent);
    githubLink->SetHoverColour(colors.accentHover);
    githubLink->SetVisitedColour(colors.accentHover);
    heroSizer->Add(githubLink, 0, wxALIGN_CENTER_HORIZONTAL);

    heroSizer->AddSpacer(16);

    heroPanel->SetSizer(heroSizer);
    mainSizer->Add(heroPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    // ── 分割线 ──
    auto* sep1 = new wxPanel(m_scroller, wxID_ANY);
    sep1->SetBackgroundColour(colors.divider);
    sep1->SetMinSize(wxSize(-1, 1));
    mainSizer->Add(sep1, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(16);

    // ══════════════════════════════════════════
    // 核心功能区块
    // ══════════════════════════════════════════
    auto* featureTitle = new wxStaticText(m_scroller, wxID_ANY, L"核心功能");
    featureTitle->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
    featureTitle->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(featureTitle, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(10);

    // 功能卡片网格（2列）
    auto* featureGrid = new wxGridSizer(2, 2, 10, 10);

    struct FeatureItem {
        wxString icon;
        wxString title;
        wxString desc;
        wxColour color;
    };

    FeatureItem features[] = {
        { L"\U0001F9F9", L"智能扫描",  L"12类扫描器全面覆盖\n系统垃圾、缓存、日志", colors.accent },
        { L"\U0001F5D1", L"一键清理",  L"7类清理器安全释放\n磁盘空间，防止误删", colors.success },
        { L"\U0001F4E6", L"安全迁移",  L"6类迁移器Junction链接\n微信/QQ/Steam等大文件", colors.warning },
        { L"\u26A1",     L"启动加速",  L"启动项/服务/计划任务\n三级进程终止策略", wxColour(0x8B, 0x5C, 0xF6) },
    };

    for (const auto& feat : features) {
        auto* card = new wxPanel(m_scroller, wxID_ANY);
        card->SetBackgroundColour(colors.surface);
        card->SetMinSize(wxSize(200, 90));

        auto* cardSizer = new wxBoxSizer(wxVERTICAL);
        cardSizer->AddSpacer(10);
        cardSizer->AddSpacer(4);

        // 图标 + 标题行
        auto* topRow = new wxBoxSizer(wxHORIZONTAL);
        topRow->AddSpacer(12);

        auto* iconLabel = new wxStaticText(card, wxID_ANY, feat.icon);
        iconLabel->SetFont(wxFont(18, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        topRow->Add(iconLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        auto* titleLabel = new wxStaticText(card, wxID_ANY, feat.title);
        titleLabel->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                    false, L"微软雅黑"));
        titleLabel->SetForegroundColour(feat.color);
        topRow->Add(titleLabel, 0, wxALIGN_CENTER_VERTICAL);

        cardSizer->Add(topRow);

        // 描述
        auto* descLabel = new wxStaticText(card, wxID_ANY, feat.desc);
        descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
        descLabel->SetForegroundColour(colors.textSecondary);
        descLabel->Wrap(220);
        auto* descSizer = new wxBoxSizer(wxHORIZONTAL);
        descSizer->AddSpacer(12);
        descSizer->Add(descLabel, 1, wxEXPAND);
        cardSizer->Add(descSizer, 0, wxEXPAND | wxTOP, 4);
        cardSizer->AddSpacer(8);

        card->SetSizer(cardSizer);
        featureGrid->Add(card, 1, wxEXPAND);
    }

    // 包裹网格在水平居中容器中
    auto* gridWrapper = new wxBoxSizer(wxHORIZONTAL);
    gridWrapper->Add(featureGrid, 1, wxEXPAND);
    mainSizer->Add(gridWrapper, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    mainSizer->AddSpacer(16);

    // ══════════════════════════════════════════
    // 更多特性列表
    // ══════════════════════════════════════════
    auto* moreTitle = new wxStaticText(m_scroller, wxID_ANY, L"更多特性");
    moreTitle->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    moreTitle->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(moreTitle, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    // 特性列表卡片
    auto* moreCard = new wxPanel(m_scroller, wxID_ANY);
    moreCard->SetBackgroundColour(colors.surface);
    auto* moreSizer = new wxBoxSizer(wxVERTICAL);

    struct MoreFeature {
        wxString icon;
        wxString title;
        wxString desc;
    };

    MoreFeature moreFeatures[] = {
        { L"\U0001F6E1", L"三级安全标识",  L"安全/谨慎/危险三级分类，系统关键项不可操作，白名单50+路径保护" },
        { L"\U0001F4CB", L"进程管理",       L"按名称分组显示，安全等级标识，三级递进终止策略（普通→提权→SYSTEM）" },
        { L"\U0001F4CA", L"磁盘分析",       L"可视化空间占用，树状结构浏览，快速定位大文件和大文件夹" },
        { L"\U0001F310", L"浏览器缓存",     L"Chrome/Edge/Firefox/Opera等8大浏览器缓存扫描与清理" },
        { L"\U0001F4BB", L"开发工具缓存",   L"Node.js/npm/pip/conda/Maven/Gradle等14种开发工具缓存清理与迁移" },
        { L"\U0001F504", L"系统还原点",     L"清理/迁移前自动创建系统还原点，支持操作日志回溯" },
        { L"\U0001F4DD", L"注册表修复",     L"扫描无效注册表项，安全修复残留键值，提升系统稳定性" },
        { L"\U0001F512", L"隐私清理",       L"Cookie/浏览历史/表单数据清理，保护个人隐私安全" },
    };

    for (int i = 0; i < _countof(moreFeatures); ++i) {
        const auto& mf = moreFeatures[i];

        auto* row = new wxPanel(moreCard, wxID_ANY);
        row->SetBackgroundColour(colors.surface);
        auto* rowSizer = new wxBoxSizer(wxHORIZONTAL);
        rowSizer->AddSpacer(12);

        // 图标
        auto* iconLabel = new wxStaticText(row, wxID_ANY, mf.icon);
        iconLabel->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
        iconLabel->SetMinSize(wxSize(28, -1));
        rowSizer->Add(iconLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

        // 标题 + 描述
        auto* textSizer = new wxBoxSizer(wxVERTICAL);

        auto* titleLabel = new wxStaticText(row, wxID_ANY, mf.title);
        titleLabel->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                    false, L"微软雅黑"));
        titleLabel->SetForegroundColour(colors.textPrimary);
        textSizer->Add(titleLabel, 0);

        auto* descLabel = new wxStaticText(row, wxID_ANY, mf.desc);
        descLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                   false, L"微软雅黑"));
        descLabel->SetForegroundColour(colors.textSecondary);
        descLabel->Wrap(500);
        textSizer->Add(descLabel, 0, wxTOP, 2);

        rowSizer->Add(textSizer, 1, wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM, 6);
        rowSizer->AddSpacer(12);

        row->SetSizer(rowSizer);
        moreSizer->Add(row, 0, wxEXPAND);

        // 分隔线（非最后一项）
        if (i < _countof(moreFeatures) - 1) {
            auto* rowSep = new wxPanel(moreCard, wxID_ANY);
            rowSep->SetBackgroundColour(colors.divider);
            rowSep->SetMinSize(wxSize(-1, 1));
            auto* sepSizer = new wxBoxSizer(wxHORIZONTAL);
            sepSizer->AddSpacer(48);
            sepSizer->Add(rowSep, 1, wxEXPAND);
            moreSizer->Add(sepSizer, 0, wxEXPAND);
        }
    }

    moreCard->SetSizer(moreSizer);
    mainSizer->Add(moreCard, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    mainSizer->AddSpacer(16);

    // ── 分割线 ──
    auto* sep2 = new wxPanel(m_scroller, wxID_ANY);
    sep2->SetBackgroundColour(colors.divider);
    sep2->SetMinSize(wxSize(-1, 1));
    mainSizer->Add(sep2, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(16);

    // ══════════════════════════════════════════
    // 技术栈信息
    // ══════════════════════════════════════════
    auto* techTitle = new wxStaticText(m_scroller, wxID_ANY, L"技术栈");
    techTitle->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                               false, L"微软雅黑"));
    techTitle->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(techTitle, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(8);

    auto* techCard = new wxPanel(m_scroller, wxID_ANY);
    techCard->SetBackgroundColour(colors.surface);
    auto* techSizer = new wxBoxSizer(wxHORIZONTAL);
    techSizer->AddSpacer(12);
    techSizer->AddSpacer(4);

    // 左列
    auto* leftTechSizer = new wxBoxSizer(wxVERTICAL);
    const wchar_t* techLeft[] = {
        L"C++20",
        L"wxWidgets 3.3",
        L"CMake + vcpkg",
    };
    const wchar_t* techLeftDesc[] = {
        L"现代C++，高效可靠",
        L"跨平台GUI框架",
        L"现代化构建工具链",
    };
    for (int i = 0; i < _countof(techLeft); ++i) {
        auto* techRow = new wxBoxSizer(wxHORIZONTAL);
        auto* dot = new wxStaticText(techCard, wxID_ANY, L"\u25B8");
        dot->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        dot->SetForegroundColour(colors.accent);
        techRow->Add(dot, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        auto* name = new wxStaticText(techCard, wxID_ANY, techLeft[i]);
        name->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                              false, L"微软雅黑"));
        name->SetForegroundColour(colors.textPrimary);
        techRow->Add(name, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        auto* desc = new wxStaticText(techCard, wxID_ANY, techLeftDesc[i]);
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textSecondary);
        techRow->Add(desc, 0, wxALIGN_CENTER_VERTICAL);
        leftTechSizer->Add(techRow, 0, wxTOP | wxBOTTOM, 3);
    }
    techSizer->Add(leftTechSizer, 1, wxEXPAND);

    // 右列
    auto* rightTechSizer = new wxBoxSizer(wxVERTICAL);
    const wchar_t* techRight[] = {
        L"Win32 API",
        L"COM / Task Scheduler",
        L"Junction Point",
    };
    const wchar_t* techRightDesc[] = {
        L"原生Windows系统集成",
        L"计划任务与服务管理",
        L"安全目录链接迁移",
    };
    for (int i = 0; i < _countof(techRight); ++i) {
        auto* techRow = new wxBoxSizer(wxHORIZONTAL);
        auto* dot = new wxStaticText(techCard, wxID_ANY, L"\u25B8");
        dot->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
        dot->SetForegroundColour(colors.accent);
        techRow->Add(dot, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        auto* name = new wxStaticText(techCard, wxID_ANY, techRight[i]);
        name->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                              false, L"微软雅黑"));
        name->SetForegroundColour(colors.textPrimary);
        techRow->Add(name, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
        auto* desc = new wxStaticText(techCard, wxID_ANY, techRightDesc[i]);
        desc->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
        desc->SetForegroundColour(colors.textSecondary);
        techRow->Add(desc, 0, wxALIGN_CENTER_VERTICAL);
        rightTechSizer->Add(techRow, 0, wxTOP | wxBOTTOM, 3);
    }
    techSizer->Add(rightTechSizer, 1, wxEXPAND);
    techSizer->AddSpacer(12);
    techSizer->AddSpacer(4);

    techCard->SetSizer(techSizer);
    mainSizer->Add(techCard, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    mainSizer->AddSpacer(16);

    // ── 分割线 ──
    auto* sep3 = new wxPanel(m_scroller, wxID_ANY);
    sep3->SetBackgroundColour(colors.divider);
    sep3->SetMinSize(wxSize(-1, 1));
    mainSizer->Add(sep3, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(16);

    // ══════════════════════════════════════════
    // 支持开发者（捐赠二维码）
    // ══════════════════════════════════════════
    auto* supportTitle = new wxStaticText(m_scroller, wxID_ANY, L"支持开发者");
    supportTitle->SetFont(wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                  false, L"微软雅黑"));
    supportTitle->SetForegroundColour(colors.textPrimary);
    mainSizer->Add(supportTitle, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(4);

    auto* supportDesc = new wxStaticText(m_scroller, wxID_ANY,
        L"如果您觉得 IceClean 对您有帮助，欢迎请开发者喝杯咖啡");
    supportDesc->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                 false, L"微软雅黑"));
    supportDesc->SetForegroundColour(colors.textSecondary);
    mainSizer->Add(supportDesc, 0, wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(10);

    // 二维码卡片
    auto* qrCard = new wxPanel(m_scroller, wxID_ANY);
    qrCard->SetBackgroundColour(colors.surface);
    auto* qrSizer = new wxBoxSizer(wxHORIZONTAL);
    qrSizer->AddSpacer(20);

    // 二维码图片
    wxImage qrImg = LoadResourceImage(QR_REWARD);
    if (qrImg.IsOk()) {
        qrImg.Rescale(280, 280, wxIMAGE_QUALITY_HIGH);
        auto* qrBmp = new wxStaticBitmap(qrCard, wxID_ANY, wxBitmap(qrImg));
        qrSizer->Add(qrBmp, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 20);
    }

    // 右侧文字
    auto* qrTextSizer = new wxBoxSizer(wxVERTICAL);
    qrTextSizer->AddSpacer(10);

    auto* qrTitle = new wxStaticText(qrCard, wxID_ANY, L"微信扫码捐赠");
    qrTitle->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                              false, L"微软雅黑"));
    qrTitle->SetForegroundColour(colors.textPrimary);
    qrTextSizer->Add(qrTitle, 0);

    auto* qrDesc = new wxStaticText(qrCard, wxID_ANY,
        L"您的支持是持续更新的动力\n感谢每一位使用者");
    qrDesc->SetFont(wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                             false, L"微软雅黑"));
    qrDesc->SetForegroundColour(colors.textSecondary);
    qrTextSizer->Add(qrDesc, 0, wxTOP, 6);

    auto* heartLabel = new wxStaticText(qrCard, wxID_ANY, L"\u2764 \u611F\u8C22\u60A8\u7684\u652F\u6301");
    heartLabel->SetFont(wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                                false, L"微软雅黑"));
    heartLabel->SetForegroundColour(colors.danger);
    qrTextSizer->Add(heartLabel, 0, wxTOP, 10);

    qrSizer->Add(qrTextSizer, 1, wxEXPAND);
    qrSizer->AddSpacer(20);
    qrSizer->AddSpacer(10);

    qrCard->SetSizer(qrSizer);
    mainSizer->Add(qrCard, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);

    mainSizer->AddSpacer(20);

    // ── 分割线 ──
    auto* sep4 = new wxPanel(m_scroller, wxID_ANY);
    sep4->SetBackgroundColour(colors.divider);
    sep4->SetMinSize(wxSize(-1, 1));
    mainSizer->Add(sep4, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(12);

    // ══════════════════════════════════════════
    // 底部版权信息
    // ══════════════════════════════════════════
    auto* footerPanel = new wxPanel(m_scroller, wxID_ANY);
    footerPanel->SetBackgroundColour(colors.background);
    auto* footerSizer = new wxBoxSizer(wxVERTICAL);

    auto* copyrightLabel = new wxStaticText(footerPanel, wxID_ANY,
        L"\u00A9 2024-2026 XiTu. All rights reserved.");
    copyrightLabel->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                                    false, L"微软雅黑"));
    copyrightLabel->SetForegroundColour(colors.textSecondary);
    footerSizer->Add(copyrightLabel, 0, wxALIGN_CENTER_HORIZONTAL);

    auto* madeWith = new wxStaticText(footerPanel, wxID_ANY,
        L"Made with C++20 & wxWidgets on Windows");
    madeWith->SetFont(wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                              false, L"微软雅黑"));
    madeWith->SetForegroundColour(colors.textSecondary);
    footerSizer->Add(madeWith, 0, wxALIGN_CENTER_HORIZONTAL | wxTOP, 2);

    footerPanel->SetSizer(footerSizer);
    mainSizer->Add(footerPanel, 0, wxEXPAND | wxLEFT | wxRIGHT, 20);
    mainSizer->AddSpacer(20);

    m_scroller->SetSizer(mainSizer);
    m_scroller->FitInside();

    outerSizer->Add(m_scroller, 1, wxEXPAND);
    SetSizer(outerSizer);
}

} // namespace IceClean::Gui
