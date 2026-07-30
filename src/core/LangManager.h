#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>

namespace IceClean::Core {

// 多语言管理器
// 基于 JSON 翻译文件的轻量级国际化框架
// 翻译文件存放于 lang/ 目录下，格式如 lang/zh-CN.json, lang/en-US.json
class LangManager {
public:
    // 获取单例
    static LangManager& Instance();

    // 初始化语言管理器
    void Initialize();

    // 获取当前语言
    const std::wstring& GetCurrentLanguage() const;

    // 设置语言（如 "zh-CN", "en-US"）
    bool SetLanguage(const std::wstring& langCode);

    // 获取可用语言列表
    std::vector<std::wstring> GetAvailableLanguages() const;

    // 获取翻译文本
    // key 格式: "section.key" 如 "menu.file", "dialog.confirm_delete"
    std::wstring Get(const std::wstring& key) const;

    // 获取翻译文本（带参数替换）
    // 参数使用 {0}, {1}, {2} 等占位符
    std::wstring Get(const std::wstring& key,
                     const std::vector<std::wstring>& args) const;

    // 检查 key 是否存在翻译
    bool HasKey(const std::wstring& key) const;

    // 注册语言变更回调
    using LanguageChangeCallback = std::function<void(const std::wstring& newLang)>;
    void RegisterChangeCallback(LanguageChangeCallback callback);

    // 获取语言目录路径
    static std::wstring GetLangDirectory();

    // 保存语言偏好
    void SaveLanguagePreference() const;

    // 加载语言偏好
    void LoadLanguagePreference();

private:
    LangManager() = default;
    ~LangManager() = default;

    LangManager(const LangManager&) = delete;
    LangManager& operator=(const LangManager&) = delete;

    // 加载翻译文件
    bool LoadTranslationFile(const std::wstring& langCode);

    // 扁平化 JSON 键（嵌套对象 → "a.b.c" 格式）
    void FlattenJson(const std::string& prefix, const void* jsonObj,
                     std::unordered_map<std::wstring, std::wstring>& output);

    std::wstring m_currentLang;
    std::unordered_map<std::wstring, std::wstring> m_translations; // key → 翻译文本
    std::vector<LanguageChangeCallback> m_callbacks;
    std::vector<std::wstring> m_availableLanguages;
    mutable std::mutex m_mutex;

    static constexpr const wchar_t* kDefaultLang = L"zh-CN";
    static constexpr const wchar_t* kLangDir = L"lang";
};

} // namespace IceClean::Core

// 便捷宏：获取翻译文本
#define TR(key) IceClean::Core::LangManager::Instance().Get(L##key)
#define TR_ARGS(key, ...) IceClean::Core::LangManager::Instance().Get(L##key, {__VA_ARGS__})
