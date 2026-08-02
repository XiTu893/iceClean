#include "PrivacyCleaner.h"
#include "utils/FileUtil.h"
#include "utils/Win32Util.h"
#include "utils/RegistryUtil.h"
#include <tlhelp32.h>
#include <algorithm>

namespace IceClean::Core::Cleaner {

PrivacyCleaner::PrivacyCleaner() {
}

std::vector<BrowserPrivacyPaths> PrivacyCleaner::GetBrowserPaths() const {
    std::vector<BrowserPrivacyPaths> paths;

    // Chrome
    {
        std::wstring chromeBase = Utils::Win32Util::ExpandEnvVars(
            L"%LOCALAPPDATA%\\Google\\Chrome\\User Data\\Default\\");
        BrowserPrivacyPaths chrome;
        chrome.browserName = L"Chrome";
        chrome.cookiesPath = chromeBase + L"Cookies";
        chrome.historyPath = chromeBase + L"History";
        chrome.formDataPath = chromeBase + L"Web Data";
        chrome.cachePath = Utils::Win32Util::ExpandEnvVars(
            L"%LOCALAPPDATA%\\Google\\Chrome\\User Data\\Default\\Cache\\");
        chrome.sessionPath = chromeBase + L"Current Session";
        chrome.loginDataPath = chromeBase + L"Login Data";
        chrome.downloadPath = chromeBase + L"History";  // 下载记录在History中
        chrome.processName = L"chrome.exe";
        paths.push_back(chrome);
    }

    // Edge
    {
        std::wstring edgeBase = Utils::Win32Util::ExpandEnvVars(
            L"%LOCALAPPDATA%\\Microsoft\\Edge\\User Data\\Default\\");
        BrowserPrivacyPaths edge;
        edge.browserName = L"Edge";
        edge.cookiesPath = edgeBase + L"Cookies";
        edge.historyPath = edgeBase + L"History";
        edge.formDataPath = edgeBase + L"Web Data";
        edge.cachePath = Utils::Win32Util::ExpandEnvVars(
            L"%LOCALAPPDATA%\\Microsoft\\Edge\\User Data\\Default\\Cache\\");
        edge.sessionPath = edgeBase + L"Current Session";
        edge.loginDataPath = edgeBase + L"Login Data";
        edge.downloadPath = edgeBase + L"History";
        edge.processName = L"msedge.exe";
        paths.push_back(edge);
    }

    // Firefox
    {
        std::wstring firefoxProfilesDir = Utils::Win32Util::ExpandEnvVars(
            L"%APPDATA%\\Mozilla\\Firefox\\Profiles\\");

        // 查找 .default-release 配置目录
        std::wstring firefoxProfileDir;
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((firefoxProfilesDir + L"*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::wstring dirName = findData.cFileName;
                    if (dirName.length() > 15 &&
                        dirName.substr(dirName.length() - 15) == L".default-release") {
                        firefoxProfileDir = firefoxProfilesDir + dirName + L"\\";
                        break;
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }

        if (!firefoxProfileDir.empty()) {
            BrowserPrivacyPaths firefox;
            firefox.browserName = L"Firefox";
            firefox.cookiesPath = firefoxProfileDir + L"cookies.sqlite";
            firefox.historyPath = firefoxProfileDir + L"places.sqlite";
            firefox.formDataPath = firefoxProfileDir + L"formhistory.sqlite";
            firefox.cachePath = Utils::Win32Util::ExpandEnvVars(
                L"%LOCALAPPDATA%\\Mozilla\\Firefox\\Profiles\\") +
                firefoxProfileDir.substr(firefoxProfileDir.find_last_of(L"\\") + 1) + L"\\cache2\\";
            firefox.sessionPath = firefoxProfileDir + L"sessionstore.jsonlz4";
            firefox.loginDataPath = firefoxProfileDir + L"logins.json";
            firefox.downloadPath = firefoxProfileDir + L"places.sqlite"; // 下载记录在places中
            firefox.processName = L"firefox.exe";
            paths.push_back(firefox);
        }
    }

    // Brave
    {
        std::wstring braveBase = Utils::Win32Util::ExpandEnvVars(
            L"%LOCALAPPDATA%\\BraveSoftware\\Brave-Browser\\User Data\\Default\\");
        BrowserPrivacyPaths brave;
        brave.browserName = L"Brave";
        brave.cookiesPath = braveBase + L"Cookies";
        brave.historyPath = braveBase + L"History";
        brave.formDataPath = braveBase + L"Web Data";
        brave.cachePath = braveBase + L"Cache\\";
        brave.sessionPath = braveBase + L"Current Session";
        brave.loginDataPath = braveBase + L"Login Data";
        brave.downloadPath = braveBase + L"History";
        brave.processName = L"brave.exe";
        paths.push_back(brave);
    }

    // Vivaldi
    {
        std::wstring vivaldiBase = Utils::Win32Util::ExpandEnvVars(
            L"%LOCALAPPDATA%\\Vivaldi\\User Data\\Default\\");
        BrowserPrivacyPaths vivaldi;
        vivaldi.browserName = L"Vivaldi";
        vivaldi.cookiesPath = vivaldiBase + L"Cookies";
        vivaldi.historyPath = vivaldiBase + L"History";
        vivaldi.formDataPath = vivaldiBase + L"Web Data";
        vivaldi.cachePath = vivaldiBase + L"Cache\\";
        vivaldi.sessionPath = vivaldiBase + L"Current Session";
        vivaldi.loginDataPath = vivaldiBase + L"Login Data";
        vivaldi.downloadPath = vivaldiBase + L"History";
        vivaldi.processName = L"vivaldi.exe";
        paths.push_back(vivaldi);
    }

    // Opera
    {
        std::wstring operaBase = Utils::Win32Util::ExpandEnvVars(
            L"%APPDATA%\\Opera Software\\Opera Stable\\");
        BrowserPrivacyPaths opera;
        opera.browserName = L"Opera";
        opera.cookiesPath = operaBase + L"Cookies";
        opera.historyPath = operaBase + L"History";
        opera.formDataPath = operaBase + L"Web Data";
        opera.cachePath = operaBase + L"Cache\\";
        opera.sessionPath = operaBase + L"Current Session";
        opera.loginDataPath = operaBase + L"Login Data";
        opera.downloadPath = operaBase + L"History";
        opera.processName = L"opera.exe";
        paths.push_back(opera);
    }

    return paths;
}

bool PrivacyCleaner::IsBrowserRunning(const std::wstring& processName) const {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                CloseHandle(hSnapshot);
                return true;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return false;
}

bool PrivacyCleaner::DeletePrivacyFile(const std::wstring& path) {
    return CleanerBase::DeleteFile(path);
}

Models::CleanResult PrivacyCleaner::Clean(const std::vector<std::wstring>& paths,
                                            std::function<void(const Models::CleanProgress&)> progressCallback,
                                            const std::atomic<bool>* cancelFlag) {
    // ICleaner 接口实现 - 将 paths 作为文件路径列表直接删除
    Models::CleanResult result;
    result.success = true;
    int totalItems = static_cast<int>(paths.size());
    uint64_t cleanedSize = 0;

    for (int i = 0; i < totalItems; ++i) {
        if (cancelFlag && cancelFlag->load()) break;

        if (progressCallback) {
            Models::CleanProgress progress;
            progress.currentItem = i;
            progress.totalItems = totalItems;
            progress.cleanedSize = cleanedSize;
            progress.currentFile = paths[i];
            progress.isRunning = true;
            progressCallback(progress);
        }

        uint64_t fileSize = Utils::FileUtil::GetFileSize(paths[i]);
        if (DeletePrivacyFile(paths[i])) {
            result.cleanedFileCount++;
            cleanedSize += fileSize;
        } else {
            result.failedFileCount++;
            result.failedFiles.push_back(paths[i]);
        }
    }

    result.totalCleanedSize = cleanedSize;
    return result;
}

Models::CleanResult PrivacyCleaner::CleanPrivacy(const std::vector<PrivacyType>& privacyTypes,
                                                   std::function<void(const Models::CleanProgress&)> progressCb) {
    Models::CleanResult result;
    result.success = true;

    // 收集所有需要清理的文件路径
    struct CleanItem {
        std::wstring path;
        std::wstring browserName;  // 为空表示系统/应用隐私
        PrivacyType type;
        bool isDirectory;
    };

    std::vector<CleanItem> itemsToClean;

    auto browserPaths = GetBrowserPaths();

    // 浏览器隐私数据
    for (const auto& browser : browserPaths) {
        for (PrivacyType type : privacyTypes) {
            std::wstring filePath;
            bool isDir = false;
            switch (type) {
            case PrivacyType::Cookies:
                filePath = browser.cookiesPath; break;
            case PrivacyType::History:
                filePath = browser.historyPath; break;
            case PrivacyType::FormData:
                filePath = browser.formDataPath; break;
            case PrivacyType::Cache:
                filePath = browser.cachePath; isDir = true; break;
            case PrivacyType::Session:
                filePath = browser.sessionPath; break;
            case PrivacyType::Passwords:
                filePath = browser.loginDataPath; break;
            case PrivacyType::DownloadHistory:
                filePath = browser.downloadPath; break;
            default:
                break;
            }

            if (!filePath.empty() && (isDir || Utils::FileUtil::Exists(filePath))) {
                itemsToClean.push_back({filePath, browser.browserName, type, isDir});
            }
        }
    }

    // 系统和应用隐私数据
    for (PrivacyType type : privacyTypes) {
        auto sysPaths = GetSystemPrivacyPaths(type);
        for (const auto& p : sysPaths) {
            itemsToClean.push_back({p, L"", type, false});
        }

        auto appPaths = GetAppPrivacyPaths(type);
        for (const auto& p : appPaths) {
            itemsToClean.push_back({p, L"", type, false});
        }
    }

    int totalItems = static_cast<int>(itemsToClean.size());

    // 预计算总大小
    uint64_t totalSize = 0;
    for (const auto& item : itemsToClean) {
        if (!item.isDirectory) {
            totalSize += Utils::FileUtil::GetFileSize(item.path);
        }
    }

    uint64_t cleanedSize = 0;
    int currentItem = 0;

    for (const auto& item : itemsToClean) {
        // 如果是浏览器隐私，检查浏览器是否在运行
        if (!item.browserName.empty()) {
            std::wstring processName;
            for (const auto& browser : browserPaths) {
                if (browser.browserName == item.browserName) {
                    processName = browser.processName;
                    break;
                }
            }
            if (IsBrowserRunning(processName)) {
                result.failedFileCount++;
                result.failedFiles.push_back(item.path);
                currentItem++;
                continue;
            }
        }

        // 检查白名单
        if (IsWhitelisted(item.path)) {
            currentItem++;
            continue;
        }

        // 发送进度回调
        if (progressCb) {
            Models::CleanProgress progress;
            progress.currentItem = currentItem;
            progress.totalItems = totalItems;
            progress.cleanedSize = cleanedSize;
            progress.totalSize = totalSize;
            progress.currentFile = item.path;
            progress.isRunning = true;
            progressCb(progress);
        }

        uint64_t fileSize = 0;
        bool success = false;

        if (item.isDirectory) {
            success = CleanerBase::DeleteDirectory(item.path);
        } else {
            fileSize = Utils::FileUtil::GetFileSize(item.path);
            success = DeletePrivacyFile(item.path);
        }

        if (success) {
            result.cleanedFileCount++;
            cleanedSize += fileSize;
        } else {
            result.failedFileCount++;
            result.failedFiles.push_back(item.path);
        }

        currentItem++;
    }

    result.totalCleanedSize = cleanedSize;

    if (progressCb) {
        Models::CleanProgress progress;
        progress.currentItem = totalItems;
        progress.totalItems = totalItems;
        progress.cleanedSize = cleanedSize;
        progress.totalSize = totalSize;
        progress.isRunning = false;
        progressCb(progress);
    }

    return result;
}

// ============================================================================
// 获取系统隐私数据文件路径
// ============================================================================

std::vector<std::wstring> PrivacyCleaner::GetSystemPrivacyPaths(PrivacyType type) const {
    std::vector<std::wstring> paths;

    switch (type) {
    case PrivacyType::RecentDocs: {
        // 最近文档记录
        std::wstring recentDir = Utils::Win32Util::ExpandEnvVars(L"%APPDATA%\\Microsoft\\Windows\\Recent\\");
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((recentDir + L"*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring fileName = findData.cFileName;
                    // 只清理 .lnk 快捷方式文件
                    if (fileName.size() > 4 && fileName.substr(fileName.size() - 4) == L".lnk") {
                        paths.push_back(recentDir + fileName);
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        // 自动跳转列表
        std::wstring automaticDestinations = Utils::Win32Util::ExpandEnvVars(
            L"%APPDATA%\\Microsoft\\Windows\\Recent\\AutomaticDestinations\\");
        hFind = FindFirstFileW((automaticDestinations + L"*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    paths.push_back(automaticDestinations + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        // CustomDestinations
        std::wstring customDestinations = Utils::Win32Util::ExpandEnvVars(
            L"%APPDATA%\\Microsoft\\Windows\\Recent\\CustomDestinations\\");
        hFind = FindFirstFileW((customDestinations + L"*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    paths.push_back(customDestinations + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        break;
    }

    case PrivacyType::RunHistory: {
        // 运行对话框历史
        // HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\RunMRU
        // 这是注册表项，无法直接删除文件，但可以通过RegistryCleaner处理
        // 这里添加相关的跟踪文件
        std::wstring runMRUFiles = Utils::Win32Util::ExpandEnvVars(
            L"%APPDATA%\\Microsoft\\Windows\\Recent\\AutomaticDestinations\\");
        // Run对话框的历史保存在AutomaticDestinations中，已在RecentDocs中包含
        break;
    }

    case PrivacyType::SearchHistory: {
        // Windows搜索历史
        std::wstring searchDir = Utils::Win32Util::ExpandEnvVars(
            L"%LOCALAPPDATA%\\Packages\\Microsoft.Windows.Search_cw5n1h2txyewy\\LocalState\\");
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((searchDir + L"*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    paths.push_back(searchDir + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        break;
    }

    case PrivacyType::ClipboardHistory: {
        // 剪贴板历史
        std::wstring clipDir = Utils::Win32Util::ExpandEnvVars(
            L"%LOCALAPPDATA%\\Microsoft\\Windows\\Clipboard\\");
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((clipDir + L"*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    paths.push_back(clipDir + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        break;
    }

    case PrivacyType::JumpList: {
        // 跳转列表已在RecentDocs中覆盖
        break;
    }

    case PrivacyType::ThumbnailCache: {
        // 缩略图缓存
        std::wstring thumbCacheDir = Utils::Win32Util::ExpandEnvVars(
            L"%LOCALAPPDATA%\\Microsoft\\Windows\\Explorer\\");
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((thumbCacheDir + L"thumbcache_*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    paths.push_back(thumbCacheDir + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        break;
    }

    default:
        break;
    }

    return paths;
}

// ============================================================================
// 获取应用程序隐私数据文件路径
// ============================================================================

std::vector<std::wstring> PrivacyCleaner::GetAppPrivacyPaths(PrivacyType type) const {
    std::vector<std::wstring> paths;

    switch (type) {
    case PrivacyType::OfficeRecent: {
        // Office 最近文件记录
        std::wstring officeRecentDir = Utils::Win32Util::ExpandEnvVars(
            L"%APPDATA%\\Microsoft\\Office\\Recent\\");
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((officeRecentDir + L"*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    paths.push_back(officeRecentDir + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        // Office 2016+ MRU
        std::wstring officeMRU = Utils::Win32Util::ExpandEnvVars(
            L"%APPDATA%\\Microsoft\\Office\\RecentItems\\");
        hFind = FindFirstFileW((officeMRU + L"*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    paths.push_back(officeMRU + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        break;
    }

    case PrivacyType::ArchiveHistory: {
        // WinRAR 历史记录
        std::wstring winrarRegKey = L"SOFTWARE\\WinRAR\\ArcHistory";
        // WinRAR历史在注册表中，清理文件路径无效
        // 但可以清理WinRAR临时解压目录
        std::wstring winrarTemp = Utils::Win32Util::ExpandEnvVars(L"%TEMP%\\Rar$*");
        // 7-Zip 历史
        std::wstring sevenZipHistory = Utils::Win32Util::ExpandEnvVars(
            L"%APPDATA%\\7-Zip\\");
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((sevenZipHistory + L"*.txt").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    paths.push_back(sevenZipHistory + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        break;
    }

    case PrivacyType::DownloadHistory: {
        // 迅雷下载记录
        std::wstring thunderData = Utils::Win32Util::ExpandEnvVars(
            L"%APPDATA%\\Thunder Network\\Thunder\\");
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW((thunderData + L"*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring name = findData.cFileName;
                    // 只清理历史相关文件
                    if (name.find(L"History") != std::wstring::npos ||
                        name.find(L"history") != std::wstring::npos ||
                        name.find(L".td") != std::wstring::npos) {
                        paths.push_back(thunderData + name);
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        // IDM下载记录
        std::wstring idmData = Utils::Win32Util::ExpandEnvVars(
            L"%APPDATA%\\IDM\\");
        hFind = FindFirstFileW((idmData + L"*").c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring name = findData.cFileName;
                    if (name.find(L"Down") != std::wstring::npos ||
                        name.find(L".lst") != std::wstring::npos) {
                        paths.push_back(idmData + name);
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
        break;
    }

    default:
        break;
    }

    return paths;
}

} // namespace IceClean::Core::Cleaner
