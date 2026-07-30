#include "SoftwareUpdateChecker.h"
#include "core/cleaner/SoftwareUninstaller.h"
#include "utils/Win32Util.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <shlobj.h>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace IceClean::Core::Analyzer {

using json = nlohmann::json;

std::vector<UpdatableSoftware> SoftwareUpdateChecker::CheckUpdates() {
    std::vector<UpdatableSoftware> updates;

    // 获取已安装软件列表
    IceClean::Core::Cleaner::SoftwareUninstaller uninstaller;
    auto installed = uninstaller.GetInstalledSoftware();

    // 加载已知软件版本信息
    auto knownVersions = LoadKnownVersions();

    // 比对已安装软件与已知最新版本
    for (const auto& software : installed) {
        if (software.isSystemComponent || software.isUpdate) continue;
        if (software.version.empty()) continue;

        for (const auto& known : knownVersions) {
            // 按名称匹配（模糊匹配，包含即可）
            if (software.displayName.find(known.name) != std::wstring::npos ||
                known.name.find(software.displayName) != std::wstring::npos) {
                // 比较版本号
                int cmp = CompareVersions(software.version, known.latestVersion);
                if (cmp < 0) {
                    UpdatableSoftware update;
                    update.name = software.displayName;
                    update.currentVersion = software.version;
                    update.latestVersion = known.latestVersion;
                    update.downloadUrl = known.downloadUrl;
                    update.publisher = software.publisher;
                    updates.push_back(update);
                }
                break;
            }
        }
    }

    return updates;
}

std::vector<UpdatableSoftware> SoftwareUpdateChecker::LoadKnownVersions() {
    std::vector<UpdatableSoftware> versions;

    // 内置常用软件版本信息（后续可从在线获取）
    // 格式: name, latestVersion, downloadUrl
    const UpdatableSoftware knownSoftware[] = {
        { L"微信", L"3.9.12", L"https://pc.weixin.qq.com/", L"Tencent" },
        { L"QQ", L"9.9.15", L"https://im.qq.com/pcqq/index.shtml", L"Tencent" },
        { L"Chrome", L"126.0", L"https://www.google.com/chrome/", L"Google" },
        { L"Firefox", L"127.0", L"https://www.mozilla.org/firefox/", L"Mozilla" },
        { L"7-Zip", L"24.08", L"https://www.7-zip.org/", L"Igor Pavlov" },
        { L"VSCode", L"1.90", L"https://code.visualstudio.com/", L"Microsoft" },
        { L"VLC", L"3.0.21", L"https://www.videolan.org/", L"VideoLAN" },
        { L"Notepad++", L"8.6.8", L"https://notepad-plus-plus.org/", L"Don Ho" },
        { L"WinRAR", L"7.01", L"https://www.win-rar.com/", L"Alexander L. Roshal" },
        { L"PotPlayer", L"1.7.22077", L"https://potplayer.daum.net/", L"Kakao" },
        { L"WPS Office", L"12.2", L"https://www.wps.cn/", L"Kingsoft" },
        { L"钉钉", L"7.6", L"https://www.dingtalk.com/", L"Alibaba" },
        { L"迅雷", L"12.1", L"https://www.xunlei.com/", L"Thunder Networking" },
        { L"网易云音乐", L"3.0", L"https://music.163.com/", L"NetEase" },
        { L"Steam", L"2.10.91", L"https://store.steampowered.com/", L"Valve" },
    };

    for (const auto& sw : knownSoftware) {
        versions.push_back(sw);
    }

    // 尝试从本地JSON配置加载额外版本信息
    wchar_t appDataPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        auto configPath = std::wstring(appDataPath) + L"\\IceClean\\software_versions.json";
        try {
            std::ifstream file(configPath);
            if (file.is_open()) {
                json j;
                file >> j;
                if (j.is_array()) {
                    for (const auto& item : j) {
                        UpdatableSoftware sw;
                        if (item.contains("name")) sw.name = std::wstring(item["name"].get<std::string>().begin(), item["name"].get<std::string>().end());
                        if (item.contains("latestVersion")) sw.latestVersion = std::wstring(item["latestVersion"].get<std::string>().begin(), item["latestVersion"].get<std::string>().end());
                        if (item.contains("downloadUrl")) sw.downloadUrl = std::wstring(item["downloadUrl"].get<std::string>().begin(), item["downloadUrl"].get<std::string>().end());
                        if (item.contains("publisher")) sw.publisher = std::wstring(item["publisher"].get<std::string>().begin(), item["publisher"].get<std::string>().end());
                        if (!sw.name.empty() && !sw.latestVersion.empty()) {
                            versions.push_back(sw);
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            spdlog::debug("加载软件版本配置失败: {}", e.what());
        }
    }

    return versions;
}

int SoftwareUpdateChecker::CompareVersions(const std::wstring& v1, const std::wstring& v2) {
    // 将版本号按'.'分割为数字段进行比较
    auto splitVersion = [](const std::wstring& v) -> std::vector<int> {
        std::vector<int> parts;
        std::wstring num;
        for (wchar_t c : v) {
            if (c == L'.') {
                if (!num.empty()) {
                    try { parts.push_back(std::stoi(num)); } catch (...) {}
                    num.clear();
                }
            } else if (c >= L'0' && c <= L'9') {
                num += c;
            }
        }
        if (!num.empty()) {
            try { parts.push_back(std::stoi(num)); } catch (...) {}
        }
        return parts;
    };

    auto p1 = splitVersion(v1);
    auto p2 = splitVersion(v2);

    size_t maxLen = (std::max)(p1.size(), p2.size());
    for (size_t i = 0; i < maxLen; ++i) {
        int n1 = (i < p1.size()) ? p1[i] : 0;
        int n2 = (i < p2.size()) ? p2[i] : 0;
        if (n1 < n2) return -1;
        if (n1 > n2) return 1;
    }
    return 0;
}

} // namespace IceClean::Core::Analyzer
