#include "DriverManager.h"
#include "utils/FormatUtil.h"
#include "utils/RegistryUtil.h"
#include <algorithm>
#include <filesystem>

namespace IceClean::Core::Optimizer {

std::vector<Models::DriverInfo> DriverManager::GetDrivers() {
    std::vector<Models::DriverInfo> drivers;

    // 枚举 HKLM\SYSTEM\CurrentControlSet\Enum 下的设备
    HKEY hEnumKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                       L"SYSTEM\\CurrentControlSet\\Enum",
                       0, KEY_READ, &hEnumKey) != ERROR_SUCCESS) {
        return drivers;
    }

    wchar_t className[256] = {};
    DWORD classNameSize = 256;
    DWORD classIndex = 0;

    while (RegEnumKeyExW(hEnumKey, classIndex, className, &classNameSize,
                          nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
        classNameSize = 256;
        HKEY hClassKey;
        if (RegOpenKeyExW(hEnumKey, className, 0, KEY_READ, &hClassKey) == ERROR_SUCCESS) {
            wchar_t deviceId[256] = {};
            DWORD deviceIdSize = 256;
            DWORD deviceIndex = 0;

            while (RegEnumKeyExW(hClassKey, deviceIndex, deviceId, &deviceIdSize,
                                  nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                deviceIdSize = 256;

                std::wstring enumPath = std::wstring(className) + L"\\" + deviceId;
                HKEY hDeviceKey;
                if (RegOpenKeyExW(hClassKey, deviceId, 0, KEY_READ, &hDeviceKey) == ERROR_SUCCESS) {
                    // 检查是否有 Driver 子键
                    wchar_t driverSubkey[256] = {};
                    DWORD driverSize = 256 * sizeof(wchar_t);
                    if (RegQueryValueExW(hDeviceKey, L"Driver", nullptr, nullptr,
                                          (LPBYTE)driverSubkey, &driverSize) == ERROR_SUCCESS) {
                        Models::DriverInfo info;

                        // 读取设备名称 FriendlyName 或 DeviceDesc
                        wchar_t friendlyName[512] = {};
                        DWORD fnSize = sizeof(friendlyName);
                        if (RegQueryValueExW(hDeviceKey, L"FriendlyName", nullptr, nullptr,
                                              (LPBYTE)friendlyName, &fnSize) != ERROR_SUCCESS) {
                            RegQueryValueExW(hDeviceKey, L"DeviceDesc", nullptr, nullptr,
                                              (LPBYTE)friendlyName, &fnSize);
                        }
                        info.deviceName = friendlyName;

                        // 读取硬件ID
                        wchar_t hardwareId[512] = {};
                        DWORD hwSize = sizeof(hardwareId);
                        RegQueryValueExW(hDeviceKey, L"HardwareID", nullptr, nullptr,
                                          (LPBYTE)hardwareId, &hwSize);
                        info.hardwareId = hardwareId;

                        // 从 Driver 子键路径读取驱动详细信息
                        // Driver值格式: {ClassGUID}\{InstanceID}
                        std::wstring driverKeyPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\" + std::wstring(driverSubkey);
                        ReadDriverFromRegistry(HKEY_LOCAL_MACHINE, driverKeyPath, info);

                        if (!info.driverDesc.empty() || !info.deviceName.empty()) {
                            drivers.push_back(info);
                        }
                    }
                    RegCloseKey(hDeviceKey);
                }
                deviceIndex++;
            }
            RegCloseKey(hClassKey);
        }
        classIndex++;
    }

    RegCloseKey(hEnumKey);

    // 去重(同一驱动可能被多个设备引用)
    std::sort(drivers.begin(), drivers.end(), [](const auto& a, const auto& b) {
        return a.driverPath < b.driverPath;
    });
    drivers.erase(std::unique(drivers.begin(), drivers.end(), [](const auto& a, const auto& b) {
        return a.driverPath == b.driverPath && a.driverVersion == b.driverVersion;
    }), drivers.end());

    return drivers;
}

std::vector<Models::DriverInfo> DriverManager::GetOutdatedDrivers() {
    auto allDrivers = GetDrivers();
    std::vector<Models::DriverInfo> outdated;

    for (auto& driver : allDrivers) {
        if (CheckDriverUpdate(driver)) {
            driver.hasUpdate = true;
            outdated.push_back(driver);
        }
    }

    return outdated;
}

std::vector<Models::DriverInfo> DriverManager::GetThirdPartyDrivers() {
    auto allDrivers = GetDrivers();
    std::vector<Models::DriverInfo> thirdParty;

    for (const auto& driver : allDrivers) {
        if (!IsSystemDriver(driver.driverProvider)) {
            thirdParty.push_back(driver);
        }
    }

    return thirdParty;
}

bool DriverManager::BackupDriver(const Models::DriverInfo& driver, const std::wstring& backupDir) {
    if (driver.driverPath.empty()) return false;

    namespace fs = std::filesystem;
    try {
        std::wstring destDir = backupDir + L"\\" + driver.driverDesc;
        // 清理非法字符
        for (auto& c : destDir) {
            if (c == L'<' || c == L'>' || c == L':' || c == L'"' ||
                c == L'|' || c == L'?' || c == L'*') {
                c = L'_';
            }
        }

        fs::create_directories(destDir);

        // 复制驱动文件
        std::wstring srcPath = driver.driverPath;
        if (srcPath.find(L"\\SystemRoot\\") == 0) {
            srcPath = L"C:\\Windows" + srcPath.substr(12);
        } else if (srcPath.find(L"\\??\\") == 0) {
            srcPath = srcPath.substr(4);
        }

        if (fs::exists(srcPath)) {
            fs::copy_file(srcPath, destDir + L"\\" + fs::path(srcPath).filename().wstring(),
                          fs::copy_options::overwrite_existing);
        }

        // 复制相关文件(.sys, .inf, .cat, .dll)
        std::wstring driverDir = fs::path(srcPath).parent_path().wstring();
        for (const auto& entry : fs::directory_iterator(driverDir)) {
            auto ext = entry.path().extension().wstring();
            // 只复制与驱动文件名前缀匹配的文件
            std::wstring baseName = fs::path(srcPath).stem().wstring();
            if (entry.path().stem().wstring().find(baseName) == 0 &&
                (ext == L".sys" || ext == L".inf" || ext == L".cat" || ext == L".dll")) {
                try {
                    fs::copy_file(entry.path(), destDir + L"\\" + entry.path().filename().wstring(),
                                  fs::copy_options::overwrite_existing);
                } catch (...) {}
            }
        }

        return true;
    } catch (...) {
        return false;
    }
}

int DriverManager::BackupAllDrivers(const std::wstring& backupDir,
                                      std::function<void(int, int, const std::wstring&)> progress) {
    auto drivers = GetThirdPartyDrivers();
    int successCount = 0;
    int total = static_cast<int>(drivers.size());

    for (int i = 0; i < total; ++i) {
        if (progress) {
            progress(i + 1, total, drivers[i].driverDesc);
        }
        if (BackupDriver(drivers[i], backupDir)) {
            successCount++;
        }
    }

    return successCount;
}

bool DriverManager::RestoreDriver(const Models::DriverBackupInfo& backup) {
    // 使用 pnputil 安装驱动
    std::wstring cmd = L"pnputil /add-driver \"" + backup.backupPath + L"\" /install";
    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                        FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 60000);  // 最多等60秒
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
}

std::vector<Models::DriverBackupInfo> DriverManager::GetBackupDrivers(const std::wstring& backupDir) {
    std::vector<Models::DriverBackupInfo> backups;
    namespace fs = std::filesystem;

    try {
        if (!fs::exists(backupDir)) return backups;

        for (const auto& entry : fs::directory_iterator(backupDir)) {
            if (entry.is_directory()) {
                Models::DriverBackupInfo info;
                info.backupPath = entry.path().wstring();
                info.driverDesc = entry.path().filename().wstring();

                // 计算备份大小
                uint64_t size = 0;
                for (const auto& f : fs::recursive_directory_iterator(entry.path())) {
                    if (f.is_regular_file()) {
                        size += f.file_size();
                    }
                }
                info.backupSize = size;

                // 获取修改时间作为备份日期
                auto ftime = fs::last_write_time(entry.path());
                auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                auto time_t_val = std::chrono::system_clock::to_time_t(sctp);
                wchar_t timeBuf[64] = {};
                struct tm tm_val;
                localtime_s(&tm_val, &time_t_val);
                wcsftime(timeBuf, 64, L"%Y-%m-%d %H:%M", &tm_val);
                info.backupDate = timeBuf;

                backups.push_back(info);
            }
        }
    } catch (...) {}

    return backups;
}

uint64_t DriverManager::CleanupOldDriverBackups(std::function<void(const std::wstring&)> progress) {
    // 使用 Dism 清理驱动存储中的旧备份
    uint64_t freed = 0;
    uint64_t beforeSize = GetDriverStoreSize();

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    std::wstring cmd = L"Dism.exe /Online /Cleanup-Image /StartComponentCleanup";
    if (CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                        FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 120000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    // 同时使用 pnputil 删除旧驱动包
    cmd = L"pnputil /enum-drivers";
    // (此处简化实现，实际需要解析输出并删除非当前驱动)

    uint64_t afterSize = GetDriverStoreSize();
    if (beforeSize > afterSize) {
        freed = beforeSize - afterSize;
    }

    return freed;
}

uint64_t DriverManager::GetDriverStoreSize() {
    uint64_t totalSize = 0;
    namespace fs = std::filesystem;

    std::wstring driverStore = L"C:\\Windows\\System32\\DriverStore\\FileRepository";
    try {
        if (fs::exists(driverStore)) {
            for (const auto& entry : fs::directory_iterator(driverStore)) {
                if (entry.is_directory()) {
                    for (const auto& f : fs::recursive_directory_iterator(entry.path())) {
                        if (f.is_regular_file()) {
                            totalSize += f.file_size();
                        }
                    }
                }
            }
        }
    } catch (...) {}

    return totalSize;
}

void DriverManager::ReadDriverFromRegistry(HKEY hRootKey, const std::wstring& subKey, Models::DriverInfo& info) {
    HKEY hKey;
    if (RegOpenKeyExW(hRootKey, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return;
    }

    auto readString = [&](const wchar_t* valueName) -> std::wstring {
        wchar_t buf[512] = {};
        DWORD size = sizeof(buf);
        if (RegQueryValueExW(hKey, valueName, nullptr, nullptr, (LPBYTE)buf, &size) == ERROR_SUCCESS) {
            return buf;
        }
        return L"";
    };

    info.driverDesc = readString(L"");
    if (info.driverDesc.empty()) {
        info.driverDesc = readString(L"DriverDesc");
    }
    info.driverProvider = readString(L"ProviderName");
    info.driverVersion = readString(L"DriverVersion");
    info.driverDate = readString(L"DriverDate");
    info.driverPath = readString(L"ImagePath");
    info.infPath = readString(L"InfPath");

    // 读取 MatchingDeviceId
    std::wstring matchingId = readString(L"MatchingDeviceId");
    if (info.hardwareId.empty() && !matchingId.empty()) {
        info.hardwareId = matchingId;
    }

    // 检查驱动文件大小
    if (!info.driverPath.empty()) {
        std::wstring sysPath = info.driverPath;
        if (sysPath.find(L"\\SystemRoot\\") == 0) {
            sysPath = L"C:\\Windows" + sysPath.substr(12);
        } else if (sysPath.find(L"\\??\\") == 0) {
            sysPath = sysPath.substr(4);
        }

        WIN32_FILE_ATTRIBUTE_DATA fileData;
        if (GetFileAttributesExW(sysPath.c_str(), GetFileExInfoStandard, &fileData)) {
            LARGE_INTEGER li;
            li.HighPart = fileData.nFileSizeHigh;
            li.LowPart = fileData.nFileSizeLow;
            info.driverSize = li.QuadPart;
        }
    }

    // 判断是否系统驱动
    info.isSystemDriver = IsSystemDriver(info.driverProvider);

    RegCloseKey(hKey);
}

bool DriverManager::IsSystemDriver(const std::wstring& provider) const {
    if (provider.empty()) return true;

    std::vector<std::wstring> systemProviders = {
        L"Microsoft", L"Microsoft Corporation",
        L"Intel Corporation",       // 常见但允许操作
    };

    // 仅 Microsoft 标记为系统驱动
    std::wstring lower = provider;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
    if (lower.find(L"microsoft") != std::wstring::npos) {
        return true;
    }

    return false;
}

bool DriverManager::CheckDriverUpdate(const Models::DriverInfo& driver) const {
    // 简化实现：检查 Windows Update 是否有更新
    // 实际产品应调用 Windows Update API
    // 这里基于驱动日期判断：超过2年的标记为可能有更新
    if (driver.driverDate.empty()) return false;

    // 解析日期 (格式: M/D/YYYY 或 YYYY-MM-DD)
    int year = 0;
    if (swscanf_s(driver.driverDate.c_str(), L"%*d/%*d/%d", &year) != 1) {
        if (swscanf_s(driver.driverDate.c_str(), L"%d-", &year) != 1) {
            return false;
        }
    }

    // 如果驱动日期在2023年之前，标记为可能有更新
    return year > 0 && year < 2023;
}

} // namespace IceClean::Core::Optimizer
