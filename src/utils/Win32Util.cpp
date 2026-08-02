#include "Win32Util.h"
#include <shlobj.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <sstream>
#include <iomanip>
#include <algorithm>

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

bool Win32Util::KillProcessById(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) return false;

    BOOL result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return result != 0;
}

bool Win32Util::EnableDebugPrivilege() {
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return false;
    }

    LUID luid{};
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
        CloseHandle(hToken);
        return false;
    }

    TOKEN_PRIVILEGES tp{};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr);
    DWORD err = GetLastError();
    CloseHandle(hToken);

    return result != 0 && err == ERROR_SUCCESS;
}

bool Win32Util::ForceKillProcessById(DWORD pid) {
    // 先尝试普通方式
    if (KillProcessById(pid)) return true;

    // 启用SeDebugPrivilege后重试
    EnableDebugPrivilege();

    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) return false;

    BOOL result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);
    return result != 0;
}

bool Win32Util::KillProcessAsSystem(const std::wstring& processName) {
    // 使用计划任务以SYSTEM身份运行taskkill命令
    // 这是终极手段，可以终止几乎所有用户态保护进程

    // 生成唯一的任务名
    std::wstring taskName = L"IceClean_KillProcess_" + std::to_wstring(GetTickCount64());

    // 构建taskkill命令
    std::wstring command = L"cmd.exe /c taskkill /F /IM " + processName;

    // 创建计划任务以SYSTEM身份运行
    std::wstring schtasksCmd = L"schtasks /Create /TN \"" + taskName +
        L"\" /TR \"" + command +
        L"\" /SC ONCE /ST 00:00 /RU SYSTEM /F";

    // 创建任务
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi{};

    std::wstring createCmd = L"cmd.exe /c " + schtasksCmd;
    if (!CreateProcessW(nullptr, createCmd.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 运行任务
    std::wstring runCmd = L"cmd.exe /c schtasks /Run /TN \"" + taskName + L"\"";
    if (!CreateProcessW(nullptr, runCmd.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        // 删除任务
        std::wstring delCmd = L"cmd.exe /c schtasks /Delete /TN \"" + taskName + L"\" /F";
        CreateProcessW(nullptr, delCmd.data(), nullptr, nullptr, FALSE,
                       CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        if (pi.hProcess) { WaitForSingleObject(pi.hProcess, 5000); CloseHandle(pi.hProcess); CloseHandle(pi.hThread); }
        return false;
    }
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 等待进程被终止（最多5秒）
    for (int i = 0; i < 25; ++i) {
        if (GetProcessIdsByName(processName).empty()) break;
        Sleep(200);
    }

    // 删除计划任务
    std::wstring delCmd = L"cmd.exe /c schtasks /Delete /TN \"" + taskName + L"\" /F";
    CreateProcessW(nullptr, delCmd.data(), nullptr, nullptr, FALSE,
                   CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    if (pi.hProcess) { WaitForSingleObject(pi.hProcess, 5000); CloseHandle(pi.hProcess); CloseHandle(pi.hThread); }

    return GetProcessIdsByName(processName).empty();
}

bool Win32Util::KillProcessByName(const std::wstring& processName) {
    auto pids = GetProcessIdsByName(processName);
    if (pids.empty()) return true;  // 进程不存在，视为成功

    // 第一轮：普通方式终止
    bool allKilled = true;
    for (DWORD pid : pids) {
        if (!KillProcessById(pid)) {
            allKilled = false;
        }
    }

    // 等待进程退出（最多2秒）
    for (int i = 0; i < 20; ++i) {
        if (GetProcessIdsByName(processName).empty()) return true;
        Sleep(100);
    }

    // 第二轮：启用SeDebugPrivilege后强制终止
    EnableDebugPrivilege();
    pids = GetProcessIdsByName(processName);
    for (DWORD pid : pids) {
        ForceKillProcessById(pid);
    }

    // 等待进程退出（最多2秒）
    for (int i = 0; i < 20; ++i) {
        if (GetProcessIdsByName(processName).empty()) return true;
        Sleep(100);
    }

    // 第三轮：终极手段 - 以SYSTEM身份终止
    pids = GetProcessIdsByName(processName);
    if (!pids.empty()) {
        return KillProcessAsSystem(processName);
    }

    return true;
}

std::wstring Win32Util::ExtractProcessName(const std::wstring& path) {
    // 处理带引号的路径
    std::wstring cleanPath = path;
    if (!cleanPath.empty() && cleanPath.front() == L'"') {
        auto endQuote = cleanPath.find(L'"', 1);
        if (endQuote != std::wstring::npos) {
            cleanPath = cleanPath.substr(1, endQuote - 1);
        }
    }

    // 去掉命令行参数，提取可执行文件路径
    auto spacePos = cleanPath.find(L' ');
    if (spacePos != std::wstring::npos) {
        // 只有当空格不在引号内时才截断
        std::wstring possiblePath = cleanPath.substr(0, spacePos);
        // 如果截取的部分包含.exe，则认为是完整路径
        if (possiblePath.size() >= 4 &&
            _wcsicmp(possiblePath.substr(possiblePath.size() - 4).c_str(), L".exe") == 0) {
            cleanPath = possiblePath;
        }
    }

    // 提取文件名
    auto lastSlash = cleanPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        return cleanPath.substr(lastSlash + 1);
    }
    return cleanPath;
}

bool Win32Util::ForceDeleteFile(const std::wstring& path) {
    // 先尝试直接删除
    if (DeleteFileW(path.c_str())) return true;

    // 如果文件被占用，查找并终止占用进程
    // 通过文件路径查找占用该文件的进程
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32{};
        pe32.dwSize = sizeof(pe32);

        std::wstring fileName = ExtractProcessName(path);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                // 检查进程可执行文件是否在目标路径下
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    wchar_t processPath[MAX_PATH] = {};
                    DWORD size = MAX_PATH;
                    if (QueryFullProcessImageNameW(hProcess, 0, processPath, &size)) {
                        std::wstring procPath(processPath);
                        // 不区分大小写比较路径
                        if (_wcsicmp(procPath.c_str(), path.c_str()) == 0) {
                            CloseHandle(hProcess);
                            // 使用三轮递进策略终止进程
                            KillProcessByName(pe32.szExeFile);
                            break;
                        }
                    }
                    CloseHandle(hProcess);
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

    // 等待进程退出
    Sleep(500);

    // 再次尝试删除
    return DeleteFileW(path.c_str()) != 0;
}

bool Win32Util::ForceDeleteDirectory(const std::wstring& path) {
    // 先终止目录下所有运行中的进程
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe32{};
        pe32.dwSize = sizeof(pe32);

        std::wstring lowerPath = path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::towlower);

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

                        // 如果进程路径在目标目录下，终止它
                        if (lowerProcPath.find(lowerPath) == 0) {
                            CloseHandle(hProcess);
                            KillProcessByName(pe32.szExeFile);
                            continue;
                        }
                    }
                    CloseHandle(hProcess);
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

    // 等待进程退出
    Sleep(500);

    // 递归删除目录内容
    WIN32_FIND_DATAW findData;
    std::wstring searchPattern = path + L"\\*";
    HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            std::wstring itemName = findData.cFileName;
            if (itemName == L"." || itemName == L"..") continue;

            std::wstring itemPath = path + L"\\" + itemName;

            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                ForceDeleteDirectory(itemPath);
            } else {
                // 先尝试普通删除
                if (!DeleteFileW(itemPath.c_str())) {
                    // 如果失败，尝试修改文件属性后删除
                    SetFileAttributesW(itemPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                    DeleteFileW(itemPath.c_str());
                }
            }
        } while (FindNextFileW(hFind, &findData));
        FindClose(hFind);
    }

    // 删除空目录
    SetFileAttributesW(path.c_str(), FILE_ATTRIBUTE_NORMAL);
    return RemoveDirectoryW(path.c_str()) != 0;
}

} // namespace IceClean::Utils
