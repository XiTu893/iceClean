#pragma once
#include <string>
#include <vector>

namespace IceClean::Models {

// 浏览器类型
enum class BrowserType {
    Chrome,
    Edge,
    Firefox,
    IE,
    Unknown
};

// 浏览器保护项
struct BrowserProtectionItem {
    BrowserType browser = BrowserType::Unknown;
    std::wstring browserName;       // 浏览器名称
    std::wstring homePage;          // 当前主页
    std::wstring defaultHomePage;   // 默认主页
    std::wstring searchEngine;      // 当前搜索引擎
    std::wstring defaultSearchEngine; // 默认搜索引擎
    bool isHomePageLocked = false;   // 主页是否被锁定
    bool isSearchEngineLocked = false; // 搜索引擎是否被锁定
    bool isHomePageHijacked = false;   // 主页是否被劫持
    bool isSearchEngineHijacked = false; // 搜索引擎是否被劫持
    std::wstring installPath;        // 安装路径
};

// 浏览器保护配置
struct BrowserProtectionConfig {
    bool enableHomePageLock = false;      // 启用主页锁定
    bool enableSearchEngineLock = false;   // 启用搜索引擎锁定
    bool enableExtensionGuard = false;     // 启用扩展保护
    std::wstring lockedHomePage = L"about:blank";     // 锁定的主页
    std::wstring lockedSearchEngine = L"google.com";  // 锁定的搜索引擎
};

} // namespace IceClean::Models
