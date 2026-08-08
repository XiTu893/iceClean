#include "WindowsDebloater.h"
#include "utils/RegistryUtil.h"
#include "utils/Win32Util.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sstream>

namespace IceClean::Core::Optimizer {

using namespace IceClean::Utils;
using namespace IceClean::Models;

WindowsDebloater::WindowsDebloater() {
    InitializeItems();
}

void WindowsDebloater::InitializeItems() {
    // ── AppxPackage 移除 ──
    m_items.push_back({ L"appx-xbox", L"Xbox 全家桶", L"移除 Xbox 应用、游戏条、Game DVR 等", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.Xbox*", L"", true, true, false, L"可通过 Microsoft Store 重新安装" });
    m_items.push_back({ L"appx-skype", L"Skype", L"移除预装 Skype 应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.SkypeApp", L"", true, true, false, L"可通过 Microsoft Store 重新安装" });
    m_items.push_back({ L"appx-onedrive", L"OneDrive", L"移除 OneDrive 集成", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.OneDriveSync", L"", true, false, false, L"可通过 Microsoft Store 重新安装" });
    m_items.push_back({ L"appx-maps", L"Windows 地图", L"移除预装地图应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.WindowsMaps", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-people", L"人脉", L"移除人脉应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.People", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-news", L"新闻", L"移除 Microsoft News 应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.BingNews", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-weather", L"天气", L"移除天气应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.BingWeather", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-solitaire", L"Microsoft Solitaire", L"移除纸牌游戏合集", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.MicrosoftSolitaireCollection", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-3dviewer", L"3D Viewer", L"移除 3D 查看器", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.Microsoft3DViewer", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-officehub", L"Office Hub", L"移除 Office 快捷入口应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.MicrosoftOfficeHub", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-powerautomate", L"Power Automate", L"移除 Power Automate 应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.PowerAutomateDesktop", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-mixedreality", L"Mixed Reality Portal", L"移除混合现实门户", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.MixedReality.Portal", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-onenote", L"OneNote for Windows 10", L"移除预装 OneNote", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.Office.OneNote", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-paint3d", L"画图 3D", L"移除画图 3D 应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.MSPaint", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-print3d", L"3D 打印", L"移除 3D 打印应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.Print3D", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-skypeorchid", L"Skype 视频", L"移除 Skype 视频应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.SkypeOrchid", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-storepurchase", L"商店购买体验", L"移除商店购买体验应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.StorePurchaseApp", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-wallet", L"钱包", L"移除钱包应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.Wallet", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-whiteboard", L"Whiteboard", L"移除 Microsoft Whiteboard", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.Whiteboard", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-windowsalarms", L"Windows 闹钟", L"移除闹钟和时钟应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.WindowsAlarms", L"", true, false, false, L"" });
    m_items.push_back({ L"appx-windowscalculator", L"Windows 计算器", L"移除计算器应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.WindowsCalculator", L"", false, true, false, L"推荐保留" });
    m_items.push_back({ L"appx-windowscamera", L"Windows 相机", L"移除相机应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.WindowsCamera", L"", false, true, false, L"推荐保留" });
    m_items.push_back({ L"appx-remotedesktop", L"远程桌面", L"移除远程桌面应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.RemoteDesktop", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-feedback", L"Feedback Hub", L"移除反馈中心", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.WindowsFeedbackHub", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-getstarted", L"提示", L"移除入门提示应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.GetStarted", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-tips", L"Windows 使用技巧", L"移除使用技巧应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.WindowsTips", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-copilot", L"Microsoft Copilot", L"移除 Copilot AI 助手", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.Copilot*", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-clipchamp", L"Clipchamp 视频编辑器", L"移除 Clipchamp 视频编辑器", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.Clipchamp", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-devhome", L"Dev Home", L"移除 Dev Home 开发者工具", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.DevHome*", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-adobeexpress", L"Adobe Express", L"移除 Adobe Express 应用", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Adobe.AdobeExpress", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-spotify", L"Spotify 音乐", L"移除预装 Spotify", DebloatCategory::AppxPackage, DebloatAction::Remove, L"SpotifyAB.SpotifyMusic", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-tiktok", L"TikTok", L"移除预装 TikTok", DebloatCategory::AppxPackage, DebloatAction::Remove, L"ByteDance.TikTok*", L"", true, true, false, L"" });
    m_items.push_back({ L"appx-widgets", L"小组件 (Widgets)", L"移除 Windows 小组件面板", DebloatCategory::AppxPackage, DebloatAction::Remove, L"Microsoft.WindowsWidgets*", L"", true, false, false, L"" });

    // ── 遥测与隐私 ──
    m_items.push_back({ L"telemetry-general", L"遥测数据收集", L"禁用遥测数据发送到 Microsoft", DebloatCategory::Telemetry, DebloatAction::Disable, L"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"AllowTelemetry", true, true, true, L"设置 AllowTelemetry=0" });
    m_items.push_back({ L"telemetry-compat", L"兼容性遥测", L"禁用兼容性遥测 (UtcOnPolicyRefresh)", DebloatCategory::Telemetry, DebloatAction::Disable, L"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"DisableTelemetryOptin", true, true, false, L"" });
    m_items.push_back({ L"telemetry-census", L"Census 数据收集", L"禁用 Census 数据收集", DebloatCategory::Telemetry, DebloatAction::Disable, L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection", L"AllowCensus", true, true, false, L"" });
    m_items.push_back({ L"telemetry-connecteduser", L"Connected User Experiences", L"禁用用户体验改善计划", DebloatCategory::Telemetry, DebloatAction::Disable, L"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\DataCollection", L"AllowConnectedUser", true, true, false, L"" });

    // ── Cortana/AI ──
    m_items.push_back({ L"cortana", L"Cortana", L"禁用 Cortana 语音助手", DebloatCategory::SystemComponent, DebloatAction::Disable, L"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"AllowCortana", true, true, true, L"" });
    m_items.push_back({ L"recall", L"Recall (快照)", L"禁用 Windows Recall 快照功能", DebloatCategory::SystemComponent, DebloatAction::Disable, L"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsAI", L"DisableAIDataCollection", true, true, true, L"Windows 24H2+" });
    m_items.push_back({ L"websearch", L"Web 搜索", L"禁止在搜索中显示 Web 结果", DebloatCategory::SystemComponent, DebloatAction::Disable, L"HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\Windows Search", L"DisableWebSearch", true, true, false, L"" });
    m_items.push_back({ L"bingsearch", L"Bing 搜索集成", L"禁用 Bing 搜索集成", DebloatCategory::SystemComponent, DebloatAction::Disable, L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search", L"BingSearchEnabled", true, true, false, L"" });
    m_items.push_back({ L"cloudsearch", L"云端内容搜索", L"禁止搜索中显示云端内容", DebloatCategory::SystemComponent, DebloatAction::Disable, L"HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Search", L"CloudSearchEnabled", true, true, false, L"" });

    // ── 服务禁用 ──
    m_items.push_back({ L"svc-wsearch", L"Windows Search", L"禁用 Windows 搜索索引服务以提升性能", DebloatCategory::Service, DebloatAction::Disable, L"WSearch", L"", false, true, false, L"搜索将变慢" });
    m_items.push_back({ L"svc-sysmain", L"SysMain (Superfetch)", L"禁用 SysMain 服务（老用户提升性能）", DebloatCategory::Service, DebloatAction::Disable, L"SysMain", L"", false, true, true, L"SSD 用户通常不需要" });
    m_items.push_back({ L"svc-wprinter", L"Windows 打印机工作流", L"禁用打印机相关服务（无打印机可关闭）", DebloatCategory::Service, DebloatAction::Disable, L"PrintWorkflow", L"", true, true, false, L"无打印机可安全禁用" });
    m_items.push_back({ L"svc-xboxgip", L"Xbox 外设管理", L"禁用 Xbox 手柄/外设服务", DebloatCategory::Service, DebloatAction::Disable, L"XboxGipSvc", L"", true, true, false, L"" });
    m_items.push_back({ L"svc-xboxnet", L"Xbox 网络", L"禁用 Xbox Live 网络服务", DebloatCategory::Service, DebloatAction::Disable, L"XboxNetApiSvc", L"", true, true, false, L"" });
    m_items.push_back({ L"svc-diagtrack", L"Diagnostic Tracking", L"禁用 Microsoft 诊断跟踪服务", DebloatCategory::Service, DebloatAction::Disable, L"DiagTrack", L"", true, true, false, L"" });
    m_items.push_back({ L"svc-wcmpts", L"WCM Performance Counters", L"禁用 WCM 性能计数器服务", DebloatCategory::Service, DebloatAction::Disable, L"Wcmsvc", L"", false, true, true, L"可能影响网络状态显示" });

    // ── 计划任务 ──
    m_items.push_back({ L"task-consolidator", L"Consolidator 任务", L"禁用遥测 Consolidator 计划任务", DebloatCategory::ScheduledTask, DebloatAction::Disable, L"\\Microsoft\\Windows\\Application Experience\\Microsoft Compatibility Appraiser", L"", true, true, false, L"" });
    m_items.push_back({ L"task-telemetry", L"遥测计划任务", L"禁用各类遥测计划任务", DebloatCategory::ScheduledTask, DebloatAction::Disable, L"\\Microsoft\\Windows\\Application Experience\\ProgramDataUpdater", L"", true, true, false, L"" });

    // ── 右键菜单 ──
    m_items.push_back({ L"ctx-share", L"右键分享菜单", L"删除右键菜单中的「共享」选项", DebloatCategory::ContextMenu, DebloatAction::Disable, L"HKCR\\*\\(Default)", L"", true, true, false, L"" });
    m_items.push_back({ L"ctx-editwith", L"右键使用画图编辑", L"删除图片文件右键的「编辑」关联画图", DebloatCategory::ContextMenu, DebloatAction::Disable, L"HKCR\\SystemFileAssociations\\image\\shell\\edit", L"ProgrammaticAccessOnly", true, true, false, L"" });
    m_items.push_back({ L"ctx-classicmenu", L"经典右键菜单", L"恢复 Windows 10 经典右键菜单（Win11）", DebloatCategory::ContextMenu, DebloatAction::Modify, L"HKCU\\Software\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\\InprocServer32", L"(default)", true, false, true, L"Win11 恢复经典菜单" });

    // ── 注册表优化 ──
    m_items.push_back({ L"tweak-anim", L"禁用动画效果", L"禁用窗口动画、淡入淡出等视觉效果", DebloatCategory::RegistryTweak, DebloatAction::Modify, L"HKCU\\Control Panel\\Desktop\\WindowMetrics", L"MinAnimate", true, false, false, L"" });
    m_items.push_back({ L"tweak-taskbar", L"任务栏对齐(左)", L"将任务栏图标改为左对齐（Win11）", DebloatCategory::RegistryTweak, DebloatAction::Modify, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarAl", true, false, false, L"" });
    m_items.push_back({ L"tweak-taskbarend", L"任务栏显示结束任务", L"在任务栏右键菜单添加「结束任务」选项", DebloatCategory::RegistryTweak, DebloatAction::Modify, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced\\TaskbarDeveloperSettings", L"TaskbarEndTask", true, false, false, L"" });
    m_items.push_back({ L"tweak-ext", L"显示文件扩展名", L"在文件管理器中默认显示文件扩展名", DebloatCategory::RegistryTweak, DebloatAction::Modify, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HideFileExt", true, false, false, L"" });
    m_items.push_back({ L"tweak-hidden", L"显示隐藏文件", L"在文件管理器中显示隐藏文件", DebloatCategory::RegistryTweak, DebloatAction::Modify, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"Hidden", true, false, false, L"" });
    m_items.push_back({ L"tweak-startupdelay", L"禁用启动延迟", L"禁用 Windows 启动时对应用的延迟启动", DebloatCategory::RegistryTweak, DebloatAction::Modify, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Serialize", L"StartupDelayInMSec", true, false, true, L"" });
    m_items.push_back({ L"tweak-autoplay", L"禁用自动播放", L"禁用 USB/光盘自动播放功能", DebloatCategory::RegistryTweak, DebloatAction::Modify, L"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoplayHandlers", L"DisableAutoplay", true, false, false, L"" });
}

std::vector<Models::DebloatItem> WindowsDebloater::GetDebloatItems() const {
    return m_items;
}

std::vector<Models::DebloatPreset> WindowsDebloater::GetPresets() const {
    std::vector<Models::DebloatPreset> presets;

    presets.push_back({
        L"推荐精简 (Recommended)",
        L"安全精简项，移除大多数预装应用和广告组件，适合大多数用户",
        {
            L"appx-xbox", L"appx-skype", L"appx-maps", L"appx-people", L"appx-news",
            L"appx-weather", L"appx-solitaire", L"appx-3dviewer", L"appx-officehub",
            L"appx-mixedreality", L"appx-paint3d", L"appx-print3d", L"appx-skypeorchid",
            L"appx-storepurchase", L"appx-wallet", L"appx-whiteboard", L"appx-feedback",
            L"appx-getstarted", L"appx-onenote", L"appx-tips", L"appx-copilot",
            L"appx-clipchamp", L"appx-widgets", L"appx-powerautomate",
            L"telemetry-general", L"telemetry-compat",
            L"cortana", L"recall", L"websearch", L"bingsearch",
            L"svc-xboxgip", L"svc-xboxnet", L"svc-diagtrack",
            L"ctx-classicmenu", L"tweak-ext", L"tweak-hidden", L"tweak-taskbarend"
        }
    });

    presets.push_back({
        L"完全精简 (Minimal)",
        L"最大程度精简，移除更多组件和服务。注意部分功能可能受影响",
        {
            L"appx-xbox", L"appx-skype", L"appx-onedrive", L"appx-maps", L"appx-people",
            L"appx-news", L"appx-weather", L"appx-solitaire", L"appx-3dviewer",
            L"appx-officehub", L"appx-powerautomate", L"appx-mixedreality", L"appx-onenote",
            L"appx-paint3d", L"appx-print3d", L"appx-skypeorchid", L"appx-storepurchase",
            L"appx-wallet", L"appx-whiteboard", L"appx-windowsalarms", L"appx-feedback",
            L"appx-getstarted", L"appx-tips", L"appx-copilot", L"appx-clipchamp",
            L"appx-devhome", L"appx-adobeexpress", L"appx-spotify", L"appx-tiktok",
            L"appx-widgets", L"appx-remotedesktop",
            L"telemetry-general", L"telemetry-compat", L"telemetry-census", L"telemetry-connecteduser",
            L"cortana", L"recall", L"websearch", L"bingsearch", L"cloudsearch",
            L"svc-wsearch", L"svc-xboxgip", L"svc-xboxnet", L"svc-diagtrack", L"svc-wprinter",
            L"task-consolidator", L"task-telemetry",
            L"ctx-classicmenu", L"ctx-share",
            L"tweak-anim", L"tweak-taskbar", L"tweak-taskbarend",
            L"tweak-ext", L"tweak-hidden", L"tweak-startupdelay", L"tweak-autoplay"
        }
    });

    return presets;
}

bool WindowsDebloater::ApplyItem(const Models::DebloatItem& item) {
    switch (item.category) {
    case DebloatCategory::AppxPackage:
        return ApplyAppxPackage(item);
    case DebloatCategory::SystemComponent:
    case DebloatCategory::Telemetry:
        return ApplyTelemetry(item);
    case DebloatCategory::Service:
        return ApplyService(item);
    case DebloatCategory::ScheduledTask:
        return ApplyScheduledTask(item);
    case DebloatCategory::ContextMenu:
        return ApplyContextMenu(item);
    case DebloatCategory::RegistryTweak:
        return ApplyRegistryTweak(item);
    }
    return false;
}

Models::DebloatResult WindowsDebloater::ApplyItems(
    const std::vector<Models::DebloatItem>& items,
    std::function<void(int, int)> progressCallback,
    const std::atomic<bool>* cancelFlag)
{
    Models::DebloatResult result;
    int total = static_cast<int>(items.size());
    int current = 0;

    for (const auto& item : items) {
        if (cancelFlag && *cancelFlag) break;

        if (progressCallback) {
            progressCallback(current, total);
        }

        if (ApplyItem(item)) {
            result.succeeded++;
        } else {
            result.failed++;
            result.failedItems.push_back(item.name);
        }
        current++;
    }

    result.success = result.failed == 0;
    return result;
}

bool WindowsDebloater::RevertItem(const Models::DebloatItem& item) {
    spdlog::info("WindowsDebloater: 还原操作尚未实现: {}",
                 std::string(item.name.begin(), item.name.end()));
    return false;
}

std::vector<std::wstring> WindowsDebloater::GetInstalledAppxPackages() const {
    std::vector<std::wstring> packages;

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return packages;

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};
    std::wstring cmd = L"powershell -NoProfile -Command \"Get-AppxPackage | Select-Object -ExpandProperty PackageFullName\"";

    if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, TRUE,
                         CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return packages;
    }

    CloseHandle(hWritePipe);

    char buffer[8192] = {};
    DWORD bytesRead = 0;
    std::string output;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, 30000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::wistringstream wiss(std::wstring(output.begin(), output.end()));
    std::wstring line;
    while (std::getline(wiss, line)) {
        if (!line.empty() && line.find(L'\n') == std::wstring::npos) {
            size_t start = line.find_first_not_of(L"\r\n\t ");
            size_t end = line.find_last_not_of(L"\r\n\t ");
            if (start != std::wstring::npos && end != std::wstring::npos) {
                packages.push_back(line.substr(start, end - start + 1));
            }
        }
    }

    return packages;
}

bool WindowsDebloater::RemoveAppxPackage(const std::wstring& packageName) const {
    std::wstring cmd = L"powershell -NoProfile -Command \"Get-AppxPackage -AllUsers '" +
                       packageName + L"' | Remove-AppxPackage -AllUsers\"";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE,
                         CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 30000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

bool WindowsDebloater::ApplyAppxPackage(const Models::DebloatItem& item) const {
    return RemoveAppxPackage(item.target);
}

bool WindowsDebloater::ApplyTelemetry(const Models::DebloatItem& item) const {
    // item.target 为注册表路径 (如 "HKLM\\SOFTWARE\\...")
    std::wstring path = item.target;
    HKEY rootKey = HKEY_LOCAL_MACHINE;
    if (path.find(L"HKCU\\") == 0) {
        rootKey = HKEY_CURRENT_USER;
        path = path.substr(5);
    } else if (path.find(L"HKLM\\") == 0) {
        rootKey = HKEY_LOCAL_MACHINE;
        path = path.substr(5);
    }

    // 创建键
    HKEY hKey;
    if (RegCreateKeyExW(rootKey, path.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    DWORD value = 0;
    if (item.value == L"AllowTelemetry") value = 0;
    else if (item.value == L"DisableTelemetryOptin") value = 1;
    else if (item.value == L"AllowCensus") value = 0;
    else if (item.value == L"AllowConnectedUser") value = 0;
    else if (item.value == L"AllowCortana") value = 0;
    else if (item.value == L"DisableAIDataCollection") value = 1;
    else if (item.value == L"DisableWebSearch") value = 1;
    else if (item.value == L"BingSearchEnabled") value = 0;
    else if (item.value == L"CloudSearchEnabled") value = 0;
    else value = 0;

    bool ok = RegSetValueExW(hKey, item.value.c_str(), 0, REG_DWORD,
                              (const BYTE*)&value, sizeof(value)) == ERROR_SUCCESS;
    RegCloseKey(hKey);
    return ok;
}

bool WindowsDebloater::ApplyService(const Models::DebloatItem& item) const {
    SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!hSCM) return false;

    SC_HANDLE hService = OpenServiceW(hSCM, item.target.c_str(), SERVICE_CHANGE_CONFIG | SERVICE_STOP);
    if (!hService) {
        CloseServiceHandle(hSCM);
        return false;
    }

    bool ok = ChangeServiceConfigW(hService, SERVICE_NO_CHANGE, SERVICE_DISABLED,
                                    SERVICE_NO_CHANGE, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (ok) {
        SERVICE_STATUS status;
        ControlService(hService, SERVICE_CONTROL_STOP, &status);
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCM);
    return ok;
}

bool WindowsDebloater::ApplyScheduledTask(const Models::DebloatItem& item) const {
    std::wstring cmd = L"schtasks /Change /TN \"" + item.target + L"\" /DISABLE";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE,
                         CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 10000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

bool WindowsDebloater::ApplyContextMenu(const Models::DebloatItem& item) const {
    if (item.id == L"ctx-classicmenu") {
        HKEY hKey;
        LSTATUS status = RegCreateKeyExW(HKEY_CURRENT_USER,
            L"Software\\Classes\\CLSID\\{86ca1aa0-34aa-4e8b-a509-50c905bae2a2}\\InprocServer32",
            0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
        if (status != ERROR_SUCCESS) return false;

        const wchar_t empty[] = L"";
        status = RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)empty, sizeof(empty));
        RegCloseKey(hKey);
        return status == ERROR_SUCCESS;
    }
    return false;
}

bool WindowsDebloater::ApplyRegistryTweak(const Models::DebloatItem& item) const {
    // item.target 为注册表键路径，item.value 为值名
    std::wstring subKey = item.target;
    HKEY rootKey = HKEY_LOCAL_MACHINE;
    if (subKey.find(L"HKCU\\") == 0) {
        rootKey = HKEY_CURRENT_USER;
        subKey = subKey.substr(5);
    } else if (subKey.find(L"HKLM\\") == 0) {
        rootKey = HKEY_LOCAL_MACHINE;
        subKey = subKey.substr(5);
    } else {
        rootKey = HKEY_CURRENT_USER;
    }

    const std::wstring& valName = item.value;

    HKEY hKey;
    if (RegCreateKeyExW(rootKey, subKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    bool ok = false;
    if (item.id == L"tweak-taskbar") {
        DWORD val = 0;
        ok = RegSetValueExW(hKey, valName.c_str(), 0, REG_DWORD, (const BYTE*)&val, sizeof(val)) == ERROR_SUCCESS;
    } else if (item.id == L"tweak-taskbarend") {
        DWORD val = 1;
        ok = RegSetValueExW(hKey, valName.c_str(), 0, REG_DWORD, (const BYTE*)&val, sizeof(val)) == ERROR_SUCCESS;
    } else if (item.id == L"tweak-ext") {
        DWORD val = 0;
        ok = RegSetValueExW(hKey, valName.c_str(), 0, REG_DWORD, (const BYTE*)&val, sizeof(val)) == ERROR_SUCCESS;
    } else if (item.id == L"tweak-hidden") {
        DWORD val = 1;
        ok = RegSetValueExW(hKey, valName.c_str(), 0, REG_DWORD, (const BYTE*)&val, sizeof(val)) == ERROR_SUCCESS;
    } else if (item.id == L"tweak-anim") {
        // MinAnimate=0 to disable
        std::wstring val = L"0";
        ok = RegSetValueExW(hKey, valName.c_str(), 0, REG_SZ, (const BYTE*)val.c_str(),
                             static_cast<DWORD>((val.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS;
    } else if (item.id == L"tweak-autoplay") {
        DWORD val = 1;
        ok = RegSetValueExW(hKey, valName.c_str(), 0, REG_DWORD, (const BYTE*)&val, sizeof(val)) == ERROR_SUCCESS;
    } else if (item.id == L"tweak-startupdelay") {
        DWORD val = 0;
        ok = RegSetValueExW(hKey, valName.c_str(), 0, REG_DWORD, (const BYTE*)&val, sizeof(val)) == ERROR_SUCCESS;
    }

    RegCloseKey(hKey);
    return ok;
}

} // namespace IceClean::Core::Optimizer
