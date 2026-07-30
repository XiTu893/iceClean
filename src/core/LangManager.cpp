#include "LangManager.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <windows.h>
#include <shlobj.h>

namespace IceClean::Core {

using json = nlohmann::json;

// ── 单例 ──

LangManager& LangManager::Instance() {
    static LangManager instance;
    return instance;
}

// ── 初始化 ──

void LangManager::Initialize() {
    // 扫描 lang/ 目录获取可用语言
    m_availableLanguages.clear();

    auto langDir = GetLangDirectory();
    if (!std::filesystem::exists(langDir)) {
        // 创建默认语言目录
        std::filesystem::create_directories(langDir);
    }

    for (const auto& entry : std::filesystem::directory_iterator(langDir)) {
        if (entry.is_regular_file() && entry.path().extension() == L".json") {
            auto stem = entry.path().stem().wstring();
            m_availableLanguages.push_back(stem);
        }
    }

    // 确保至少有默认语言
    if (m_availableLanguages.empty()) {
        m_availableLanguages.push_back(kDefaultLang);
    }

    // 加载语言偏好
    LoadLanguagePreference();

    // 加载当前语言
    if (!m_currentLang.empty()) {
        LoadTranslationFile(m_currentLang);
    } else {
        SetLanguage(kDefaultLang);
    }
}

// ── 语言切换 ──

const std::wstring& LangManager::GetCurrentLanguage() const {
    return m_currentLang;
}

bool LangManager::SetLanguage(const std::wstring& langCode) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_currentLang == langCode && !m_translations.empty()) {
        return true; // 已经是当前语言
    }

    if (LoadTranslationFile(langCode)) {
        m_currentLang = langCode;
        spdlog::info("语言已切换: {}", std::string(langCode.begin(), langCode.end()));

        // 通知回调
        for (const auto& cb : m_callbacks) {
            cb(langCode);
        }

        return true;
    }

    return false;
}

std::vector<std::wstring> LangManager::GetAvailableLanguages() const {
    return m_availableLanguages;
}

// ── 翻译获取 ──

std::wstring LangManager::Get(const std::wstring& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_translations.find(key);
    if (it != m_translations.end()) {
        return it->second;
    }

    // 未找到翻译，返回 key 本身
    return key;
}

std::wstring LangManager::Get(const std::wstring& key,
                               const std::vector<std::wstring>& args) const {
    auto text = Get(key);

    // 替换 {0}, {1}, {2} ... 占位符
    for (size_t i = 0; i < args.size(); ++i) {
        std::wstring placeholder = L"{" + std::to_wstring(i) + L"}";
        size_t pos = 0;
        while ((pos = text.find(placeholder, pos)) != std::wstring::npos) {
            text.replace(pos, placeholder.length(), args[i]);
            pos += args[i].length();
        }
    }

    return text;
}

bool LangManager::HasKey(const std::wstring& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_translations.find(key) != m_translations.end();
}

// ── 回调注册 ──

void LangManager::RegisterChangeCallback(LanguageChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.push_back(std::move(callback));
}

// ── 翻译文件加载 ──

std::wstring LangManager::GetLangDirectory() {
    wchar_t exePath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::filesystem::path p(exePath);
    return (p.parent_path() / kLangDir).wstring();
}

bool LangManager::LoadTranslationFile(const std::wstring& langCode) {
    auto filePath = GetLangDirectory() + L"\\" + langCode + L".json";

    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            spdlog::warn("翻译文件不存在: {}", std::string(filePath.begin(), filePath.end()));
            return false;
        }

        json j;
        file >> j;

        m_translations.clear();
        FlattenJson("", &j, m_translations);

        spdlog::info("已加载 {} 条翻译 (语言: {})", m_translations.size(),
                     std::string(langCode.begin(), langCode.end()));
        return true;
    }
    catch (const std::exception& e) {
        spdlog::error("加载翻译文件失败: {}", e.what());
        return false;
    }
}

void LangManager::FlattenJson(const std::string& prefix, const void* jsonObj,
                               std::unordered_map<std::wstring, std::wstring>& output) {
    const auto* j = static_cast<const json*>(jsonObj);

    if (!j->is_object()) return;

    for (auto it = j->begin(); it != j->end(); ++it) {
        std::string fullKey = prefix.empty() ? it.key() : prefix + "." + it.key();

        if (it.value().is_string()) {
            auto val = it.value().get<std::string>();
            output[std::wstring(fullKey.begin(), fullKey.end())] =
                std::wstring(val.begin(), val.end());
        } else if (it.value().is_object()) {
            FlattenJson(fullKey, &it.value(), output);
        }
    }
}

// ── 语言偏好持久化 ──

void LangManager::SaveLanguagePreference() const {
    wchar_t appDataPath[MAX_PATH] = {0};
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) return;

    auto configPath = std::wstring(appDataPath) + L"\\IceClean\\lang_config.json";

    try {
        json j;
        j["language"] = std::string(m_currentLang.begin(), m_currentLang.end());

        // 确保目录存在
        auto dir = configPath.substr(0, configPath.find_last_of(L'\\'));
        CreateDirectoryW(dir.c_str(), NULL);

        std::ofstream file(configPath);
        if (file.is_open()) {
            file << j.dump(2);
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("保存语言偏好失败: {}", e.what());
    }
}

void LangManager::LoadLanguagePreference() {
    wchar_t appDataPath[MAX_PATH] = {0};
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        m_currentLang = kDefaultLang;
        return;
    }

    auto configPath = std::wstring(appDataPath) + L"\\IceClean\\lang_config.json";

    try {
        std::ifstream file(configPath);
        if (!file.is_open()) {
            m_currentLang = kDefaultLang;
            return;
        }

        json j;
        file >> j;

        if (j.contains("language")) {
            auto lang = j["language"].get<std::string>();
            m_currentLang = std::wstring(lang.begin(), lang.end());
        } else {
            m_currentLang = kDefaultLang;
        }
    }
    catch (const std::exception& e) {
        spdlog::warn("加载语言偏好失败: {}", e.what());
        m_currentLang = kDefaultLang;
    }
}

} // namespace IceClean::Core
