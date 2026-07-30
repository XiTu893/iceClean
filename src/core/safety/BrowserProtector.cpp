#include "BrowserProtector.h"
#include "utils/RegistryUtil.h"
#include "utils/Win32Util.h"

#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

namespace IceClean::Core::Safety {

using namespace IceClean::Utils;

// ── 已知劫持主页/搜索引擎 ──
const std::vector<std::wstring> BrowserProtector::s_hijackedHomePages = {
    L"2345.com", L"hao123.com", L"haosou.com", L"tao123.com",
    L"7654.com", L"9991.com", L"qq161.com", L"dh9.com",
    L"navi.jnwjd.com", L"delta-homes.com", L"qvo6.com",
    L"istartsurf.com", L"search.conduit.com", L"v9.com",
    L"myway.com", L"mindspark.com", L"ask.com",
    L"baidu.com/home", L"so.com/home",
};

const std::vector<std::wstring> BrowserProtector::s_hijackedSearchEngines = {
    L"2345.com", L"haosou.com", L"so.com", L"baidu.com/search",
    L"delta-homes.com", L"qvo6.com", L"istartsurf.com",
    L"search.conduit.com", L"v9.com", L"myway.com",
};

// ── 扫描所有浏览器 ──

std::vector<Models::BrowserProtectionItem> BrowserProtector::ScanBrowsers() {
    std::vector<Models::BrowserProtectionItem> items;

    ScanChrome(items);
    ScanEdge(items);
    ScanFirefox(items);
    ScanIE(items);

    return items;
}

// ── 扫描 Chrome ──

void BrowserProtector::ScanChrome(std::vector<Models::BrowserProtectionItem>& items) {
    std::wstring prefsPath = Win32Util::ExpandEnvVars(
        L"%LOCALAPPDATA%\\Google\\Chrome\\User Data\\Default\\Preferences");

    if (GetFileAttributesW(prefsPath.c_str()) == INVALID_FILE_ATTRIBUTES) return;

    Models::BrowserProtectionItem item;
    item.browser = Models::BrowserType::Chrome;
    item.browserName = L"Google Chrome";
    item.installPath = prefsPath;

    std::wstring homePage, searchEngine;
    if (ReadChromiumBrowserSettings(prefsPath, homePage, searchEngine)) {
        item.homePage = homePage;
        item.searchEngine = searchEngine;
        item.defaultHomePage = L"chrome://newtab";
        item.defaultSearchEngine = L"Google";
        item.isHomePageHijacked = IsHomePageHijacked(homePage);
        item.isSearchEngineHijacked = IsSearchEngineHijacked(searchEngine);
    }

    items.push_back(item);
}

// ── 扫描 Edge ──

void BrowserProtector::ScanEdge(std::vector<Models::BrowserProtectionItem>& items) {
    std::wstring prefsPath = Win32Util::ExpandEnvVars(
        L"%LOCALAPPDATA%\\Microsoft\\Edge\\User Data\\Default\\Preferences");

    if (GetFileAttributesW(prefsPath.c_str()) == INVALID_FILE_ATTRIBUTES) return;

    Models::BrowserProtectionItem item;
    item.browser = Models::BrowserType::Edge;
    item.browserName = L"Microsoft Edge";
    item.installPath = prefsPath;

    std::wstring homePage, searchEngine;
    if (ReadChromiumBrowserSettings(prefsPath, homePage, searchEngine)) {
        item.homePage = homePage;
        item.searchEngine = searchEngine;
        item.defaultHomePage = L"edge://newtab";
        item.defaultSearchEngine = L"Bing";
        item.isHomePageHijacked = IsHomePageHijacked(homePage);
        item.isSearchEngineHijacked = IsSearchEngineHijacked(searchEngine);
    }

    items.push_back(item);
}

// ── 扫描 Firefox ──

void BrowserProtector::ScanFirefox(std::vector<Models::BrowserProtectionItem>& items) {
    std::wstring profilePath = Win32Util::ExpandEnvVars(
        L"%APPDATA%\\Mozilla\\Firefox\\Profiles");

    if (GetFileAttributesW(profilePath.c_str()) == INVALID_FILE_ATTRIBUTES) return;

    // 读取 profiles.ini 找到默认配置
    std::wstring iniPath = Win32Util::ExpandEnvVars(
        L"%APPDATA%\\Mozilla\\Firefox\\profiles.ini");

    Models::BrowserProtectionItem item;
    item.browser = Models::BrowserType::Firefox;
    item.browserName = L"Mozilla Firefox";
    item.defaultHomePage = L"about:home";
    item.defaultSearchEngine = L"Google";

    // 简化：读取 prefs.js 中的主页设置
    std::wstring prefsJsPath = Win32Util::ExpandEnvVars(
        L"%APPDATA%\\Mozilla\\Firefox\\Profiles");

    // 查找 .default-release 目录
    WIN32_FIND_DATAW findData;
    std::wstring searchPath = prefsJsPath + L"\\*default*";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                std::wstring dirName(findData.cFileName);
                if (dirName.find(L"default") != std::wstring::npos) {
                    std::wstring fullPath = prefsJsPath + L"\\" + dirName + L"\\prefs.js";
                    if (GetFileAttributesW(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        item.installPath = fullPath;

                        // 简化解析 - 读取 browser.startup.homepage
                        std::ifstream ifs(fullPath);
                        if (ifs.is_open()) {
                            std::string line;
                            while (std::getline(ifs, line)) {
                                if (line.find("browser.startup.homepage") != std::string::npos) {
                                    // 提取引号中的 URL
                                    size_t start = line.find("\"", line.find("="));
                                    size_t end = line.rfind("\"");
                                    if (start != std::string::npos && end > start) {
                                        std::string url = line.substr(start + 1, end - start - 1);
                                        item.homePage = std::wstring(url.begin(), url.end());
                                    }
                                }
                                if (line.find("browser.search.defaultenginename") != std::string::npos) {
                                    size_t start = line.find("\"", line.find("="));
                                    size_t end = line.rfind("\"");
                                    if (start != std::string::npos && end > start) {
                                        std::string engine = line.substr(start + 1, end - start - 1);
                                        item.searchEngine = std::wstring(engine.begin(), engine.end());
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    item.isHomePageHijacked = IsHomePageHijacked(item.homePage);
    item.isSearchEngineHijacked = IsSearchEngineHijacked(item.searchEngine);

    items.push_back(item);
}

// ── 扫描 IE ──

void BrowserProtector::ScanIE(std::vector<Models::BrowserProtectionItem>& items) {
    Models::BrowserProtectionItem item;
    item.browser = Models::BrowserType::IE;
    item.browserName = L"Internet Explorer";
    item.homePage = GetIEHomePage();
    item.defaultHomePage = L"about:tabs";
    item.searchEngine = L"Bing";
    item.defaultSearchEngine = L"Bing";
    item.isHomePageHijacked = IsHomePageHijacked(item.homePage);
    item.isSearchEngineHijacked = false;

    items.push_back(item);
}

// ── 读取 Chromium 系浏览器设置 ──

bool BrowserProtector::ReadChromiumBrowserSettings(const std::wstring& prefsPath,
                                                      std::wstring& homePage,
                                                      std::wstring& searchEngine) {
    std::ifstream ifs(prefsPath);
    if (!ifs.is_open()) return false;

    try {
        nlohmann::json j;
        ifs >> j;

        // 读取主页
        if (j.contains("homepage")) {
            homePage = j["homepage"].get<std::wstring>();
        } else if (j.contains("session") && j["session"].contains("restore_on_startup")) {
            int restoreOnStartup = j["session"]["restore_on_startup"].get<int>();
            if (restoreOnStartup == 5) {
                homePage = L"chrome://newtab";
            } else if (restoreOnStartup == 4) {
                // 自定义URL
                if (j.contains("session") && j["session"].contains("startup_urls")) {
                    auto urls = j["session"]["startup_urls"];
                    if (!urls.empty()) {
                        homePage = urls[0].get<std::wstring>();
                    }
                }
            }
        }

        if (homePage.empty()) {
            homePage = L"chrome://newtab";
        }

        // 读取搜索引擎
        if (j.contains("default_search_provider") &&
            j["default_search_provider"].contains("name")) {
            searchEngine = j["default_search_provider"]["name"].get<std::wstring>();
        }

        if (searchEngine.empty()) {
            searchEngine = L"Google";
        }

        return true;
    } catch (...) {
        return false;
    }
}

// ── 锁定主页 ──

bool BrowserProtector::LockHomePage(const Models::BrowserProtectionItem& item,
                                      const std::wstring& homePage) {
    switch (item.browser) {
    case Models::BrowserType::Chrome:
    case Models::BrowserType::Edge:
        return SetChromiumHomePage(item.installPath, homePage);

    case Models::BrowserType::IE:
        return SetIEHomePage(homePage);

    case Models::BrowserType::Firefox:
        // Firefox 需要修改 prefs.js，这里简化处理
        return false;

    default:
        return false;
    }
}

bool BrowserProtector::UnlockHomePage(const Models::BrowserProtectionItem& item) {
    // 解锁 = 恢复默认
    return RestoreDefaultHomePage(item);
}

bool BrowserProtector::LockSearchEngine(const Models::BrowserProtectionItem& item,
                                          const std::wstring& searchEngine) {
    // 搜索引擎锁定主要通过注册策略实现（简化处理）
    return false;
}

bool BrowserProtector::UnlockSearchEngine(const Models::BrowserProtectionItem& item) {
    return RestoreDefaultSearchEngine(item);
}

// ── 恢复默认主页 ──

bool BrowserProtector::RestoreDefaultHomePage(const Models::BrowserProtectionItem& item) {
    return LockHomePage(item, item.defaultHomePage);
}

bool BrowserProtector::RestoreDefaultSearchEngine(const Models::BrowserProtectionItem& item) {
    // 简化处理
    return false;
}

// ── 设置 Chromium 主页 ──

bool BrowserProtector::SetChromiumHomePage(const std::wstring& prefsPath,
                                             const std::wstring& homePage) {
    std::ifstream ifs(prefsPath);
    if (!ifs.is_open()) return false;

    try {
        nlohmann::json j;
        ifs >> j;
        ifs.close();

        j["homepage"] = homePage;
        j["homepage_is_newtab_page"] = (homePage.find(L"newtab") != std::wstring::npos ||
                                         homePage.find(L"about:blank") != std::wstring::npos);
        j["session"]["restore_on_startup"] = 4;
        j["session"]["startup_urls"] = nlohmann::json::array({ homePage });

        std::ofstream ofs(prefsPath);
        if (!ofs.is_open()) return false;
        ofs << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

// ── IE 主页操作 ──

std::wstring BrowserProtector::GetIEHomePage() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"SOFTWARE\\Microsoft\\Internet Explorer\\Main",
                       0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t buf[512] = {};
        DWORD size = sizeof(buf);
        if (RegQueryValueExW(hKey, L"Start Page", nullptr, nullptr,
                              (LPBYTE)buf, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return buf;
        }
        RegCloseKey(hKey);
    }
    return L"about:blank";
}

bool BrowserProtector::SetIEHomePage(const std::wstring& homePage) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"SOFTWARE\\Microsoft\\Internet Explorer\\Main",
                       0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        bool ok = RegSetValueExW(hKey, L"Start Page", 0, REG_SZ,
                                  (const BYTE*)homePage.c_str(),
                                  static_cast<DWORD>((homePage.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
        RegCloseKey(hKey);
        return ok;
    }
    return false;
}

// ── 劫持检测 ──

bool BrowserProtector::IsHomePageHijacked(const std::wstring& homePage) {
    if (homePage.empty()) return false;

    std::wstring lower = homePage;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    for (const auto& hijacked : s_hijackedHomePages) {
        if (lower.find(hijacked) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

bool BrowserProtector::IsSearchEngineHijacked(const std::wstring& searchEngine) {
    if (searchEngine.empty()) return false;

    std::wstring lower = searchEngine;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    for (const auto& hijacked : s_hijackedSearchEngines) {
        if (lower.find(hijacked) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

// ── 配置管理 ──

Models::BrowserProtectionConfig BrowserProtector::GetConfig() {
    Models::BrowserProtectionConfig config;

    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"SOFTWARE\\IceClean\\BrowserProtector",
                       0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 0;
        DWORD size = sizeof(value);

        if (RegQueryValueExW(hKey, L"HomePageLock", nullptr, nullptr, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            config.enableHomePageLock = (value != 0);
        }
        if (RegQueryValueExW(hKey, L"SearchEngineLock", nullptr, nullptr, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            config.enableSearchEngineLock = (value != 0);
        }

        wchar_t buf[512] = {};
        size = sizeof(buf);
        if (RegQueryValueExW(hKey, L"LockedHomePage", nullptr, nullptr, (LPBYTE)buf, &size) == ERROR_SUCCESS) {
            config.lockedHomePage = buf;
        }

        RegCloseKey(hKey);
    }

    return config;
}

void BrowserProtector::SaveConfig(const Models::BrowserProtectionConfig& config) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                         L"SOFTWARE\\IceClean\\BrowserProtector",
                         0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD homePageLock = config.enableHomePageLock ? 1 : 0;
        DWORD searchEngineLock = config.enableSearchEngineLock ? 1 : 0;

        RegSetValueExW(hKey, L"HomePageLock", 0, REG_DWORD, (const BYTE*)&homePageLock, sizeof(homePageLock));
        RegSetValueExW(hKey, L"SearchEngineLock", 0, REG_DWORD, (const BYTE*)&searchEngineLock, sizeof(searchEngineLock));
        RegSetValueExW(hKey, L"LockedHomePage", 0, REG_SZ,
                        (const BYTE*)config.lockedHomePage.c_str(),
                        static_cast<DWORD>((config.lockedHomePage.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(hKey, L"LockedSearchEngine", 0, REG_SZ,
                        (const BYTE*)config.lockedSearchEngine.c_str(),
                        static_cast<DWORD>((config.lockedSearchEngine.size() + 1) * sizeof(wchar_t)));

        RegCloseKey(hKey);
    }
}

std::wstring BrowserProtector::GetBrowserTypeName(Models::BrowserType type) {
    switch (type) {
    case Models::BrowserType::Chrome:  return L"Google Chrome";
    case Models::BrowserType::Edge:    return L"Microsoft Edge";
    case Models::BrowserType::Firefox: return L"Mozilla Firefox";
    case Models::BrowserType::IE:      return L"Internet Explorer";
    default:                           return L"未知浏览器";
    }
}

} // namespace IceClean::Core::Safety
