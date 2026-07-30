#include "ThemeManager.h"
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/scrolwin.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <windows.h>
#include <shlobj.h>

namespace IceClean::Gui {

using json = nlohmann::json;

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
    spdlog::info("主题已切换: {}", std::string(GetThemeName(theme).begin(), GetThemeName(theme).end()));
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
    // 侧边栏采用明亮的蓝灰色调，渐变效果清晰可见
    m_colors.background          = wxColour(242, 244, 248);   // #F2F4F8 - 内容区背景
    m_colors.surface             = wxColour(255, 255, 255);   // #FFFFFF - 卡片表面
    m_colors.surfaceHover        = wxColour(240, 242, 246);   // #F0F2F6
    m_colors.textPrimary         = wxColour(29, 33, 41);      // #1D2129
    m_colors.textSecondary       = wxColour(78, 89, 105);     // #4E5969
    m_colors.textDisabled        = wxColour(201, 205, 212);   // #C9CDD4
    m_colors.accent              = wxColour(22, 93, 255);      // #165DFF - Arco Blue
    m_colors.accentHover         = wxColour(53, 118, 255);     // #3576FF
    m_colors.accentGradientEnd   = wxColour(80, 160, 255);    // #50A0FF
    m_colors.danger              = wxColour(245, 63, 63);     // #F53F3F
    m_colors.warning             = wxColour(255, 156, 0);     // #FF9C00
    m_colors.success             = wxColour(0, 180, 42);      // #00B42A
    m_colors.sidebar             = wxColour(55, 80, 140);      // #37508C - 明亮蓝灰（清晰可辨）
    m_colors.sidebarGradientEnd  = wxColour(75, 105, 170);     // #4B69AA - 渐变终止（明显提亮）
    m_colors.sidebarText         = wxColour(220, 228, 242);   // #DCE4F2 - 亮文字
    m_colors.sidebarSelected     = wxColour(22, 93, 255);     // #165DFF
    m_colors.sidebarSelectedText = wxColour(255, 255, 255);   // #FFFFFF
    m_colors.sidebarHover        = wxColour(65, 92, 155);     // #415C9B
    m_colors.border              = wxColour(229, 230, 235);   // #E5E6EB
    m_colors.divider             = wxColour(229, 230, 235);   // #E5E6EB
    m_colors.cardShadow          = wxColour(0, 0, 0, 15);    // rgba(0,0,0,0.06)
    m_colors.cardGradientStart   = wxColour(255, 255, 255);   // #FFFFFF
    m_colors.cardGradientEnd     = wxColour(250, 251, 253);   // #FAFBFD
    m_colors.progressBar         = wxColour(22, 93, 255);     // #165DFF
    m_colors.progressBarBg       = wxColour(229, 230, 235);   // #E5E6EB
}

void ThemeManager::InitDarkTheme() {
    // 参考 Arco Design/Ant Design/QQ 暗色规范
    // 侧边栏使用明显蓝灰色调，与内容区形成层次
    m_colors.background          = wxColour(26, 26, 31);       // #1A1A1F - 深灰蓝
    m_colors.surface             = wxColour(37, 37, 42);       // #25252A - 卡片表面
    m_colors.surfaceHover        = wxColour(46, 46, 53);       // #2E2E35
    m_colors.textPrimary         = wxColour(232, 234, 240);    // #E8EAF0 - 主文字
    m_colors.textSecondary       = wxColour(160, 166, 182);    // #A0A6B6 - 次要文字
    m_colors.textDisabled        = wxColour(92, 93, 110);      // #5C5D6E
    m_colors.accent              = wxColour(61, 127, 255);     // #3D7FFF
    m_colors.accentHover         = wxColour(91, 148, 255);     // #5B94FF
    m_colors.accentGradientEnd   = wxColour(107, 164, 255);    // #6BA4FF
    m_colors.danger              = wxColour(247, 105, 101);    // #F76965
    m_colors.warning             = wxColour(255, 183, 50);     // #FFB732
    m_colors.success             = wxColour(52, 209, 144);     // #34D190
    m_colors.sidebar             = wxColour(32, 42, 72);       // #202A48 - 深蓝灰（明显蓝色调）
    m_colors.sidebarGradientEnd  = wxColour(45, 58, 95);       // #2D3A5F - 渐变终止（更亮）
    m_colors.sidebarText         = wxColour(192, 200, 216);    // #C0C8D8 - 亮文字
    m_colors.sidebarSelected     = wxColour(61, 127, 255);     // #3D7FFF
    m_colors.sidebarSelectedText = wxColour(255, 255, 255);    // #FFFFFF
    m_colors.sidebarHover        = wxColour(40, 52, 85);       // #283455
    m_colors.border              = wxColour(51, 51, 58);       // #33333A
    m_colors.divider             = wxColour(46, 46, 54);       // #2E2E36
    m_colors.cardShadow          = wxColour(0, 0, 0, 64);     // rgba(0,0,0,0.25)
    m_colors.cardGradientStart   = wxColour(37, 37, 42);       // #25252A
    m_colors.cardGradientEnd     = wxColour(42, 42, 48);       // #2A2A30
    m_colors.progressBar         = wxColour(61, 127, 255);     // #3D7FFF
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

// ── 持久化 ──

void ThemeManager::SavePreference() const {
    wchar_t appDataPath[MAX_PATH] = {0};
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) return;

    auto configPath = std::wstring(appDataPath) + L"\\IceClean\\" + kConfigFileName;

    try {
        json j;
        j["theme"] = static_cast<int>(m_currentTheme);

        auto dir = configPath.substr(0, configPath.find_last_of(L'\\'));
        CreateDirectoryW(dir.c_str(), NULL);

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
    wchar_t appDataPath[MAX_PATH] = {0};
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        m_currentTheme = ThemeType::Light;
        return;
    }

    auto configPath = std::wstring(appDataPath) + L"\\IceClean\\" + kConfigFileName;

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
