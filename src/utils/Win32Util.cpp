#include "Win32Util.h"
#include <shlobj.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "psapi.lib")

namespace IceClean::Utils {

bool Win32Util::IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    PSID adminGroup = nullptr;

    if (AllocateAndInitializeSid(&ntAuth, 2,
        SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin == TRUE;
}

std::wstring Win32Util::GetWindowsVersion() {
    // 使用RtlGetVersion获取真实版本号(不受兼容性shim影响)
    typedef LONG(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return L"Unknown";

    auto RtlGetVersion = reinterpret_cast<RtlGetVersionPtr>(
        GetProcAddress(ntdll, "RtlGetVersion"));
    if (!RtlGetVersion) return L"Unknown";

    RTL_OSVERSIONINFOW osvi = {};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (RtlGetVersion(&osvi) != 0) return L"Unknown";

    std::wostringstream oss;
    oss << L"Windows ";

    // 根据版本号判断产品名称
    if (osvi.dwMajorVersion == 10) {
        if (osvi.dwBuildNumber >= 22000) {
            oss << L"11";
        } else {
            oss << L"10";
        }
    } else if (osvi.dwMajorVersion == 6) {
        if (osvi.dwMinorVersion == 3) oss << L"8.1";
        else if (osvi.dwMinorVersion == 2) oss << L"8";
        else if (osvi.dwMinorVersion == 1) oss << L"7";
        else if (osvi.dwMinorVersion == 0) oss << L"Vista";
        else oss << osvi.dwMajorVersion << L"." << osvi.dwMinorVersion;
    } else {
        oss << osvi.dwMajorVersion << L"." << osvi.dwMinorVersion;
    }

    oss << L" (Build " << osvi.dwBuildNumber << L")";
    return oss.str();
}

std::wstring Win32Util::GetSystemDrive() {
    wchar_t buffer[MAX_PATH] = {};
    if (GetEnvironmentVariableW(L"SystemDrive", buffer, MAX_PATH) > 0) {
        return buffer;
    }
    return L"C:";
}

std::vector<std::wstring> Win32Util::GetAvailableDrives() {
    std::vector<std::wstring> drives;
    DWORD driveMask = GetLogicalDrives();

    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        if (driveMask & (1 << (letter - L'A'))) {
            drives.push_back(std::wstring(1, letter) + L":\\");
        }
    }

    return drives;
}

bool Win32Util::GetDiskSpace(const std::wstring& drive, uint64_t& totalBytes, uint64_t& freeBytes) {
    ULARGE_INTEGER freeAvail{};
    ULARGE_INTEGER total{};
    ULARGE_INTEGER free{};

    std::wstring drivePath = drive;
    if (drivePath.back() != L'\\') {
        drivePath += L'\\';
    }

    if (GetDiskFreeSpaceExW(drivePath.c_str(), &freeAvail, &total, &free)) {
        totalBytes = total.QuadPart;
        freeBytes = freeAvail.QuadPart;
        return true;
    }

    return false;
}

std::wstring Win32Util::GetSpecialFolder(int csidl) {
    wchar_t buffer[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathW(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, buffer))) {
        return buffer;
    }
    return L"";
}

std::wstring Win32Util::ExpandEnvVars(const std::wstring& path) {
    DWORD size = ExpandEnvironmentStringsW(path.c_str(), nullptr, 0);
    if (size == 0) return path;

    std::wstring result(size - 1, L'\0');
    ExpandEnvironmentStringsW(path.c_str(), result.data(), size);
    return result;
}

bool Win32Util::IsProcessRunning(const std::wstring& processName) {
    return !GetProcessIdsByName(processName).empty();
}

std::vector<DWORD> Win32Util::GetProcessIdsByName(const std::wstring& processName) {
    std::vector<DWORD> pids;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return pids;

    PROCESSENTRY32W pe32{};
    pe32.dwSize = sizeof(pe32);

    if (Process32FirstW(snapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                pids.push_back(pe32.th32ProcessID);
            }
        } while (Process32NextW(snapshot, &pe32));
    }

    CloseHandle(snapshot);
    return pids;
}

} // namespace IceClean::Utils
