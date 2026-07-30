#pragma once
#include <string>
#include <unordered_map>
#include <wx/wx.h>
#include <wx/colour.h>

namespace IceClean::Gui {

// 主题类型
enum class ThemeType {
    Light,      // 浅色主题（默认）
    Dark,       // 深色主题
    System      // 跟随系统
};

// 主题颜色定义
struct ThemeColors {
    wxColour background;            // 主背景色
    wxColour surface;               // 卡片/面板表面色
    wxColour surfaceHover;          // 悬停表面色
    wxColour textPrimary;           // 主要文字颜色
    wxColour textSecondary;         // 次要文字颜色
    wxColour textDisabled;          // 禁用文字颜色
    wxColour accent;                // 强调色（主蓝色）
    wxColour accentHover;           // 强调色悬停
    wxColour accentGradientEnd;     // 品牌色渐变终止色
    wxColour danger;                // 危险色（红色）
    wxColour warning;               // 警告色（黄色）
    wxColour success;               // 成功色（绿色）
    wxColour sidebar;               // 侧边栏背景
    wxColour sidebarGradientEnd;    // 侧边栏渐变终止色
    wxColour sidebarText;           // 侧边栏文字
    wxColour sidebarSelected;       // 侧边栏选中
    wxColour sidebarSelectedText;   // 侧边栏选中文字
    wxColour sidebarHover;          // 侧边栏悬停
    wxColour border;                // 边框颜色
    wxColour divider;               // 分割线颜色
    wxColour cardShadow;            // 卡片阴影
    wxColour cardGradientStart;     // 卡片渐变起始色
    wxColour cardGradientEnd;       // 卡片渐变终止色
    wxColour progressBar;           // 进度条颜色
    wxColour progressBarBg;         // 进度条背景
};

// 主题管理器
// 管理应用的浅色/深色主题切换
class ThemeManager {
public:
    // 获取单例
    static ThemeManager& Instance();

    // 初始化
    void Initialize();

    // 获取/设置主题
    ThemeType GetTheme() const;
    void SetTheme(ThemeType theme);

    // 获取当前主题颜色
    const ThemeColors& GetColors() const;

    // 字体工厂方法（统一字体规范）
    static wxFont GetTitleFont();       // 14号粗体 - 页面标题
    static wxFont GetSubtitleFont();    // 12号粗体 - 区域标题
    static wxFont GetBodyFont();        // 10号常规 - 正文/标签
    static wxFont GetSmallFont();       // 9号常规 - 描述/辅助文字
    static wxFont GetButtonFont();      // 11号粗体 - 主按钮
    static wxFont GetSmallButtonFont(); // 10号常规 - 小按钮

    // 按钮尺寸常量
    struct ButtonSize {
        static constexpr int PrimaryW = 200;   // 主操作按钮宽度
        static constexpr int PrimaryH = 44;    // 主操作按钮高度
        static constexpr int ActionW = 120;    // 操作按钮宽度
        static constexpr int ActionH = 36;     // 操作按钮高度
        static constexpr int SmallW = 80;      // 小按钮宽度
        static constexpr int SmallH = 28;      // 小按钮高度
    };

    // 获取主题名称
    static std::wstring GetThemeName(ThemeType theme);

    // 保存/加载主题偏好
    void SavePreference() const;
    void LoadPreference();

    // 注册主题变更回调
    using ThemeChangeCallback = std::function<void(const ThemeColors& newColors)>;
    void RegisterChangeCallback(ThemeChangeCallback callback);

    // 应用主题到窗口及其子控件
    void ApplyTheme(wxWindow* window) const;

private:
    ThemeManager() = default;
    ~ThemeManager() = default;

    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    // 初始化颜色方案
    void InitLightTheme();
    void InitDarkTheme();

    // 根据系统设置检测是否应使用暗色主题
    bool IsSystemDarkMode() const;

    ThemeType m_currentTheme = ThemeType::Light;
    ThemeColors m_colors;
    std::vector<ThemeChangeCallback> m_callbacks;

    static constexpr const wchar_t* kConfigFileName = L"theme_config.json";
};

} // namespace IceClean::Gui
