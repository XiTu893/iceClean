#include "PrivacyCleaner.h"
#include "utils/FileUtil.h"
#include "utils/Win32Util.h"
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
            firefox.processName = L"firefox.exe";
            paths.push_back(firefox);
        }
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

    auto browserPaths = GetBrowserPaths();

    // 收集所有需要清理的文件路径
    struct CleanItem {
        std::wstring path;
        std::wstring browserName;
        PrivacyType type;
    };

    std::vector<CleanItem> itemsToClean;

    for (const auto& browser : browserPaths) {
        for (PrivacyType type : privacyTypes) {
            std::wstring filePath;
            switch (type) {
            case PrivacyType::Cookies:
                filePath = browser.cookiesPath;
                break;
            case PrivacyType::History:
                filePath = browser.historyPath;
                break;
            case PrivacyType::FormData:
                filePath = browser.formDataPath;
                break;
            }

            if (!filePath.empty() && Utils::FileUtil::Exists(filePath)) {
                itemsToClean.push_back({filePath, browser.browserName, type});
            }
        }
    }

    int totalItems = static_cast<int>(itemsToClean.size());

    // 预计算总大小
    uint64_t totalSize = 0;
    for (const auto& item : itemsToClean) {
        totalSize += Utils::FileUtil::GetFileSize(item.path);
    }

    uint64_t cleanedSize = 0;
    int currentItem = 0;

    for (const auto& item : itemsToClean) {
        // 查找该文件所属浏览器的进程名
        std::wstring processName;
        for (const auto& browser : browserPaths) {
            if (browser.browserName == item.browserName) {
                processName = browser.processName;
                break;
            }
        }

        // 如果浏览器正在运行，跳过该浏览器的所有文件
        if (IsBrowserRunning(processName)) {
            result.failedFileCount++;
            result.failedFiles.push_back(item.path);
            currentItem++;
            continue;
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

        uint64_t fileSize = Utils::FileUtil::GetFileSize(item.path);

        if (DeletePrivacyFile(item.path)) {
            result.cleanedFileCount++;
            cleanedSize += fileSize;
        } else {
            result.failedFileCount++;
            result.failedFiles.push_back(item.path);
        }

        currentItem++;
    }

    result.totalCleanedSize = cleanedSize;

    // 发送完成回调
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

} // namespace IceClean::Core::Cleaner
