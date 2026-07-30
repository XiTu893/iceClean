#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include "models/BrowserProtectionInfo.h"

namespace IceClean::Core::Safety {

// 浏览器保护器 - 防篡改主页/搜索引擎
class BrowserProtector {
public:
    // 扫描所有浏览器的保护状态
    std::vector<Models::BrowserProtectionItem> ScanBrowsers();

    // 锁定浏览器主页
    bool LockHomePage(const Models::BrowserProtectionItem& item, const std::wstring& homePage);

    // 解锁浏览器主页
    bool UnlockHomePage(const Models::BrowserProtectionItem& item);

    // 锁定搜索引擎
    bool LockSearchEngine(const Models::BrowserProtectionItem& item, const std::wstring& searchEngine);

    // 解锁搜索引擎
    bool UnlockSearchEngine(const Models::BrowserProtectionItem& item);

    // 恢复默认主页
    bool RestoreDefaultHomePage(const Models::BrowserProtectionItem& item);

    // 恢复默认搜索引擎
    bool RestoreDefaultSearchEngine(const Models::BrowserProtectionItem& item);

    // 获取保护配置
    static Models::BrowserProtectionConfig GetConfig();

    // 保存保护配置
    static void SaveConfig(const Models::BrowserProtectionConfig& config);

    // 获取浏览器类型显示名
    static std::wstring GetBrowserTypeName(Models::BrowserType type);

private:
    // 扫描 Chrome
    void ScanChrome(std::vector<Models::BrowserProtectionItem>& items);
    // 扫描 Edge
    void ScanEdge(std::vector<Models::BrowserProtectionItem>& items);
    // 扫描 Firefox
    void ScanFirefox(std::vector<Models::BrowserProtectionItem>& items);
    // 扫描 IE
    void ScanIE(std::vector<Models::BrowserProtectionItem>& items);

    // 读取 Chrome/Edge Preferences 文件中的主页和搜索引擎
    bool ReadChromiumBrowserSettings(const std::wstring& prefsPath,
                                      std::wstring& homePage,
                                      std::wstring& searchEngine);

    // 设置 Chrome/Edge 主页
    bool SetChromiumHomePage(const std::wstring& prefsPath, const std::wstring& homePage);

    // 检查主页是否被劫持
    bool IsHomePageHijacked(const std::wstring& homePage);

    // 检查搜索引擎是否被劫持
    bool IsSearchEngineHijacked(const std::wstring& searchEngine);

    // 获取 IE 主页
    std::wstring GetIEHomePage();
    // 设置 IE 主页
    bool SetIEHomePage(const std::wstring& homePage);

    // 已知劫持主页列表
    static const std::vector<std::wstring> s_hijackedHomePages;
    // 已知劫持搜索引擎列表
    static const std::vector<std::wstring> s_hijackedSearchEngines;
};

} // namespace IceClean::Core::Safety
