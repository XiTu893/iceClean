#include "SoftwareUninstaller.h"
#include "utils/RegistryUtil.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <tlhelp32.h>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace IceClean::Core::Cleaner {

using namespace IceClean::Models;
using namespace IceClean::Utils;

std::vector<InstalledSoftware> SoftwareUninstaller::GetInstalledSoftware() {
    std::vector<InstalledSoftware> items;

    // 读取 HKLM 下的卸载项
    ReadUninstallEntries(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", items);

    // 读取 HKLM WOW6432Node 下的卸载项（32位程序）
    ReadUninstallEntries(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall", items);

    // 读取 HKCU 下的卸载项（用户级安装）
    ReadUninstallEntries(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", items);

    // 按名称排序
    std::sort(items.begin(), items.end(),
        [](const InstalledSoftware& a, const InstalledSoftware& b) {
            return _wcsicmp(a.displayName.c_str(), b.displayName.c_str()) < 0;
        });

    return items;
}

void SoftwareUninstaller::ReadUninstallEntries(HKEY rootKey, const std::wstring& subKey,
                                                 std::vector<InstalledSoftware>& items) {
    auto subKeys = RegistryUtil::EnumSubKeys(rootKey, subKey);

    for (const auto& childKey : subKeys) {
        std::wstring fullKeyPath = subKey + L"\\" + childKey;

        // 跳过系统组件
        if (IsSystemComponent(rootKey, fullKeyPath)) {
            continue;
        }

        // 读取显示名称
        std::wstring displayName = RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"DisplayName");
        if (displayName.empty()) {
            continue; // 没有显示名称的项跳过
        }

        // 跳过Windows更新
        if (IsWindowsUpdate(displayName)) {
            continue;
        }

        InstalledSoftware item;
        item.rootKey = rootKey;
        item.displayName = displayName;
        item.registryKeyPath = fullKeyPath;

        // 读取详细信息
        item.publisher = RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"Publisher");
        item.version = RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"DisplayVersion");
        item.installDate = FormatInstallDate(
            RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"InstallDate"));
        item.installLocation = RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"InstallLocation");
        item.uninstallString = RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"UninstallString");
        item.modifyPath = RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"ModifyPath");

        // 读取估计大小
        DWORD sizeDword = RegistryUtil::ReadDwordValue(rootKey, fullKeyPath, L"EstimatedSize");
        if (sizeDword > 0) {
            item.estimatedSize = static_cast<uint64_t>(sizeDword) * 1024; // KB -> Bytes
        }

        // 读取系统组件标记
        DWORD systemComponent = RegistryUtil::ReadDwordValue(rootKey, fullKeyPath, L"SystemComponent");
        item.isSystemComponent = (systemComponent == 1);

        // 标记更新
        std::wstring parentKeyName = RegistryUtil::ReadStringValue(rootKey, fullKeyPath, L"ParentKeyName");
        item.isUpdate = !parentKeyName.empty();

        // 只有有卸载命令的才加入列表
        if (!item.uninstallString.empty()) {
            items.push_back(item);
        }
    }
}

bool SoftwareUninstaller::Uninstall(const InstalledSoftware& software) {
    if (software.uninstallString.empty()) {
        return false;
    }

    // 检查软件是否正在运行
    if (IsSoftwareRunning(software.installLocation)) {
        return false;
    }

    // 启动卸载程序
    std::wstring cmdLine = Win32Util::ExpandEnvVars(software.uninstallString);

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};

    BOOL success = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                                   0, nullptr, nullptr, &si, &pi);

    if (!success) {
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

bool SoftwareUninstaller::SilentUninstall(const InstalledSoftware& software) {
    if (software.uninstallString.empty()) {
        return false;
    }

    std::wstring cmdLine = Win32Util::ExpandEnvVars(software.uninstallString);

    // 检查是否支持静默卸载参数
    // MSI 安装: msiexec /x{GUID} /qn
    // NSIS: /S
    // Inno Setup: /SILENT 或 /VERYSILENT
    std::wstring lowerCmd = cmdLine;
    std::transform(lowerCmd.begin(), lowerCmd.end(), lowerCmd.begin(), ::towlower);

    if (lowerCmd.find(L"msiexec") != std::wstring::npos) {
        // MSI安装包
        if (lowerCmd.find(L"/i") != std::wstring::npos) {
            // 替换 /i 为 /x 并添加静默参数
            size_t pos = lowerCmd.find(L"/i");
            cmdLine.replace(pos, 2, L"/x");
            cmdLine += L" /qn";
        } else if (lowerCmd.find(L"/x") == std::wstring::npos) {
            cmdLine += L" /qn";
        }
    } else {
        // 尝试添加常见的静默参数
        // 对于带引号的路径，在引号后添加参数
        if (cmdLine.back() == L'"') {
            cmdLine = cmdLine.substr(0, cmdLine.size() - 1) + L" /S\"";
        } else {
            cmdLine += L" /S";
        }
    }

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    BOOL success = CreateProcessW(nullptr, &cmdLine[0], nullptr, nullptr, FALSE,
                                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

    if (!success) {
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

bool SoftwareUninstaller::IsSoftwareRunning(const std::wstring& installLocation) const {
    if (installLocation.empty()) return false;

    std::wstring expandedPath = Win32Util::ExpandEnvVars(installLocation);
    std::wstring lowerPath = expandedPath;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    bool isRunning = false;
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            std::wstring exePath = pe32.szExeFile;
            std::transform(exePath.begin(), exePath.end(), exePath.begin(), ::towlower);

            // 简单检查：如果进程可执行文件路径包含安装目录
            if (!lowerPath.empty() && exePath.find(lowerPath) != std::wstring::npos) {
                isRunning = true;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return isRunning;
}

std::vector<std::wstring> SoftwareUninstaller::ScanResidualFiles(const InstalledSoftware& software) {
    std::vector<std::wstring> residual;

    // 检查安装目录是否仍然存在
    if (!software.installLocation.empty()) {
        std::wstring expandedPath = Win32Util::ExpandEnvVars(software.installLocation);
        if (FileUtil::Exists(expandedPath)) {
            residual.push_back(expandedPath);
        }
    }

    // 检查 AppData 中的残留
    std::wstring appDataLocal = Win32Util::ExpandEnvVars(L"%LOCALAPPDATA%");
    std::wstring appDataRoaming = Win32Util::ExpandEnvVars(L"%APPDATA%");

    // 简化处理：搜索包含软件名称的目录
    std::wstring searchName = software.displayName;
    // 去掉常见后缀
    size_t parenPos = searchName.find(L" (");
    if (parenPos != std::wstring::npos) {
        searchName = searchName.substr(0, parenPos);
    }

    if (!searchName.empty()) {
        // 在 AppData\Local 中搜索
        WIN32_FIND_DATAW findData;
        std::wstring searchPattern = appDataLocal + L"\\" + searchName + L"*";
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    residual.push_back(appDataLocal + L"\\" + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }

        // 在 AppData\Roaming 中搜索
        searchPattern = appDataRoaming + L"\\" + searchName + L"*";
        hFind = FindFirstFileW(searchPattern.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    residual.push_back(appDataRoaming + L"\\" + findData.cFileName);
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    return residual;
}

bool SoftwareUninstaller::CleanResidual(const std::vector<std::wstring>& residualPaths) {
    bool allCleaned = true;

    for (const auto& path : residualPaths) {
        DWORD attrs = GetFileAttributesW(path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES) {
            continue; // 已不存在
        }

        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
            // 删除目录
            if (!FileUtil::DeleteFolder(path)) {
                allCleaned = false;
            }
        } else {
            // 删除文件
            if (!FileUtil::DeleteFilePermanently(path)) {
                allCleaned = false;
            }
        }
    }

    return allCleaned;
}

uint64_t SoftwareUninstaller::ParseEstimatedSize(const std::wstring& sizeStr) const {
    if (sizeStr.empty()) return 0;
    try {
        return std::stoull(sizeStr) * 1024; // KB -> Bytes
    } catch (...) {
        return 0;
    }
}

std::wstring SoftwareUninstaller::FormatInstallDate(const std::wstring& rawDate) const {
    // 安装日期格式: YYYYMMDD 或 YYYY-MM-DD
    if (rawDate.length() >= 8) {
        // YYYYMMDD -> YYYY-MM-DD
        if (rawDate.find(L'-') == std::wstring::npos) {
            return rawDate.substr(0, 4) + L"-" + rawDate.substr(4, 2) + L"-" + rawDate.substr(6, 2);
        }
    }
    return rawDate;
}

bool SoftwareUninstaller::IsSystemComponent(HKEY rootKey, const std::wstring& subKey) const {
    DWORD value = RegistryUtil::ReadDwordValue(rootKey, subKey, L"SystemComponent");
    return value == 1;
}

bool SoftwareUninstaller::IsWindowsUpdate(const std::wstring& displayName) const {
    // 以 "Security Update" 或 "Update for" 或 KB 开头的通常是Windows更新
    std::wstring lower = displayName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    return lower.find(L"security update") != std::wstring::npos ||
           lower.find(L"update for microsoft") != std::wstring::npos ||
           lower.find(L"update for windows") != std::wstring::npos ||
           lower.find(L"update for .net") != std::wstring::npos ||
           lower.find(L"hotfix") != std::wstring::npos ||
           (lower.find(L"kb") == 0 && lower.size() >= 8);
}

SoftwareUninstaller::ForceUninstallResult SoftwareUninstaller::ForceUninstall(
    const InstalledSoftware& software)
{
    ForceUninstallResult result;

    // 步骤1：先尝试正常卸载
    if (Uninstall(software)) {
        // 等待卸载程序启动
        Sleep(2000);
        result.message = L"已启动正常卸载程序";
        result.success = true;
        return result;
    }

    // 步骤2：终止关联进程
    if (!software.installLocation.empty()) {
        std::wstring expandedPath = Win32Util::ExpandEnvVars(software.installLocation);
        std::wstring lowerPath = expandedPath;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);

        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32W pe32{};
            pe32.dwSize = sizeof(pe32);

            if (Process32FirstW(hSnapshot, &pe32)) {
                do {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
                    if (hProcess) {
                        wchar_t processPath[MAX_PATH] = {};
                        DWORD size = MAX_PATH;
                        if (QueryFullProcessImageNameW(hProcess, 0, processPath, &size)) {
                            std::wstring procPath(processPath);
                            std::wstring lowerProcPath = procPath;
                            std::transform(lowerProcPath.begin(), lowerProcPath.end(), lowerProcPath.begin(), ::towlower);

                            if (lowerProcPath.find(lowerPath) == 0) {
                                CloseHandle(hProcess);
                                Win32Util::KillProcessByName(pe32.szExeFile);
                                result.processKilled = true;
                                continue;
                            }
                        }
                        CloseHandle(hProcess);
                    }
                } while (Process32NextW(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }
    }

    // 等待进程退出
    Sleep(1000);

    // 步骤3：强制删除安装目录
    if (!software.installLocation.empty()) {
        std::wstring expandedPath = Win32Util::ExpandEnvVars(software.installLocation);
        if (GetFileAttributesW(expandedPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            result.directoryRemoved = Win32Util::ForceDeleteDirectory(expandedPath);
        } else {
            result.directoryRemoved = true;  // 目录已不存在
        }
    } else {
        result.directoryRemoved = true;  // 无安装路径信息
    }

    // 步骤4：清理注册表残留项
    if (software.rootKey != nullptr && !software.registryKeyPath.empty()) {
        // 删除卸载注册表项
        LSTATUS status = RegDeleteTreeW(software.rootKey, software.registryKeyPath.c_str());
        result.registryCleaned = (status == ERROR_SUCCESS);

        // 同时清理相关的启动项
        std::wstring displayName = software.displayName;
        if (!displayName.empty()) {
            // 清理 HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Run 中的相关项
            auto runValues = RegistryUtil::EnumValues(HKEY_CURRENT_USER,
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
            for (const auto& val : runValues) {
                std::wstring data = RegistryUtil::ReadStringValue(HKEY_CURRENT_USER,
                    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", val);
                if (data.find(displayName) != std::wstring::npos ||
                    (!software.installLocation.empty() && data.find(software.installLocation) != std::wstring::npos)) {
                    HKEY hKey;
                    if (RegOpenKeyExW(HKEY_CURRENT_USER,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                        RegDeleteValueW(hKey, val.c_str());
                        RegCloseKey(hKey);
                    }
                }
            }

            // 清理 HKLM 中的相关启动项
            auto runValuesHKLM = RegistryUtil::EnumValues(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
            for (const auto& val : runValuesHKLM) {
                std::wstring data = RegistryUtil::ReadStringValue(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", val);
                if (data.find(displayName) != std::wstring::npos ||
                    (!software.installLocation.empty() && data.find(software.installLocation) != std::wstring::npos)) {
                    HKEY hKey;
                    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
                        0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                        RegDeleteValueW(hKey, val.c_str());
                        RegCloseKey(hKey);
                    }
                }
            }
        }
    } else {
        result.registryCleaned = true;  // 无注册表信息
    }

    // 判断总体是否成功
    result.success = result.directoryRemoved && result.registryCleaned;

    if (result.success) {
        result.message = L"强制卸载完成";
    } else {
        result.message = L"强制卸载部分完成";
        if (!result.directoryRemoved) result.message += L"（安装目录未完全删除）";
        if (!result.registryCleaned) result.message += L"（注册表未完全清理）";
    }

    return result;
}

// ── 多来源应用检测 ──

std::vector<AppSourceInfo> SoftwareUninstaller::GetAllApps() {
    std::vector<AppSourceInfo> allApps;

    // 注册表应用
    auto regApps = GetInstalledSoftware();
    for (const auto& app : regApps) {
        AppSourceInfo info;
        info.source = InstallSource::Registry;
        info.identifier = app.registryKeyPath;
        info.displayName = app.displayName;
        info.installLocation = app.installLocation;
        info.estimatedSize = app.estimatedSize;
        allApps.push_back(info);
    }

    // Store/UWP 应用
    auto storeApps = GetStoreApps();
    allApps.insert(allApps.end(), storeApps.begin(), storeApps.end());

    return allApps;
}

std::vector<AppSourceInfo> SoftwareUninstaller::GetStoreApps() {
    std::vector<AppSourceInfo> apps;

    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) return apps;

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};
    std::wstring cmd = L"powershell -NoProfile -Command \"Get-AppxPackage | Select-Object PackageFullName, Name, PackageSize\"";

    if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, TRUE,
                         CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        return apps;
    }

    CloseHandle(hWritePipe);

    char buffer[16384] = {};
    DWORD bytesRead = 0;
    std::string output;
    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        output.append(buffer, bytesRead);
    }

    CloseHandle(hReadPipe);
    WaitForSingleObject(pi.hProcess, 15000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 解析 PowerShell 输出（简化版）
    std::istringstream iss(output);
    std::string line;
    bool headerSkipped = false;

    while (std::getline(iss, line)) {
        if (!headerSkipped) {
            headerSkipped = true;
            continue;
        }

        // 跳过空行和分隔线
        if (line.empty() || line.find("---") != std::string::npos) continue;

        // 简单解析：空格分隔
        std::istringstream lineStream(line);
        std::string packageFullName, name, sizeStr;
        lineStream >> packageFullName >> name >> sizeStr;

        if (!packageFullName.empty()) {
            AppSourceInfo info;
            info.source = InstallSource::Store;
            info.identifier = std::wstring(packageFullName.begin(), packageFullName.end());
            info.displayName = std::wstring(name.begin(), name.end());
            if (!sizeStr.empty()) {
                try {
                    info.estimatedSize = std::stoull(sizeStr);
                } catch (...) {}
            }
            apps.push_back(info);
        }
    }

    return apps;
}

bool SoftwareUninstaller::UninstallStoreApp(const std::wstring& packageFullName) {
    std::wstring cmd = L"powershell -NoProfile -Command \"Get-AppxPackage -AllUsers '" +
                       packageFullName + L"' | Remove-AppxPackage -AllUsers\"";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE,
                         CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 60000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

} // namespace IceClean::Core::Cleaner
