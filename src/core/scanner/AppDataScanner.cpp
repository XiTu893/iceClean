#include "AppDataScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <shlobj.h>
#include <algorithm>

namespace IceClean::Core::Scanner {

using namespace IceClean::Utils;

bool AppDataScanner::IsAvailable() const {
    std::wstring localAppData = Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%");
    return FileUtil::Exists(localAppData);
}

Models::ScanCategory AppDataScanner::Scan(const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = false; // 默认不选中，Caution 级别

    ScanLocalTemp(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;
    ScanBrowserCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;
    ScanElectronCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;
    ScanPackageManagerCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;
    ScanIdeCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;
    ScanLogFiles(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;
    ScanThumbnailCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;
    ScanUwpCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;
    ScanDeliveryOptCache(category, stopFlag, progressCb);

    return category;
}

void AppDataScanner::ScanAllFiles(const std::wstring& dirPath, Models::ScanCategory& category,
                                   const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    if (!FileUtil::Exists(dirPath)) return;
    ScanDirectory(dirPath, L"*", true, true, category, stopFlag, progressCb);
}

// ── Local\Temp ──

void AppDataScanner::ScanLocalTemp(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring tempPath = Win32Util::ExpandEnvVars(L"%TEMP%");
    if (FileUtil::Exists(tempPath)) {
        ScanDirectory(tempPath, L"*", true, true, category, stopFlag, progressCb);
    }
}

// ── 浏览器缓存 ──

void AppDataScanner::ScanBrowserCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring localAppData = Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%");

    // Chrome 缓存
    std::wstring chromeCache = localAppData + L"\\Google\\Chrome\\User Data\\Default\\Cache";
    if (FileUtil::Exists(chromeCache)) ScanAllFiles(chromeCache, category, stopFlag, progressCb);
    std::wstring chromeCodeCache = localAppData + L"\\Google\\Chrome\\User Data\\Default\\Code Cache";
    if (FileUtil::Exists(chromeCodeCache)) ScanAllFiles(chromeCodeCache, category, stopFlag, progressCb);

    // Edge 缓存
    std::wstring edgeCache = localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Cache";
    if (FileUtil::Exists(edgeCache)) ScanAllFiles(edgeCache, category, stopFlag, progressCb);
    std::wstring edgeCodeCache = localAppData + L"\\Microsoft\\Edge\\User Data\\Default\\Code Cache";
    if (FileUtil::Exists(edgeCodeCache)) ScanAllFiles(edgeCodeCache, category, stopFlag, progressCb);

    // IE/Edge 旧缓存
    std::wstring ieCache = localAppData + L"\\Microsoft\\Windows\\INetCache";
    if (FileUtil::Exists(ieCache)) ScanAllFiles(ieCache, category, stopFlag, progressCb);

    // Firefox 缓存
    std::wstring firefoxCache = localAppData + L"\\Mozilla\\Firefox\\Profiles";
    if (FileUtil::Exists(firefoxCache)) {
        WIN32_FIND_DATAW findData;
        std::wstring searchPath = firefoxCache + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::wstring dirName(findData.cFileName);
                    if (dirName == L"." || dirName == L"..") continue;
                    std::wstring cacheDir = firefoxCache + L"\\" + dirName + L"\\cache2";
                    if (FileUtil::Exists(cacheDir)) ScanAllFiles(cacheDir, category, stopFlag, progressCb);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    // 国内浏览器
    std::vector<std::wstring> cnBrowsers = {
        localAppData + L"\\360Chrome\\Chrome\\User Data\\Default\\Cache",
        localAppData + L"\\360se\\User Data\\Default\\Cache",
        localAppData + L"\\Tencent\\QQBrowser\\User Data\\Default\\Cache",
        localAppData + L"\\Sogou\\SogouExplorer\\Cache",
        localAppData + L"\\liebao\\User Data\\Default\\Cache",
    };
    for (const auto& path : cnBrowsers) {
        if (FileUtil::Exists(path)) ScanAllFiles(path, category, stopFlag, progressCb);
    }
}

// ── Electron 应用缓存 ──

const std::vector<std::pair<std::wstring, std::wstring>>& AppDataScanner::GetElectronAppPaths() {
    static const std::vector<std::pair<std::wstring, std::wstring>> paths = {
        { L"Discord",       L"Discord\\Cache" },
        { L"Slack",         L"Slack\\Cache" },
        { L"Visual Studio Code", L"Programs\\Microsoft VS Code\\User Data\\CachedData" },
        { L"Postman",       L"Postman\\Cache" },
        { L"Figma",         L"Figma\\Cache" },
        { L"Notion",        L"Notion\\Cache" },
        { L"Telegram",      L"Telegram Desktop\\tdata" },
        { L"WhatsApp",      L"WhatsApp\\Cache" },
        { L"Signal",        L"Signal\\Cache" },
        { L"Microsoft Teams", L"Microsoft\\Teams\\Cache" },
        { L"Obsidian",      L"Obsidian\\Cache" },
        { L"Discord",       L"Discord\\Code Cache" },
        { L"Slack",         L"Slack\\Service Worker\\CacheStorage" },
    };
    return paths;
}

void AppDataScanner::ScanElectronCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring localAppData = Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%");
    std::wstring roaming = Win32Util::ExpandEnvVars(L"%APPDATA%");

    for (const auto& [name, relPath] : GetElectronAppPaths()) {
        std::wstring cachePath = localAppData + L"\\" + relPath;
        if (!FileUtil::Exists(cachePath)) {
            cachePath = roaming + L"\\" + relPath;
        }
        if (FileUtil::Exists(cachePath)) {
            ScanAllFiles(cachePath, category, stopFlag, progressCb);
        }
    }
}

// ── 包管理器缓存 ──

void AppDataScanner::ScanPackageManagerCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring localAppData = Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%");
    std::wstring roaming = Win32Util::ExpandEnvVars(L"%APPDATA%");
    std::wstring userProfile = Win32Util::ExpandEnvVars(L"%USERPROFILE%");

    std::vector<std::wstring> cacheDirs = {
        localAppData + L"\\npm-cache",
        localAppData + L"\\NuGet\\Cache",
        localAppData + L"\\pip\\Cache",
        localAppData + L"\\Cargo\\Registry",
        localAppData + L"\\vcpkg\\downloads",
        roaming + L"\\npm-cache",
        userProfile + L"\\.nuget\\packages",
        userProfile + L"\\.gradle\\caches",
        userProfile + L"\\.m2\\repository",
        userProfile + L"\\AppData\\Local\\Temp\\chocolatey",
    };

    for (const auto& dir : cacheDirs) {
        if (FileUtil::Exists(dir)) ScanAllFiles(dir, category, stopFlag, progressCb);
    }
}

// ── IDE 缓存 ──

void AppDataScanner::ScanIdeCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring localAppData = Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%");
    std::wstring roaming = Win32Util::ExpandEnvVars(L"%APPDATA%");

    std::vector<std::wstring> ideCacheDirs = {
        roaming + L"\\Code\\CachedData",
        roaming + L"\\Code\\CachedExtensionVSIXs",
        roaming + L"\\Code\\Cache",
        localAppData + L"\\JetBrains\\cache",
        localAppData + L"\\JetBrains\\TakeOutCache",
        localAppData + L"\\Microsoft\\VisualStudio\\*\\Cache",
        roaming + L"\\JetBrains\\*\\cache",
    };

    // VS Code 缓存
    std::wstring vscodeCache = roaming + L"\\Code\\CachedData";
    if (FileUtil::Exists(vscodeCache)) ScanAllFiles(vscodeCache, category, stopFlag, progressCb);

    // JetBrains 缓存（通配处理：查找所有版本）
    std::wstring jetbrainsDir = localAppData + L"\\JetBrains";
    if (FileUtil::Exists(jetbrainsDir)) {
        WIN32_FIND_DATAW findData;
        std::wstring searchPath = jetbrainsDir + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::wstring dirName(findData.cFileName);
                    if (dirName == L"." || dirName == L"..") continue;
                    std::wstring cacheDir = jetbrainsDir + L"\\" + dirName + L"\\cache";
                    if (FileUtil::Exists(cacheDir)) ScanAllFiles(cacheDir, category, stopFlag, progressCb);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }
}

// ── 日志文件 ──

void AppDataScanner::ScanLogFiles(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring localAppData = Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%");

    // Windows 日志
    std::wstring windowsLogs = localAppData + L"\\Microsoft\\Windows\\*.log";
    ScanDirectory(localAppData + L"\\Microsoft\\Windows", L"*.log", true, true, category, stopFlag, progressCb);

    // Windows ETL 跟踪日志
    std::wstring etlDir = localAppData + L"\\Temp";
    if (FileUtil::Exists(etlDir)) {
        ScanDirectory(etlDir, L"*.etl", false, true, category, stopFlag, progressCb);
        ScanDirectory(etlDir, L"*.log", false, true, category, stopFlag, progressCb);
    }

    // 软件日志（常见位置）
    std::vector<std::wstring> logDirs = {
        localAppData + L"\\Microsoft\\Windows\\WER",
        localAppData + L"\\CrashDumps",
        localAppData + L"\\Microsoft\\Windows\\INetCache\\IE",
        localAppData + L"\\Microsoft\\Windows\\WebCache",
    };
    for (const auto& dir : logDirs) {
        if (FileUtil::Exists(dir)) ScanAllFiles(dir, category, stopFlag, progressCb);
    }
}

// ── 缩略图缓存 ──

void AppDataScanner::ScanThumbnailCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring thumbCacheDir = Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\Microsoft\\Windows\\Explorer");
    if (FileUtil::Exists(thumbCacheDir)) {
        ScanDirectory(thumbCacheDir, L"thumbcache_*.db", false, true, category, stopFlag, progressCb);
        ScanDirectory(thumbCacheDir, L"thumbcache_*.idx", false, true, category, stopFlag, progressCb);
    }
}

// ── UWP 应用缓存 ──

void AppDataScanner::ScanUwpCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring packagesDir = Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%\\Packages");

    if (!FileUtil::Exists(packagesDir)) return;

    WIN32_FIND_DATAW findData;
    std::wstring searchPath = packagesDir + L"\\*";
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (ShouldStop(stopFlag)) break;
        if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        std::wstring dirName(findData.cFileName);
        if (dirName == L"." || dirName == L"..") continue;

        // 只扫描已知可清理的 UWP 包
        std::wstring lowerName = dirName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::towlower);

        // 已知可清理的 UWP 包缓存
        if (lowerName.find(L"microsoft.windowsstore") != std::wstring::npos ||
            lowerName.find(L"microsoft.msn") != std::wstring::npos ||
            lowerName.find(L"microsoft.bing") != std::wstring::npos ||
            lowerName.find(L"microsoft.people") != std::wstring::npos ||
            lowerName.find(L"microsoft.skypeapp") != std::wstring::npos ||
            lowerName.find(L"microsoft.windowsmaps") != std::wstring::npos ||
            lowerName.find(L"microsoft.windowsalarms") != std::wstring::npos ||
            lowerName.find(L"microsoft.microsoftofficehub") != std::wstring::npos) {
            std::wstring localState = packagesDir + L"\\" + dirName + L"\\LocalState";
            if (FileUtil::Exists(localState)) ScanAllFiles(localState, category, stopFlag, progressCb);
        }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);
}

// ── 传递优化缓存 ──

void AppDataScanner::ScanDeliveryOptCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring deliveryOptDir = L"C:\\ProgramData\\Microsoft\\DeliveryOptimization\\Cache";
    if (FileUtil::Exists(deliveryOptDir)) {
        ScanAllFiles(deliveryOptDir, category, stopFlag, progressCb);
    }
}

} // namespace IceClean::Core::Scanner
