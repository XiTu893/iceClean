#include "PopupBlocker.h"
#include "utils/RegistryUtil.h"
#include "utils/Win32Util.h"

#include <algorithm>
#include <fstream>
#include <tlhelp32.h>
#include <nlohmann/json.hpp>

namespace IceClean::Core::Safety {

using namespace IceClean::Utils;

// ── 已知弹窗广告软件进程名 ──
const std::vector<std::wstring> PopupBlocker::s_knownAdwareProcesses = {
    L"360safe.exe", L"360tray.exe", L"360sd.exe", L"ZhuDongFangYu.exe",
    L"QQPCTray.exe", L"QQPCMgr.exe", L"QQPCRTP.exe",
    L"kxetray.exe", L"KSWebShield.exe", L"knsd.exe",
    L"huohu.exe", L"2345Explorer.exe", L"2345Pic.exe", L"2345MiniPage.exe",
    L"51wlt.exe", L"wslm.exe",
    L"SogouCloud.exe", L"SogouComMgr.exe", L"SogouTS.exe",
    L"baidusd.exe", L"BaiduSdTray.exe", L"BaiduAn.exe",
    L"wpsnotify.exe",
    L"FlashPaper.exe", L"FlashPlayer.exe",
    L"MiniThunder.exe", L"Thunder.exe",
    L"DingTalk.exe", L"DingTalkUpdater.exe",
    L"YoudaoNote.exe", L"YNote.exe",
    L"WifiMaster.exe", L"FreeWifi.exe",
    L"PCManager.exe", L"PCManagerTray.exe",  // 华为电脑管家
};

// ── 扫描所有弹窗源 ──

std::vector<Models::PopupBlockerItem> PopupBlocker::ScanPopupSources() {
    std::vector<Models::PopupBlockerItem> items;

    ScanScheduledTaskPopups(items);
    ScanStartupPopups(items);
    ScanAdwarePopups(items);
    ScanBrowserNotificationPopups(items);

    return items;
}

// ── 扫描计划任务弹窗 ──

void PopupBlocker::ScanScheduledTaskPopups(std::vector<Models::PopupBlockerItem>& items) {
    // 扫描常见弹窗计划任务路径
    static const std::vector<std::wstring> popupTaskNames = {
        L"\\Microsoft\\Windows\\Setup\\GWXTriggers",
        L"\\Microsoft\\Windows\\Setup\\GWXTriggers\\refreshgwxcontent",
        L"\\Microsoft\\Windows\\Setup\\GWXTriggers\\Logon-5d",
        L"\\Microsoft\\Windows\\Setup\\GWXTriggers\\Time-5d",
        L"\\Microsoft\\Windows\\Setup\\GWXTriggers\\OutOfSleep-5d",
        L"\\Microsoft\\Windows\\Setup\\GWXTriggers\\OutOfIdle-5d",
        L"\\Microsoft\\Windows\\Application Experience\\Microsoft Compatibility Appraiser",
        L"\\Microsoft\\Windows\\Application Experience\\ProgramDataUpdater",
        L"\\Microsoft\\Windows\\Autochk\\Proxy",
        L"\\Microsoft\\Windows\\CloudExperienceHost\\CreateObjectTask",
        L"\\Microsoft\\Windows\\DiskFootprint\\Diagnostics",
        L"\\Microsoft\\Windows\\NetTrace\\GatherNetworkInfo",
        L"\\Microsoft\\Windows\\Windows Error Reporting\\QueueReporting",
    };

    for (const auto& taskPath : popupTaskNames) {
        Models::PopupBlockerItem item;
        item.sourcePath = taskPath;

        // 提取任务名
        size_t lastSlash = taskPath.rfind(L'\\');
        item.name = (lastSlash != std::wstring::npos) ? taskPath.substr(lastSlash + 1) : taskPath;
        item.type = Models::PopupType::ScheduledTask;
        item.isSystem = true;  // 这些都是系统计划任务

        // 生成描述
        if (taskPath.find(L"GWXTriggers") != std::wstring::npos) {
            item.description = L"Windows升级提示弹窗(可安全拦截)";
            item.isSystem = false;
        } else if (taskPath.find(L"Compatibility Appraiser") != std::wstring::npos) {
            item.description = L"Windows兼容性评估提示";
        } else if (taskPath.find(L"QueueReporting") != std::wstring::npos) {
            item.description = L"Windows错误报告弹窗";
            item.isSystem = false;
        } else if (taskPath.find(L"GatherNetworkInfo") != std::wstring::npos) {
            item.description = L"网络信息收集提示";
            item.isSystem = false;
        } else {
            item.description = L"系统计划任务弹窗";
        }

        items.push_back(item);
    }
}

// ── 扫描启动项弹窗 ──

void PopupBlocker::ScanStartupPopups(std::vector<Models::PopupBlockerItem>& items) {
    // 扫描当前运行进程中已知的弹窗软件
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            std::wstring processName = pe32.szExeFile;

            // 检查是否在已知弹窗软件列表中
            for (const auto& adware : s_knownAdwareProcesses) {
                if (_wcsicmp(processName.c_str(), adware.c_str()) == 0) {
                    Models::PopupBlockerItem item;
                    item.name = adware;
                    item.processName = processName;
                    item.type = Models::PopupType::AdwarePopup;
                    item.description = L"已知弹窗广告软件: " + adware;
                    item.isSystem = false;

                    // 获取进程路径
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
                    if (hProcess) {
                        wchar_t path[MAX_PATH] = {};
                        DWORD size = MAX_PATH;
                        if (QueryFullProcessImageNameW(hProcess, 0, path, &size)) {
                            item.sourcePath = path;
                        }
                        CloseHandle(hProcess);
                    }

                    items.push_back(item);
                    break;
                }
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
}

// ── 扫描广告弹窗注册表项 ──

void PopupBlocker::ScanAdwarePopups(std::vector<Models::PopupBlockerItem>& items) {
    // 扫描常见弹窗注册表位置
    static const struct {
        HKEY rootKey;
        std::wstring subKey;
        std::wstring description;
    } adwareRegKeys[] = {
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
          L"用户启动项（弹窗常驻项）" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
          L"系统启动项（弹窗常驻项）" },
        { HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
          L"一次性启动项" },
        { HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
          L"一次性启动项" },
    };

    for (const auto& regKey : adwareRegKeys) {
        auto values = RegistryUtil::EnumValues(regKey.rootKey, regKey.subKey);
        for (const auto& val : values) {
            std::wstring data = RegistryUtil::ReadStringValue(regKey.rootKey, regKey.subKey, val);

            // 检查是否为已知弹窗软件
            bool isAdware = false;
            for (const auto& adware : s_knownAdwareProcesses) {
                if (data.find(adware) != std::wstring::npos) {
                    isAdware = true;
                    break;
                }
            }

            // 检查是否包含弹窗特征
            if (!isAdware) {
                static const std::vector<std::wstring> popupKeywords = {
                    L"notify", L"popup", L"tray", L"helper", L"update",
                    L"push", L"advert", L"promo"
                };
                std::wstring lowerData = data;
                std::transform(lowerData.begin(), lowerData.end(), lowerData.begin(), ::towlower);
                std::wstring lowerVal = val;
                std::transform(lowerVal.begin(), lowerVal.end(), lowerVal.begin(), ::towlower);

                for (const auto& keyword : popupKeywords) {
                    if (lowerData.find(keyword) != std::wstring::npos ||
                        lowerVal.find(keyword) != std::wstring::npos) {
                        isAdware = true;
                        break;
                    }
                }
            }

            if (isAdware) {
                Models::PopupBlockerItem item;
                item.name = val;
                item.sourcePath = (regKey.rootKey == HKEY_CURRENT_USER ? L"HKCU\\" : L"HKLM\\") + regKey.subKey + L"\\" + val;
                item.processName = data;
                item.type = Models::PopupType::AdwarePopup;
                item.description = regKey.description + L": " + val;
                item.isSystem = false;
                items.push_back(item);
            }
        }
    }
}

// ── 扫描浏览器通知弹窗 ──

void PopupBlocker::ScanBrowserNotificationPopups(std::vector<Models::PopupBlockerItem>& items) {
    // Chrome 通知权限
    std::wstring chromeNotifPath = Win32Util::ExpandEnvVars(
        L"%LOCALAPPDATA%\\Google\\Chrome\\User Data\\Default\\Preferences");
    if (!chromeNotifPath.empty() && GetFileAttributesW(chromeNotifPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        Models::PopupBlockerItem item;
        item.name = L"Chrome 通知";
        item.sourcePath = chromeNotifPath;
        item.type = Models::PopupType::BrowserNotification;
        item.description = L"Chrome浏览器通知弹窗（可在浏览器设置中管理）";
        item.isSystem = false;
        items.push_back(item);
    }

    // Edge 通知权限
    std::wstring edgeNotifPath = Win32Util::ExpandEnvVars(
        L"%LOCALAPPDATA%\\Microsoft\\Edge\\User Data\\Default\\Preferences");
    if (!edgeNotifPath.empty() && GetFileAttributesW(edgeNotifPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        Models::PopupBlockerItem item;
        item.name = L"Edge 通知";
        item.sourcePath = edgeNotifPath;
        item.type = Models::PopupType::BrowserNotification;
        item.description = L"Edge浏览器通知弹窗（可在浏览器设置中管理）";
        item.isSystem = false;
        items.push_back(item);
    }

    // Firefox 通知权限
    std::wstring firefoxProfilePath = Win32Util::ExpandEnvVars(
        L"%APPDATA%\\Mozilla\\Firefox\\Profiles");
    if (!firefoxProfilePath.empty() && GetFileAttributesW(firefoxProfilePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        Models::PopupBlockerItem item;
        item.name = L"Firefox 通知";
        item.sourcePath = firefoxProfilePath;
        item.type = Models::PopupType::BrowserNotification;
        item.description = L"Firefox浏览器通知弹窗（可在浏览器设置中管理）";
        item.isSystem = false;
        items.push_back(item);
    }

    // Windows 通知设置
    Models::PopupBlockerItem winNotifItem;
    winNotifItem.name = L"Windows 通知";
    winNotifItem.sourcePath = L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PushNotifications";
    winNotifItem.type = Models::PopupType::SystemNotification;
    winNotifItem.description = L"Windows系统通知弹窗（可在系统设置中管理）";
    winNotifItem.isSystem = true;
    items.push_back(winNotifItem);
}

// ── 拦截弹窗 ──

bool PopupBlocker::BlockPopup(const Models::PopupBlockerItem& item) {
    switch (item.type) {
    case Models::PopupType::ScheduledTask:
        // 禁用计划任务
        {
            std::wstring cmd = L"schtasks /Change /TN \"" + item.sourcePath + L"\" /Disable";
            std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
            cmdBuf.push_back(L'\0');
            STARTUPINFOW si = { sizeof(si) };
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION pi = {};
            if (!CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr,
                                FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                return false;
            }
            WaitForSingleObject(pi.hProcess, 10000);
            DWORD exitCode = 1;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return exitCode == 0;
        }

    case Models::PopupType::AdwarePopup:
        // 从注册表删除启动项（不再直接终止进程，避免触发安全软件自我保护导致级联崩溃）
        if (item.sourcePath.find(L"HKCU\\") == 0) {
            std::wstring subKey = item.sourcePath.substr(5);
            size_t lastSlash = subKey.rfind(L'\\');
            if (lastSlash != std::wstring::npos) {
                std::wstring parentKey = subKey.substr(0, lastSlash);
                std::wstring valueName = subKey.substr(lastSlash + 1);
                HKEY hKey;
                if (RegOpenKeyExW(HKEY_CURRENT_USER, parentKey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                    bool ok = RegDeleteValueW(hKey, valueName.c_str()) == ERROR_SUCCESS;
                    RegCloseKey(hKey);
                    return ok;
                }
            }
        } else if (item.sourcePath.find(L"HKLM\\") == 0) {
            std::wstring subKey = item.sourcePath.substr(5);
            size_t lastSlash = subKey.rfind(L'\\');
            if (lastSlash != std::wstring::npos) {
                std::wstring parentKey = subKey.substr(0, lastSlash);
                std::wstring valueName = subKey.substr(lastSlash + 1);
                HKEY hKey;
                if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, parentKey.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                    bool ok = RegDeleteValueW(hKey, valueName.c_str()) == ERROR_SUCCESS;
                    RegCloseKey(hKey);
                    return ok;
                }
            }
        }
        // 不再终止进程 — 仅通过注册表操作拦截，避免触发安全软件自我保护导致级联崩溃
        // 安全软件（360、腾讯管家等）被终止后会反杀本进程
        return true;

    case Models::PopupType::BrowserNotification:
        // 浏览器通知需要通过浏览器设置管理，这里只做标记
        IncrementBlockCount(item.name);
        return true;

    case Models::PopupType::SystemNotification:
        // 禁用Windows通知推送
        {
            HKEY hKey;
            std::wstring regPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PushNotifications";
            if (RegOpenKeyExW(HKEY_CURRENT_USER, regPath.c_str(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                DWORD value = 0;
                RegSetValueExW(hKey, L"ToastEnabled", 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
                RegCloseKey(hKey);
            }
            return true;
        }

    default:
        return false;
    }
}

// ── 解除拦截 ──

bool PopupBlocker::UnblockPopup(const Models::PopupBlockerItem& item) {
    switch (item.type) {
    case Models::PopupType::ScheduledTask:
        // 启用计划任务
        {
            std::wstring cmd = L"schtasks /Change /TN \"" + item.sourcePath + L"\" /Enable";
            std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
            cmdBuf.push_back(L'\0');
            STARTUPINFOW si = { sizeof(si) };
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_HIDE;
            PROCESS_INFORMATION pi = {};
            if (!CreateProcessW(nullptr, cmdBuf.data(), nullptr, nullptr,
                                FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                return false;
            }
            WaitForSingleObject(pi.hProcess, 10000);
            DWORD exitCode = 1;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return exitCode == 0;
        }

    case Models::PopupType::SystemNotification:
        // 启用Windows通知推送
        {
            HKEY hKey;
            std::wstring regPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PushNotifications";
            if (RegOpenKeyExW(HKEY_CURRENT_USER, regPath.c_str(), 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                DWORD value = 1;
                RegSetValueExW(hKey, L"ToastEnabled", 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
                RegCloseKey(hKey);
            }
            return true;
        }

    default:
        return false;
    }
}

// ── 拦截统计 ──

Models::PopupBlockerStats PopupBlocker::GetStats() {
    Models::PopupBlockerStats stats;
    stats.isEnabled = IsBlockerEnabled();

    // 读取配置文件中的统计
    auto configPath = GetConfigFilePath();
    std::ifstream ifs(configPath);
    if (ifs.is_open()) {
        try {
            nlohmann::json j;
            ifs >> j;
            stats.totalBlocked = j.value("totalBlocked", 0);
            stats.todayBlocked = j.value("todayBlocked", 0);
            stats.rulesCount = j.value("rulesCount", 0);
        } catch (...) {}
    }

    return stats;
}

void PopupBlocker::IncrementBlockCount(const std::wstring& name) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    auto configPath = GetConfigFilePath();
    nlohmann::json j;

    std::ifstream ifs(configPath);
    if (ifs.is_open()) {
        try { ifs >> j; } catch (...) {}
    }

    j["totalBlocked"] = j.value("totalBlocked", 0) + 1;
    j["todayBlocked"] = j.value("todayBlocked", 0) + 1;
    j["rulesCount"] = j.value("rulesCount", 0);

    std::ofstream ofs(configPath);
    if (ofs.is_open()) {
        ofs << j.dump(2);
    }
}

// ── 辅助方法 ──

std::wstring PopupBlocker::GetPopupTypeName(Models::PopupType type) {
    switch (type) {
    case Models::PopupType::ScheduledTask:      return L"计划任务弹窗";
    case Models::PopupType::StartupPopup:        return L"启动项弹窗";
    case Models::PopupType::BrowserNotification: return L"浏览器通知";
    case Models::PopupType::AdwarePopup:         return L"广告弹窗";
    case Models::PopupType::SystemNotification:  return L"系统通知";
    default:                                     return L"其他";
    }
}

bool PopupBlocker::IsBlockerEnabled() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                       L"SOFTWARE\\IceClean\\PopupBlocker",
                       0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value = 1;
        DWORD size = sizeof(value);
        if (RegQueryValueExW(hKey, L"Enabled", nullptr, nullptr, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return value != 0;
        }
        RegCloseKey(hKey);
    }
    return true;  // 默认启用
}

void PopupBlocker::SetBlockerEnabled(bool enabled) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                         L"SOFTWARE\\IceClean\\PopupBlocker",
                         0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        DWORD value = enabled ? 1 : 0;
        RegSetValueExW(hKey, L"Enabled", 0, REG_DWORD, (const BYTE*)&value, sizeof(value));
        RegCloseKey(hKey);
    }
}

bool PopupBlocker::IsSystemPopup(const std::wstring& name, const std::wstring& path) const {
    // 系统关键进程和安全软件不应拦截
    static const std::vector<std::wstring> systemProcesses = {
        // 系统进程
        L"explorer.exe", L"svchost.exe", L"csrss.exe", L"lsass.exe",
        L"services.exe", L"winlogon.exe", L"dwm.exe", L"taskhostw.exe",
        L"RuntimeBroker.exe", L"SearchUI.exe", L"ShellExperienceHost.exe",
        L"SecurityHealthService.exe", L"SecurityHealthSystray.exe",
        // 安全软件进程 — 终止这些会触发自我保护导致级联崩溃
        L"360safe.exe", L"360tray.exe", L"ZhuDongFangYu.exe", L"360sd.exe",
        L"360rp.exe", L"360leakfixer.exe", L"QQPCTray.exe", L"QQPCMgr.exe",
        L"QQPCRTP.exe", L"kxetray.exe", L"KSWebShield.exe", L"knsdtray.exe",
        L"huorong.exe", L"wsctrl.exe", L"usysdiag.exe", L"hipstray.exe",
        L"avp.exe", L"kavfs.exe", L"McAfee.exe", L"mbam.exe",
        L"MsMpEng.exe", L"SecurityHealth.exe",
    };

    for (const auto& sys : systemProcesses) {
        if (_wcsicmp(name.c_str(), sys.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

std::wstring PopupBlocker::GetConfigFilePath() {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::wstring dir(path);
    size_t lastSlash = dir.rfind(L'\\');
    if (lastSlash != std::wstring::npos) {
        dir = dir.substr(0, lastSlash);
    }
    return dir + L"\\popup_blocker_config.json";
}

} // namespace IceClean::Core::Safety
