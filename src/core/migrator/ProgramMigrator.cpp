#include "ProgramMigrator.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include "utils/RegistryUtil.h"
#include "utils/FormatUtil.h"
#include <windows.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cctype>
#include <spdlog/spdlog.h>

namespace IceClean::Core::Migrator {

namespace {
constexpr uint64_t kMinSizeBytes = 50ULL * 1024 * 1024; // 50 MB

const std::vector<std::wstring>& Blacklist() {
    static const std::vector<std::wstring> kList = {
        L"Windows Defender",
        L"Windows Security",
        L"Windows Mail",
        L"Windows Media Player",
        L"Windows Photo Viewer",
        L"Windows Internet Explorer",
        L"Internet Explorer",
        L"Microsoft Edge",
        L"Microsoft EdgeWebView",
        L"Microsoft Visual C++",
        L"Microsoft .NET",
        L"Microsoft OneDrive",
        L"Windows Subsystem for Linux",
        L"Windows Terminal",
        L"WindowsAppRuntime",
    };
    return kList;
}

const std::vector<std::wstring>& CautionList() {
    static const std::vector<std::wstring> kList = {
        L"Microsoft Office",
        L"Microsoft Visual Studio",
        L"Microsoft SQL Server",
        L"Adobe",
        L"Autodesk",
        L"NVIDIA",
        L"AMD",
        L"Intel",
        L"Realtek",
    };
    return kList;
}
} // namespace

ProgramMigrator::ProgramMigrator() = default;

std::wstring ProgramMigrator::GetName() const {
    return L"已安装应用迁移";
}

Models::MigrationType ProgramMigrator::GetMigrationType() const {
    return Models::MigrationType::InstalledProgram;
}

std::vector<ProgramMigrator::ProgramRegInfo>
ProgramMigrator::ScanRegistryKey(HKEY rootKey, const std::wstring& subKey) {
    std::vector<ProgramRegInfo> results;

    HKEY hKey{};
    if (RegOpenKeyExW(rootKey, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return results;
    }

    DWORD index = 0;
    constexpr DWORD kMaxName = 256;
    wchar_t subName[kMaxName]{};
    DWORD subNameLen = kMaxName;

    while (RegEnumKeyExW(hKey, index++, subName, &subNameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
        std::wstring subPath = subKey + L"\\" + subName;

        HKEY hSub{};
        if (RegOpenKeyExW(rootKey, subPath.c_str(), 0, KEY_READ, &hSub) == ERROR_SUCCESS) {
            ProgramRegInfo info;
            info.is64Bit = (rootKey == HKEY_LOCAL_MACHINE && subKey.find(L"Wow6432Node") == std::wstring::npos);

            constexpr DWORD kBuf = 1024;
            wchar_t buffer[kBuf]{};
            DWORD bufLen = kBuf * sizeof(wchar_t);
            DWORD type = 0;

            if (RegQueryValueExW(hSub, L"DisplayName", nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buffer), &bufLen) == ERROR_SUCCESS) {
                info.displayName = buffer;
            }

            bufLen = kBuf * sizeof(wchar_t);
            if (RegQueryValueExW(hSub, L"InstallLocation", nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buffer), &bufLen) == ERROR_SUCCESS) {
                info.installLocation = buffer;
                if (!info.installLocation.empty() && info.installLocation.back() == L'\\') {
                    info.installLocation.pop_back();
                }
            }

            bufLen = kBuf * sizeof(wchar_t);
            if (RegQueryValueExW(hSub, L"Publisher", nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buffer), &bufLen) == ERROR_SUCCESS) {
                info.publisher = buffer;
            }

            bufLen = kBuf * sizeof(wchar_t);
            if (RegQueryValueExW(hSub, L"DisplayVersion", nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buffer), &bufLen) == ERROR_SUCCESS) {
                info.version = buffer;
            }

            bufLen = kBuf * sizeof(wchar_t);
            if (RegQueryValueExW(hSub, L"UninstallString", nullptr, &type,
                                 reinterpret_cast<LPBYTE>(buffer), &bufLen) == ERROR_SUCCESS) {
                info.uninstallString = buffer;
            }

            RegCloseKey(hSub);

            if (!info.displayName.empty() && !info.installLocation.empty() && IsInProgramFiles(info.installLocation)) {
                results.push_back(std::move(info));
            }
        }

        subNameLen = kMaxName;
    }

    RegCloseKey(hKey);
    return results;
}

std::vector<ProgramMigrator::ProgramRegInfo> ProgramMigrator::ScanRegistry() {
    std::vector<ProgramRegInfo> results;

    auto infos1 = ScanRegistryKey(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
    results.insert(results.end(), infos1.begin(), infos1.end());
    auto infos2 = ScanRegistryKey(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
    results.insert(results.end(), infos2.begin(), infos2.end());
    auto infos3 = ScanRegistryKey(HKEY_CURRENT_USER,
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
    results.insert(results.end(), infos3.begin(), infos3.end());

    return results;
}

bool ProgramMigrator::IsInProgramFiles(const std::wstring& path) {
    if (path.empty()) return false;
    std::wstring lower = path;
    std::transform(lower.begin(), lower.end(), lower.begin(), towlower);

    return lower.find(L"c:\\program files") == 0;
}

bool ProgramMigrator::IsSystemProgram(const std::wstring& name) {
    std::wstring lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), towlower);
    for (const auto& black : Blacklist()) {
        std::wstring lblack = black;
        std::transform(lblack.begin(), lblack.end(), lblack.begin(), towlower);
        if (lower.find(lblack) != std::wstring::npos) return true;
    }
    return false;
}

ProgramMigrator::SafetyConfig ProgramMigrator::EvaluateSafety(
    const std::wstring& programName,
    const std::wstring& installPath,
    bool /*isSystemComponent*/) {
    std::wstring lowerName = programName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), towlower);

    if (IsSystemProgram(programName)) {
        return { Models::ProgramSafetyLevel::Dangerous,
                 L"系统组件/Windows 必备应用，迁移可能导致系统不稳定" };
    }

    for (const auto& caution : CautionList()) {
        std::wstring lcaution = caution;
        std::transform(lcaution.begin(), lcaution.end(), lcaution.begin(), towlower);
        if (lowerName.find(lcaution) != std::wstring::npos) {
            return { Models::ProgramSafetyLevel::Caution,
                     L"可能影响系统或其他应用，建议先关闭后迁移" };
        }
    }

    return { Models::ProgramSafetyLevel::Safe, L"普通桌面应用，可安全迁移" };
}

uint64_t ProgramMigrator::GetFolderSizeSync(const std::wstring& path) {
    uint64_t total = 0;
    std::wstring searchPath = path;
    if (!searchPath.empty() && searchPath.back() != L'\\') searchPath += L'\\';
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring full = path;
        if (!full.empty() && full.back() != L'\\') full += L'\\';
        full += findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) continue;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            total += GetFolderSizeSync(full);
        } else {
            ULARGE_INTEGER sz{};
            sz.LowPart = findData.nFileSizeLow;
            sz.HighPart = findData.nFileSizeHigh;
            total += sz.QuadPart;
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return total;
}

uint64_t ProgramMigrator::MeasureDirectorySize(const std::wstring& path,
                                                ProgressCallback& cb,
                                                int foundCount) {
    if (cb) cb(path, foundCount, 0);
    return GetFolderSizeSync(path);
}

bool ProgramMigrator::IsProcessRunning(const std::wstring& processName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    bool running = false;
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName.c_str()) == 0) {
                running = true;
                break;
            }
        } while (Process32NextW(hSnap, &pe));
    }

    CloseHandle(hSnap);
    return running;
}

std::vector<Models::MigrationItem> ProgramMigrator::Detect() {
    return DetectWithCallback(nullptr);
}

std::vector<Models::MigrationItem> ProgramMigrator::DetectWithCallback(ProgressCallback progressCallback) {
    cancelled_ = false;

    auto regInfos = ScanRegistry();
    spdlog::info("ProgramMigrator: found {} registry entries", static_cast<int>(regInfos.size()));

    std::vector<Models::MigrationItem> results;
    int processed = 0;
    uint64_t totalBytes = 0;

    for (auto& info : regInfos) {
        if (cancelled_) break;
        processed++;

        if (!Utils::FileUtil::Exists(info.installLocation)) {
            continue;
        }

        // 黑名单：系统组件
        if (IsSystemProgram(info.displayName)) {
            if (progressCallback) {
                progressCallback(info.installLocation, processed, totalBytes);
            }
            continue;
        }

        // 测量目录大小
        auto cb = progressCallback;
        int count = processed;
        uint64_t size = MeasureDirectorySize(info.installLocation, cb, count);

        if (progressCallback) {
            progressCallback(info.installLocation, processed, totalBytes + size);
        }

        if (size < kMinSizeBytes) continue;

        auto safety = EvaluateSafety(info.displayName, info.installLocation, false);

        Models::MigrationItem item;
        item.name = info.displayName;
        item.sourcePath = info.installLocation;
        item.size = size;
        item.type = Models::MigrationType::InstalledProgram;
        item.advice = safety.level == Models::ProgramSafetyLevel::Safe
                          ? Models::MigrationAdvice::Recommended
                      : safety.level == Models::ProgramSafetyLevel::Caution
                          ? Models::MigrationAdvice::Possible
                          : Models::MigrationAdvice::NotRecommended;
        item.publisher = info.publisher;
        item.version = info.version;
        item.uninstallString = info.uninstallString;
        item.programSafety = safety.level;
        item.safetyReason = safety.reason;
        item.is64Bit = info.is64Bit;

        // 检测同名进程是否正在运行
        std::wstring exeName = info.installLocation;
        size_t pos = exeName.find_last_of(L'\\');
        if (pos != std::wstring::npos) {
            exeName = exeName.substr(pos + 1) + L".exe";
        }
        item.isRunning = IsProcessRunning(exeName);

        results.push_back(item);
        totalBytes += size;
    }

    std::sort(results.begin(), results.end(),
              [](const Models::MigrationItem& a, const Models::MigrationItem& b) {
                  return a.size > b.size;
              });

    spdlog::info("ProgramMigrator: {} migratable programs found", static_cast<int>(results.size()));
    return results;
}

Models::MigrationResult ProgramMigrator::Migrate(
    const std::vector<Models::MigrationItem>& items,
    const std::wstring& targetDrive,
    std::function<void(const Models::MigrationProgress&)> progressCb) {

    Models::MigrationResult result{};

    for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];

        if (!progressCb) {
            if (!MoveAndCreateJunction(item.sourcePath, targetDrive)) {
                result.failedCount++;
                continue;
            }
        } else {
            if (!MoveAndCreateJunction(
                    item.sourcePath, targetDrive,
                    [progressCb, i, total = items.size()](const Models::MigrationProgress& p) {
                        Models::MigrationProgress prog = p;
                        prog.currentItem = static_cast<int>(i);
                        prog.totalItems = static_cast<int>(total);
                        progressCb(prog);
                    },
                    static_cast<int>(i), static_cast<int>(items.size()))) {
                result.failedCount++;
                continue;
            }
        }

        result.migratedCount++;
        result.totalMigratedSize += item.size;
    }

    result.success = result.failedCount == 0;
    return result;
}

void ProgramMigrator::Cancel() {
    cancelled_ = true;
}

} // namespace IceClean::Core::Migrator
