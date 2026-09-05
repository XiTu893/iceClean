#include "ThemeManager.h"
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/scrolwin.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>

namespace IceClean::Gui {

using json = nlohmann::json;

namespace {
    std::wstring GetDataDir() {
        wchar_t exePath[MAX_PATH] = {};
        DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        if (len == 0) return L"";
        std::filesystem::path p(exePath);
        std::filesystem::path dataDir = p.parent_path() / L"data";
        std::error_code ec;
        std::filesystem::create_directories(dataDir, ec);
        return dataDir.wstring();
    }
}

// ── 单例 ──

ThemeManager& ThemeManager::Instance() {
    static ThemeManager instance;
    return instance;
}

// ── 初始化 ──

void ThemeManager::Initialize() {
    LoadPreference();
    InitLightTheme();

    if (m_currentTheme == ThemeType::Dark ||
        (m_currentTheme == ThemeType::System && IsSystemDarkMode())) {
        InitDarkTheme();
    }
}

// ── 主题切换 ──

ThemeType ThemeManager::GetTheme() const {
    return m_currentTheme;
}

void ThemeManager::SetTheme(ThemeType theme) {
    m_currentTheme = theme;

    switch (theme) {
        case ThemeType::Light:
            InitLightTheme();
            break;
        case ThemeType::Dark:
            InitDarkTheme();
            break;
        case ThemeType::System:
            if (IsSystemDarkMode()) {
                InitDarkTheme();
            } else {
                InitLightTheme();
            }
            break;
    }

    // 通知回调
    for (const auto& cb : m_callbacks) {
        cb(m_colors);
    }

    SavePreference();
    auto themeName = GetThemeName(theme);
    spdlog::info("主题已切换: {}", std::string(themeName.begin(), themeName.end()));
}

const ThemeColors& ThemeManager::GetColors() const {
    return m_colors;
}

// ── 字体工厂方法 ──

wxFont ThemeManager::GetTitleFont() {
    return wxFont(14, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                  false, L"微软雅黑");
}

wxFont ThemeManager::GetSubtitleFont() {
    return wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                  false, L"微软雅黑");
}

wxFont ThemeManager::GetBodyFont() {
    return wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                  false, L"微软雅黑");
}

wxFont ThemeManager::GetSmallFont() {
    return wxFont(9, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                  false, L"微软雅黑");
}

wxFont ThemeManager::GetButtonFont() {
    return wxFont(11, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD,
                  false, L"微软雅黑");
}

wxFont ThemeManager::GetSmallButtonFont() {
    return wxFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL,
                  false, L"微软雅黑");
}

std::wstring ThemeManager::GetThemeName(ThemeType theme) {
    switch (theme) {
        case ThemeType::Light:  return L"浅色";
        case ThemeType::Dark:   return L"深色";
        case ThemeType::System: return L"跟随系统";
        default:                return L"未知";
    }
}

// ── 回调注册 ──

void ThemeManager::RegisterChangeCallback(ThemeChangeCallback callback) {
    m_callbacks.push_back(std::move(callback));
}

// ── 初始化颜色方案 ──

void ThemeManager::InitLightTheme() {
    // 参考 QQ/Arco Design/Ant Design 商业级浅色主题
    // 侧边栏采用清新的深绿色调，渐变效果清晰可见
    m_colors.background          = wxColour(242, 244, 248);   // #F2F4F8 - 内容区背景
    m_colors.surface             = wxColour(255, 255, 255);   // #FFFFFF - 卡片表面
    m_colors.surfaceHover        = wxColour(240, 242, 246);   // #F0F2F6
    m_colors.textPrimary         = wxColour(29, 33, 41);      // #1D2129
    m_colors.textSecondary       = wxColour(78, 89, 105);     // #4E5969
    m_colors.textDisabled        = wxColour(201, 205, 212);   // #C9CDD4
    m_colors.accent              = wxColour(0, 180, 42);       // #00B42A - Arco Green（清新绿）
    m_colors.accentHover         = wxColour(35, 195, 67);     // #23C343
    m_colors.accentGradientEnd   = wxColour(123, 225, 136);   // #7BE188
    m_colors.danger              = wxColour(245, 63, 63);     // #F53F3F
    m_colors.warning             = wxColour(255, 156, 0);     // #FF9C00
    m_colors.success             = wxColour(0, 200, 83);      // #00C853 - 明亮翠绿（区分主色）
    m_colors.sidebar             = wxColour(24, 138, 76);      // #188A4C - 浓郁深绿（清晰可辨）
    m_colors.sidebarGradientEnd  = wxColour(36, 166, 91);      // #24A65B - 渐变终止（明显提亮）
    m_colors.sidebarText         = wxColour(225, 246, 232);   // #E1F6E8 - 亮文字
    m_colors.sidebarSelected     = wxColour(0, 180, 42);     // #00B42A
    m_colors.sidebarSelectedText = wxColour(255, 255, 255);   // #FFFFFF
    m_colors.sidebarHover        = wxColour(31, 154, 83);     // #1F9A53
    m_colors.border              = wxColour(229, 230, 235);   // #E5E6EB
    m_colors.divider             = wxColour(229, 230, 235);   // #E5E6EB
    m_colors.cardShadow          = wxColour(0, 0, 0, 15);    // rgba(0,0,0,0.06)
    m_colors.cardGradientStart   = wxColour(255, 255, 255);   // #FFFFFF
    m_colors.cardGradientEnd     = wxColour(250, 251, 253);   // #FAFBFD
    m_colors.progressBar         = wxColour(0, 180, 42);     // #00B42A
    m_colors.progressBarBg       = wxColour(229, 230, 235);   // #E5E6EB
}

void ThemeManager::InitDarkTheme() {
    // 参考 Arco Design/Ant Design/QQ 暗色规范
    // 侧边栏使用明显绿灰调，与内容区形成层次
    m_colors.background          = wxColour(26, 26, 31);       // #1A1A1F - 深灰蓝
    m_colors.surface             = wxColour(37, 37, 42);       // #25252A - 卡片表面
    m_colors.surfaceHover        = wxColour(46, 46, 53);       // #2E2E35
    m_colors.textPrimary         = wxColour(232, 234, 240);    // #E8EAF0 - 主文字
    m_colors.textSecondary       = wxColour(160, 166, 182);    // #A0A6B6 - 次要文字
    m_colors.textDisabled        = wxColour(92, 93, 110);      // #5C5D6E
    m_colors.accent              = wxColour(35, 195, 67);     // #23C343
    m_colors.accentHover         = wxColour(76, 210, 99);     // #4CD263
    m_colors.accentGradientEnd   = wxColour(123, 225, 136);   // #7BE188
    m_colors.danger              = wxColour(247, 105, 101);   // #F76965
    m_colors.warning             = wxColour(255, 183, 50);    // #FFB732
    m_colors.success             = wxColour(52, 209, 144);    // #34D190
    m_colors.sidebar             = wxColour(22, 59, 39);      // #163B27 - 深绿（明显绿色调）
    m_colors.sidebarGradientEnd  = wxColour(32, 82, 56);      // #205238 - 渐变终止（更亮）
    m_colors.sidebarText         = wxColour(194, 230, 208);   // #C2E6D0 - 亮文字
    m_colors.sidebarSelected     = wxColour(35, 195, 67);     // #23C343
    m_colors.sidebarSelectedText = wxColour(255, 255, 255);   // #FFFFFF
    m_colors.sidebarHover        = wxColour(27, 69, 47);      // #1B452F
    m_colors.border              = wxColour(51, 51, 58);       // #33333A
    m_colors.divider             = wxColour(46, 46, 54);       // #2E2E36
    m_colors.cardShadow          = wxColour(0, 0, 0, 64);     // rgba(0,0,0,0.25)
    m_colors.cardGradientStart   = wxColour(37, 37, 42);       // #25252A
    m_colors.cardGradientEnd     = wxColour(42, 42, 48);       // #2A2A30
    m_colors.progressBar         = wxColour(35, 195, 67);     // #23C343
    m_colors.progressBarBg       = wxColour(51, 51, 58);       // #33333A
}

// ── 系统暗色检测 ──

bool ThemeManager::IsSystemDarkMode() const {
    // 检查 Windows 系统暗色模式设置
    // 注册表路径: HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize
    // AppsUseLightTheme = 0 表示暗色模式
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {

        DWORD value = 1;
        DWORD size = sizeof(value);
        if (RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL,
            reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return value == 0; // 0 = 暗色模式
        }
        RegCloseKey(hKey);
    }
    return false;
}

// ── 应用主题到窗口 ──

void ThemeManager::ApplyTheme(wxWindow* window) const {
    if (!window) return;

    window->SetBackgroundColour(m_colors.background);

    // 递归应用到子控件
    for (auto* child : window->GetChildren()) {
        auto name = child->GetName().ToStdWstring();

        // 面板
        auto* panel = dynamic_cast<wxPanel*>(child);
        if (panel) {
            if (name.find(L"NavSidebar") != std::wstring::npos) {
                panel->SetBackgroundColour(m_colors.sidebar);
            } else if (name.find(L"card") != std::wstring::npos ||
                       name.find(L"Card") != std::wstring::npos) {
                panel->SetBackgroundColour(m_colors.surface);
            } else {
                panel->SetBackgroundColour(m_colors.background);
            }
        }

        // 静态文本
        auto* text = dynamic_cast<wxStaticText*>(child);
        if (text) {
            text->SetForegroundColour(m_colors.textPrimary);
        }

        // 按钮 — 按名称前缀区分语义
        auto* btn = dynamic_cast<wxButton*>(child);
        if (btn) {
            if (name.find(L"btn_primary") != std::wstring::npos ||
                name.find(L"btnPrimary") != std::wstring::npos) {
                btn->SetBackgroundColour(m_colors.accent);
                btn->SetForegroundColour(*wxWHITE);
            } else if (name.find(L"btn_danger") != std::wstring::npos ||
                       name.find(L"btnDanger") != std::wstring::npos) {
                btn->SetBackgroundColour(m_colors.danger);
                btn->SetForegroundColour(*wxWHITE);
            } else if (name.find(L"btn_success") != std::wstring::npos ||
                       name.find(L"btnSuccess") != std::wstring::npos) {
                btn->SetBackgroundColour(m_colors.success);
                btn->SetForegroundColour(*wxWHITE);
            } else {
                btn->SetBackgroundColour(m_colors.surface);
                btn->SetForegroundColour(m_colors.textPrimary);
            }
        }

        // 列表控件
        auto* listCtrl = dynamic_cast<wxListCtrl*>(child);
        if (listCtrl) {
            listCtrl->SetBackgroundColour(m_colors.surface);
            listCtrl->SetForegroundColour(m_colors.textPrimary);
        }

        // 复选框
        auto* checkBox = dynamic_cast<wxCheckBox*>(child);
        if (checkBox) {
            checkBox->SetForegroundColour(m_colors.textPrimary);
            checkBox->SetBackgroundColour(m_colors.background);
        }

        // 下拉选择框
        auto* choice = dynamic_cast<wxChoice*>(child);
        if (choice) {
            choice->SetBackgroundColour(m_colors.surface);
            choice->SetForegroundColour(m_colors.textPrimary);
        }

        // 文本输入框
        auto* textCtrl = dynamic_cast<wxTextCtrl*>(child);
        if (textCtrl) {
            textCtrl->SetBackgroundColour(m_colors.surface);
            textCtrl->SetForegroundColour(m_colors.textPrimary);
        }

        // 滚动窗口
        auto* scrolled = dynamic_cast<wxScrolledWindow*>(child);
        if (scrolled) {
            scrolled->SetBackgroundColour(m_colors.background);
        }

        // Notebook
        auto* notebook = dynamic_cast<wxNotebook*>(child);
        if (notebook) {
            notebook->SetBackgroundColour(m_colors.background);
            notebook->SetForegroundColour(m_colors.textPrimary);
        }

        // 递归
        ApplyTheme(child);
    }

    window->Refresh();
}

// ── 按钮悬停辅助 ──

// 提升亮度（用于深色背景的悬停态：轻微提亮让按钮"浮"起来）
static wxColour Lighten(const wxColour& c, int delta) {
    auto clamp = [](int v) { return v > 255 ? 255 : (v < 0 ? 0 : v); };
    return wxColour(clamp(c.Red() + delta), clamp(c.Green() + delta), clamp(c.Blue() + delta));
}

// 降低亮度（用于浅色背景的悬停态：轻微变暗让按钮"下沉"一点）
static wxColour Darken(const wxColour& c, int delta) {
    auto clamp = [](int v) { return v > 255 ? 255 : (v < 0 ? 0 : v); };
    return wxColour(clamp(c.Red() - delta), clamp(c.Green() - delta), clamp(c.Blue() - delta));
}

// 自动计算微调后的悬停色：根据背景亮度决定加深还是提亮，但变化幅度很小
// 浅色 → 略微变深（< 10% 亮度），深色 → 略微提亮
// 目的：仅提示"可点击"，不改变整体观感
static wxColour AutoHoverColor(const wxColour& base) {
    double luminance = 0.299 * base.Red() + 0.587 * base.Green() + 0.114 * base.Blue();
    if (luminance > 128.0) {
        // 浅色背景：轻微变暗，delta=12（约 5% 亮度变化）
        return Darken(base, 12);
    }
    // 深色背景：轻微提亮，delta=15
    return Lighten(base, 15);
}

wxColour ThemeManager::ApplyButtonHover(wxButton* btn, const wxColour& normalBg, const wxColour& hoverBg) {
    if (!btn) return wxColour();
    const wxColour effectiveHover = hoverBg.IsOk() ? hoverBg : AutoHoverColor(normalBg);
    const wxColour normal = normalBg;
    const wxColour hover = effectiveHover;

    btn->Bind(wxEVT_ENTER_WINDOW, [btn, normal, hover](wxMouseEvent&) {
        if (!btn->IsEnabled()) return;
        btn->SetBackgroundColour(hover);
        btn->Refresh();
    });
    btn->Bind(wxEVT_LEAVE_WINDOW, [btn, normal](wxMouseEvent&) {
        if (!btn->IsEnabled()) return;
        btn->SetBackgroundColour(normal);
        btn->Refresh();
    });
    // 按钮被禁用时：背景色由 ApplyTheme / SetBackgroundColour 统一控制，不再重复设置

    return effectiveHover;
}

// ── 持久化 ──

void ThemeManager::SavePreference() const {
    std::wstring dataDir = GetDataDir();
    if (dataDir.empty()) return;

    auto configPath = std::filesystem::path(dataDir) / L"config" / kConfigFileName;

    try {
        std::error_code ec;
        std::filesystem::create_directories(configPath.parent_path(), ec);

        json j;
        j["theme"] = static_cast<int>(m_currentTheme);

        std::ofstream file(configPath);
        if (file.is_open()) {
            file << j.dump(2);
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("保存主题偏好失败: {}", e.what());
    }
}

void ThemeManager::LoadPreference() {
    std::wstring dataDir = GetDataDir();
    if (dataDir.empty()) {
        m_currentTheme = ThemeType::Light;
        return;
    }

    auto configPath = std::filesystem::path(dataDir) / L"config" / kConfigFileName;

    try {
        std::ifstream file(configPath);
        if (!file.is_open()) {
            m_currentTheme = ThemeType::Light;
            return;
        }

        json j;
        file >> j;

        if (j.contains("theme")) {
            m_currentTheme = static_cast<ThemeType>(j["theme"].get<int>());
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("加载主题偏好失败: {}", e.what());
        m_currentTheme = ThemeType::Light;
    }
}

} // namespace IceClean::Gui
