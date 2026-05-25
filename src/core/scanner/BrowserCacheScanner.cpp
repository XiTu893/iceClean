#include "BrowserCacheScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <shlobj.h>

namespace IceClean::Core::Scanner {

Models::ScanCategory BrowserCacheScanner::Scan() {
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
        ScanBrowserCache(chromeCache, category);

        // Chrome 新版缓存目录
        std::wstring chromeCacheData = localAppData + L"\\Google\\Chrome\\User Data\\Default\\Code Cache";
        ScanBrowserCache(chromeCacheData, category);

        // Chrome Service Worker 缓存
        std::wstring chromeSWCache = localAppData + L"\\Google\\Chrome\\User Data\\Default\\Service Worker\\CacheStorage";
        ScanBrowserCache(chromeSWCache, category);
    }

    // ---- Microsoft Edge ----
    if (!IsBrowserRunning(L"msedge.exe")) {
        std::wstring edgeCache = localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Cache";
        ScanBrowserCache(edgeCache, category);

        std::wstring edgeCodeCache = localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache";
        ScanBrowserCache(edgeCodeCache, category);

        std::wstring edgeSWCache = localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Service Worker\\CacheStorage";
        ScanBrowserCache(edgeSWCache, category);
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
                        ScanBrowserCache(cacheDir, category);
                    }
                } while (FindNextFileW(hFind, &findData));
                FindClose(hFind);
            }
        }
    }

    return category;
}

void BrowserCacheScanner::ScanBrowserCache(const std::wstring& cachePath, Models::ScanCategory& category) {
    if (!Utils::FileUtil::Exists(cachePath)) return;
    ScanDirectory(cachePath, L"*", true, false, category);
}

bool BrowserCacheScanner::IsBrowserRunning(const std::wstring& browserExeName) const {
    return Utils::Win32Util::IsProcessRunning(browserExeName);
}

bool BrowserCacheScanner::IsAvailable() const {
    // 至少有一个浏览器缓存目录存在
    std::wstring localAppData = Utils::Win32Util::GetSpecialFolder(CSIDL_LOCAL_APPDATA);
    if (localAppData.empty()) return false;

    return Utils::FileUtil::Exists(localAppData + L"\\Google\\Chrome\\User Data") ||
           Utils::FileUtil::Exists(localAppData + L"\\Microsoft\\Edge\\User Data") ||
           Utils::FileUtil::Exists(localAppData + L"\\Mozilla\\Firefox\\Profiles");
}

} // namespace IceClean::Core::Scanner
