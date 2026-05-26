#include "BrowserCacheScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <shlobj.h>

namespace IceClean::Core::Scanner {

Models::ScanCategory BrowserCacheScanner::Scan(const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    std::wstring localAppData = Utils::Win32Util::GetSpecialFolder(CSIDL_LOCAL_APPDATA);

    // ---- Google Chrome ----
    if (!IsBrowserRunning(L"chrome.exe")) {
        std::wstring chromeCache = localAppData + L"\\Google\\Chrome\\User Data\\Default\\Cache";
        ScanBrowserCache(chromeCache, category, stopFlag, progressCb);

        // Chrome 新版缓存目录
        std::wstring chromeCacheData = localAppData + L"\\Google\\Chrome\\User Data\\Default\\Code Cache";
        ScanBrowserCache(chromeCacheData, category, stopFlag, progressCb);

        // Chrome Service Worker 缓存
        std::wstring chromeSWCache = localAppData + L"\\Google\\Chrome\\User Data\\Default\\Service Worker\\CacheStorage";
        ScanBrowserCache(chromeSWCache, category, stopFlag, progressCb);
    }

    // ---- Microsoft Edge ----
    if (!IsBrowserRunning(L"msedge.exe")) {
        std::wstring edgeCache = localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Cache";
        ScanBrowserCache(edgeCache, category, stopFlag, progressCb);

        std::wstring edgeCodeCache = localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache";
        ScanBrowserCache(edgeCodeCache, category, stopFlag, progressCb);

        std::wstring edgeSWCache = localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Service Worker\\CacheStorage";
        ScanBrowserCache(edgeSWCache, category, stopFlag, progressCb);
    }

    // ---- Mozilla Firefox ----
    if (!IsBrowserRunning(L"firefox.exe")) {
        std::wstring firefoxCache = localAppData + L"\\Mozilla\\Firefox\\Profiles";
        if (Utils::FileUtil::Exists(firefoxCache)) {
            // Firefox 的缓存目录在各个 profile 子目录下
            std::wstring searchPath = firefoxCache;
            if (!searchPath.empty() && searchPath.back() != L'\\') {
                searchPath += L'\\';
            }
            searchPath += L"*";

            WIN32_FIND_DATAW findData{};
            HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        if (wcscmp(findData.cFileName, L".") == 0 ||
                            wcscmp(findData.cFileName, L"..") == 0) continue;

                        std::wstring profilePath = firefoxCache + L"\\" + findData.cFileName;
                        std::wstring cacheDir = profilePath + L"\\cache2";
                        ScanBrowserCache(cacheDir, category, stopFlag, progressCb);
                    }
                } while (FindNextFileW(hFind, &findData));
                FindClose(hFind);
            }
        }
    }

    // ---- Opera ----
    if (!IsBrowserRunning(L"opera.exe")) {
        std::wstring appData = Utils::Win32Util::GetSpecialFolder(CSIDL_APPDATA);
        std::wstring operaBase = appData + L"\\Opera Software\\Opera Stable";
        ScanBrowserCache(operaBase + L"\\Cache", category, stopFlag, progressCb);
        ScanBrowserCache(operaBase + L"\\Code Cache", category, stopFlag, progressCb);
        ScanBrowserCache(operaBase + L"\\Service Worker\\CacheStorage", category, stopFlag, progressCb);
    }

    // ---- Brave ----
    if (!IsBrowserRunning(L"brave.exe")) {
        std::wstring braveBase = localAppData + L"\\BraveSoftware\\Brave-Browser\\User Data";
        ScanBrowserCache(braveBase + L"\\Default\\Cache", category, stopFlag, progressCb);
        ScanBrowserCache(braveBase + L"\\Default\\Code Cache", category, stopFlag, progressCb);
        ScanBrowserCache(braveBase + L"\\Default\\Service Worker\\CacheStorage", category, stopFlag, progressCb);
    }

    // ---- Vivaldi ----
    if (!IsBrowserRunning(L"vivaldi.exe")) {
        std::wstring vivaldiBase = localAppData + L"\\Vivaldi\\User Data";
        ScanBrowserCache(vivaldiBase + L"\\Default\\Cache", category, stopFlag, progressCb);
        ScanBrowserCache(vivaldiBase + L"\\Default\\Code Cache", category, stopFlag, progressCb);
        ScanBrowserCache(vivaldiBase + L"\\Default\\Service Worker\\CacheStorage", category, stopFlag, progressCb);
    }

    // ---- 360安全浏览器 ----
    if (!IsBrowserRunning(L"360se.exe")) {
        std::wstring appData = Utils::Win32Util::GetSpecialFolder(CSIDL_APPDATA);
        std::wstring se360Base = appData + L"\\360se6\\User Data";
        ScanBrowserCache(se360Base + L"\\Default\\Cache", category, stopFlag, progressCb);
        ScanBrowserCache(se360Base + L"\\Default\\Code Cache", category, stopFlag, progressCb);
    }

    // ---- QQ浏览器 ----
    if (!IsBrowserRunning(L"QQBrowser.exe")) {
        std::wstring appData = Utils::Win32Util::GetSpecialFolder(CSIDL_APPDATA);
        std::wstring qqBase = appData + L"\\Tencent\\QQBrowser\\User Data";
        ScanBrowserCache(qqBase + L"\\Default\\Cache", category, stopFlag, progressCb);
        ScanBrowserCache(qqBase + L"\\Default\\Code Cache", category, stopFlag, progressCb);
    }

    return category;
}

void BrowserCacheScanner::ScanBrowserCache(const std::wstring& cachePath, Models::ScanCategory& category,
                                            const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    if (!Utils::FileUtil::Exists(cachePath)) return;
    ScanDirectory(cachePath, L"*", true, false, category, stopFlag, progressCb);
}

bool BrowserCacheScanner::IsBrowserRunning(const std::wstring& browserExeName) const {
    return Utils::Win32Util::IsProcessRunning(browserExeName);
}

bool BrowserCacheScanner::IsAvailable() const {
    // 至少有一个浏览器缓存目录存在
    std::wstring localAppData = Utils::Win32Util::GetSpecialFolder(CSIDL_LOCAL_APPDATA);
    std::wstring appData = Utils::Win32Util::GetSpecialFolder(CSIDL_APPDATA);
    if (localAppData.empty() && appData.empty()) return false;

    return Utils::FileUtil::Exists(localAppData + L"\\Google\\Chrome\\User Data") ||
           Utils::FileUtil::Exists(localAppData + L"\\Microsoft\\Edge\\User Data") ||
           Utils::FileUtil::Exists(localAppData + L"\\Mozilla\\Firefox\\Profiles") ||
           Utils::FileUtil::Exists(appData + L"\\Opera Software\\Opera Stable") ||
           Utils::FileUtil::Exists(localAppData + L"\\BraveSoftware\\Brave-Browser\\User Data") ||
           Utils::FileUtil::Exists(localAppData + L"\\Vivaldi\\User Data") ||
           Utils::FileUtil::Exists(appData + L"\\360se6\\User Data") ||
           Utils::FileUtil::Exists(appData + L"\\Tencent\\QQBrowser\\User Data");
}

} // namespace IceClean::Core::Scanner
