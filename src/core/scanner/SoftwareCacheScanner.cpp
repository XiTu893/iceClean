#include "SoftwareCacheScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <shlobj.h>

namespace IceClean::Core::Scanner {

Models::ScanCategory SoftwareCacheScanner::Scan(const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    ScanWeChatCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanQQCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanThunderCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanVideoAppCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanWPSCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanDingTalkCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanYoudaoCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanElectronAppCache(category, stopFlag, progressCb);
    if (ShouldStop(stopFlag)) return category;

    ScanPackageManagerCache(category, stopFlag, progressCb);

    return category;
}

void SoftwareCacheScanner::ScanWeChatCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    // 微信缓存目录: Documents/WeChat Files/*/FileStorage/Cache
    std::wstring weChatBase = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\Documents\\WeChat Files");
    if (Utils::FileUtil::Exists(weChatBase)) {
        // 扫描所有微信账号目录下的缓存
        WIN32_FIND_DATAW findData;
        std::wstring searchPath = weChatBase + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::wstring dirName(findData.cFileName);
                    if (dirName == L"." || dirName == L"..") continue;

                    // FileStorage\Cache
                    std::wstring cachePath = weChatBase + L"\\" + dirName + L"\\FileStorage\\Cache";
                    if (Utils::FileUtil::Exists(cachePath)) {
                        ScanDirectory(cachePath, L"*", true, true, category, stopFlag, progressCb);
                    }

                    // FileStorage\Temp
                    std::wstring tempPath = weChatBase + L"\\" + dirName + L"\\FileStorage\\Temp";
                    if (Utils::FileUtil::Exists(tempPath)) {
                        ScanDirectory(tempPath, L"*", true, true, category, stopFlag, progressCb);
                    }

                    // FileStorage\MsgAttach\Temp
                    std::wstring msgTempPath = weChatBase + L"\\" + dirName + L"\\FileStorage\\MsgAttach\\Temp";
                    if (Utils::FileUtil::Exists(msgTempPath)) {
                        ScanDirectory(msgTempPath, L"*", true, true, category, stopFlag, progressCb);
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    // 微信小程序缓存
    std::wstring wxApplet = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\Documents\\WeChat Files\\Applet");
    if (Utils::FileUtil::Exists(wxApplet)) {
        ScanDirectory(wxApplet, L"*", true, true, category, stopFlag, progressCb);
    }
}

void SoftwareCacheScanner::ScanQQCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    // QQ缓存目录: Documents\Tencent Files\*\Image\ 和 Video
    std::wstring qqBase = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\Documents\\Tencent Files");
    if (Utils::FileUtil::Exists(qqBase)) {
        WIN32_FIND_DATAW findData;
        std::wstring searchPath = qqBase + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::wstring dirName(findData.cFileName);
                    if (dirName == L"." || dirName == L"..") continue;

                    // Image
                    std::wstring imagePath = qqBase + L"\\" + dirName + L"\\Image";
                    if (Utils::FileUtil::Exists(imagePath)) {
                        ScanDirectory(imagePath, L"*", true, true, category, stopFlag, progressCb);
                    }

                    // Video
                    std::wstring videoPath = qqBase + L"\\" + dirName + L"\\Video";
                    if (Utils::FileUtil::Exists(videoPath)) {
                        ScanDirectory(videoPath, L"*", true, true, category, stopFlag, progressCb);
                    }

                    // Photo
                    std::wstring photoPath = qqBase + L"\\" + dirName + L"\\Photo";
                    if (Utils::FileUtil::Exists(photoPath)) {
                        ScanDirectory(photoPath, L"*", true, true, category, stopFlag, progressCb);
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    // QQ轻聊版/NTQQ缓存
    std::wstring qqNtCache = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\Documents\\Tencent Files\\QQNT");
    if (Utils::FileUtil::Exists(qqNtCache)) {
        ScanDirectory(qqNtCache, L"*", true, true, category, stopFlag, progressCb);
    }
}

void SoftwareCacheScanner::ScanThunderCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    // 迅雷下载缓存
    std::wstring thunderCache = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Thunder Network");
    if (Utils::FileUtil::Exists(thunderCache)) {
        // Cache目录
        WIN32_FIND_DATAW findData;
        std::wstring searchPath = thunderCache + L"\\*";
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    std::wstring dirName(findData.cFileName);
                    if (dirName == L"." || dirName == L"..") continue;

                    std::wstring cachePath = thunderCache + L"\\" + dirName + L"\\Cache";
                    if (Utils::FileUtil::Exists(cachePath)) {
                        ScanDirectory(cachePath, L"*", true, true, category, stopFlag, progressCb);
                    }

                    std::wstring tempPath = thunderCache + L"\\" + dirName + L"\\Temp";
                    if (Utils::FileUtil::Exists(tempPath)) {
                        ScanDirectory(tempPath, L"*", true, true, category, stopFlag, progressCb);
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }
}

void SoftwareCacheScanner::ScanVideoAppCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    // 爱奇艺缓存
    std::wstring qiyiCache = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Qiyi");
    if (Utils::FileUtil::Exists(qiyiCache)) {
        ScanDirectory(qiyiCache, L"*", true, true, category, stopFlag, progressCb);
    }

    // 腾讯视频缓存
    std::wstring qliveCache = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Tencent\\QLive");
    if (Utils::FileUtil::Exists(qliveCache)) {
        ScanDirectory(qliveCache, L"*", true, true, category, stopFlag, progressCb);
    }

    // 优酷缓存
    std::wstring youkuCache = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Youku");
    if (Utils::FileUtil::Exists(youkuCache)) {
        ScanDirectory(youkuCache, L"*", true, true, category, stopFlag, progressCb);
    }

    // 哔哩哔哩缓存
    std::wstring biliCache = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\bilibili");
    if (Utils::FileUtil::Exists(biliCache)) {
        ScanDirectory(biliCache, L"*", true, true, category, stopFlag, progressCb);
    }
}

void SoftwareCacheScanner::ScanWPSCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    // WPS云端缓存
    std::wstring wpsCloud = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Kingsoft\\WPS Cloud Files");
    if (Utils::FileUtil::Exists(wpsCloud)) {
        ScanDirectory(wpsCloud, L"*", true, true, category, stopFlag, progressCb);
    }

    // WPS临时文件
    std::wstring wpsTemp = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Kingsoft\\WPS Office");
    if (Utils::FileUtil::Exists(wpsTemp)) {
        ScanDirectory(wpsTemp, L"*.tmp", true, true, category, stopFlag, progressCb);
    }
}

void SoftwareCacheScanner::ScanDingTalkCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    // 钉钉缓存
    std::wstring dingtalkCache = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\DingTalk");
    if (Utils::FileUtil::Exists(dingtalkCache)) {
        ScanDirectory(dingtalkCache, L"*", true, true, category, stopFlag, progressCb);
    }

    // 钉钉文件缓存
    std::wstring dingtalkFile = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\Documents\\DingTalk");
    if (Utils::FileUtil::Exists(dingtalkFile)) {
        ScanDirectory(dingtalkFile, L"*", true, true, category, stopFlag, progressCb);
    }
}

void SoftwareCacheScanner::ScanYoudaoCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    // 有道云笔记缓存
    std::wstring youdaoCache = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Youdao\\YoudaoNote");
    if (Utils::FileUtil::Exists(youdaoCache)) {
        ScanDirectory(youdaoCache, L"*", true, true, category, stopFlag, progressCb);
    }

    // 有道词典缓存
    std::wstring youdaoDict = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Youdao\\Dict");
    if (Utils::FileUtil::Exists(youdaoDict)) {
        ScanDirectory(youdaoDict, L"*", true, true, category, stopFlag, progressCb);
    }
}

bool SoftwareCacheScanner::IsAvailable() const {
    // 检查至少一个软件缓存目录是否存在
    return Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\Documents\\WeChat Files")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\Documents\\Tencent Files")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Thunder Network")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Qiyi")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Tencent\\QLive")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Youku")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\bilibili")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Kingsoft")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\DingTalk")) ||
           Utils::FileUtil::Exists(Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%\\AppData\\Local\\Youdao"));
}

// ── Electron 应用缓存 ──

void SoftwareCacheScanner::ScanElectronAppCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring localAppData = Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%");
    std::wstring roaming = Utils::Win32Util::ExpandEnvVars(L"%APPDATA%");

    // Discord
    std::wstring discordCache = localAppData + L"\\Discord\\Cache";
    if (Utils::FileUtil::Exists(discordCache)) {
        ScanDirectory(discordCache, L"*", true, true, category, stopFlag, progressCb);
        std::wstring discordCodeCache = localAppData + L"\\Discord\\Code Cache";
        if (Utils::FileUtil::Exists(discordCodeCache)) {
            ScanDirectory(discordCodeCache, L"*", true, true, category, stopFlag, progressCb);
        }
    }

    // Slack
    std::wstring slackCache = localAppData + L"\\Slack\\Cache";
    if (Utils::FileUtil::Exists(slackCache)) {
        ScanDirectory(slackCache, L"*", true, true, category, stopFlag, progressCb);
    }
    std::wstring slackSWCache = localAppData + L"\\Slack\\Service Worker\\CacheStorage";
    if (Utils::FileUtil::Exists(slackSWCache)) {
        ScanDirectory(slackSWCache, L"*", true, true, category, stopFlag, progressCb);
    }

    // VS Code
    std::wstring vscodeCachedData = roaming + L"\\Code\\CachedData";
    if (Utils::FileUtil::Exists(vscodeCachedData)) {
        ScanDirectory(vscodeCachedData, L"*", true, true, category, stopFlag, progressCb);
    }
    std::wstring vscodeCache = roaming + L"\\Code\\Cache";
    if (Utils::FileUtil::Exists(vscodeCache)) {
        ScanDirectory(vscodeCache, L"*", true, true, category, stopFlag, progressCb);
    }

    // Microsoft Teams
    std::wstring teamsCache = localAppData + L"\\Microsoft\\Teams\\Cache";
    if (Utils::FileUtil::Exists(teamsCache)) {
        ScanDirectory(teamsCache, L"*", true, true, category, stopFlag, progressCb);
    }

    // Telegram
    std::wstring telegramCache = localAppData + L"\\Telegram Desktop\\tdata";
    if (Utils::FileUtil::Exists(telegramCache)) {
        ScanDirectory(telegramCache, L"*", true, true, category, stopFlag, progressCb);
    }

    // Postman
    std::wstring postmanCache = localAppData + L"\\Postman\\Cache";
    if (Utils::FileUtil::Exists(postmanCache)) {
        ScanDirectory(postmanCache, L"*", true, true, category, stopFlag, progressCb);
    }
}

// ── 包管理器缓存 ──

void SoftwareCacheScanner::ScanPackageManagerCache(Models::ScanCategory& category, const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    std::wstring localAppData = Utils::Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%");
    std::wstring userProfile = Utils::Win32Util::ExpandEnvVars(L"%USERPROFILE%");

    std::vector<std::wstring> cacheDirs = {
        localAppData + L"\\npm-cache",
        localAppData + L"\\NuGet\\Cache",
        localAppData + L"\\pip\\Cache",
        localAppData + L"\\Cargo\\Registry",
        localAppData + L"\\vcpkg\\downloads",
        userProfile + L"\\.gradle\\caches",
        userProfile + L"\\.m2\\repository",
    };

    for (const auto& dir : cacheDirs) {
        if (Utils::FileUtil::Exists(dir)) {
            ScanDirectory(dir, L"*", true, true, category, stopFlag, progressCb);
        }
    }
}

} // namespace IceClean::Core::Scanner
