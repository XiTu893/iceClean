#include "PnpUtilRunner.h"
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <thread>

namespace IceClean::Core::Driver {

namespace {

std::wstring ToWideFromOem(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_OEMCP, 0, s.c_str(), static_cast<int>(s.size()),
                                      nullptr, 0);
    if (n == 0) return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_OEMCP, 0, s.c_str(), static_cast<int>(s.size()),
                        out.data(), n);
    return out;
}

std::wstring Trim(const std::wstring& s) {
    const size_t b = s.find_first_not_of(L" \t\r\n");
    if (b == std::wstring::npos) return {};
    const size_t e = s.find_last_not_of(L" \t\r\n");
    return s.substr(b, e - b + 1);
}

// 双语键匹配：给定一行与若干别名（英文/中文），命中返回 true
bool MatchKey(const std::wstring& line, const std::vector<std::wstring>& aliases,
              std::wstring& value) {
    const size_t colon = line.find(L':');
    if (colon == std::wstring::npos) return false;
    std::wstring key = Trim(line.substr(0, colon));
    std::wstring lowerKey = key;
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::towlower);
    for (const auto& alias : aliases) {
        std::wstring a = alias;
        std::transform(a.begin(), a.end(), a.begin(), ::towlower);
        if (lowerKey == a || lowerKey.find(a) != std::wstring::npos) {
            value = Trim(line.substr(colon + 1));
            return true;
        }
    }
    return false;
}

} // namespace

std::wstring PnpUtilRunner::GetPnpUtilPath() {
    wchar_t sysDir[MAX_PATH] = {};
    if (GetSystemDirectoryW(sysDir, MAX_PATH) == 0) {
        return L"C:\\Windows\\System32\\pnputil.exe";
    }
    return std::wstring(sysDir) + L"\\pnputil.exe";
}

bool PnpUtilRunner::Run(const std::wstring& args, std::wstring& output, DWORD timeoutMs) {
    const std::wstring cmd = L"\"" + GetPnpUtilPath() + L"\" " + args;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                        TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }
    CloseHandle(hWrite);

    std::string raw;
    char buf[4096]{};
    DWORD read = 0;
    while (ReadFile(hRead, buf, sizeof(buf), &read, nullptr) && read > 0) {
        raw.append(buf, read);
    }
    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, timeoutMs);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    output = ToWideFromOem(raw);
    return true;
}

std::vector<DriverPackageInfo> PnpUtilRunner::ParseEnumOutput(const std::wstring& output) {
    std::vector<DriverPackageInfo> packages;
    DriverPackageInfo cur;
    bool hasCur = false;

    auto flush = [&]() {
        if (hasCur && !cur.publishedName.empty()) {
            packages.push_back(cur);
        }
        cur = DriverPackageInfo{};
        hasCur = false;
    };

    std::wistringstream iss(output);
    std::wstring line;
    while (std::getline(iss, line)) {
        std::wstring value;
        if (MatchKey(line, {L"Published Name", L"发布名称"}, value)) {
            if (hasCur) flush();
            cur.publishedName = value;
            hasCur = true;
        } else if (MatchKey(line, {L"Original Name", L"原始名称"}, value)) {
            cur.originalName = value;
        } else if (MatchKey(line, {L"Provider Name", L"提供商名称", L"Driver Provider", L"驱动提供商"}, value)) {
            cur.provider = value;
        } else if (MatchKey(line, {L"Class Name", L"类名", L"类别名称"}, value)) {
            cur.className = value;
        } else if (MatchKey(line, {L"Driver Version", L"驱动版本"}, value)) {
            cur.version = value;
        } else if (MatchKey(line, {L"Driver Date", L"驱动日期"}, value)) {
            cur.date = value;
        } else if (MatchKey(line, {L"Signer Name", L"签名者", L"签名"}, value)) {
            cur.signedDriver = !value.empty() && value != L"-";
        }
    }
    flush();
    return packages;
}

std::vector<DriverPackageInfo> PnpUtilRunner::EnumDrivers() {
    std::wstring output;
    if (!Run(L"/enum-drivers", output)) {
        return {};
    }
    return ParseEnumOutput(output);
}

bool PnpUtilRunner::ExportAll(const std::wstring& destDir,
                              std::function<void(int, int, const std::wstring&)> progress) {
    if (!std::filesystem::exists(destDir)) return false;

    const std::wstring args = L"/export-driver * \"" + destDir + L"\"";

    // 预估计总数：先枚举一次
    int estimated = 0;
    if (progress) {
        estimated = static_cast<int>(EnumDrivers().size());
    }

    // 使用管道 + 后台轮询目标目录增量文件数，实现 ActivityTicker 式进度
    const std::wstring cmd = L"\"" + GetPnpUtilPath() + L"\" " + args;

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, const_cast<LPWSTR>(cmd.c_str()), nullptr, nullptr,
                        TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return false;
    }
    CloseHandle(hWrite);

    // 丢弃输出（仍在后台线程读取以防管道阻塞）
    std::thread reader([hRead]() {
        char buf[4096]{};
        DWORD read = 0;
        while (ReadFile(hRead, buf, sizeof(buf), &read, nullptr) && read > 0) {}
        CloseHandle(hRead);
    });

    // 轮询目标目录 .inf 数量作为进度
    int lastCount = 0;
    std::wstring lastFile;
    while (true) {
        DWORD code = 0;
        if (GetExitCodeProcess(pi.hProcess, &code) && code != STILL_ACTIVE) break;
        if (progress) {
            int count = 0;
            std::error_code ec;
            for (auto it = std::filesystem::recursive_directory_iterator(destDir, ec);
                 it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
                if (ec) break;
                if (it->path().extension().wstring() == L".inf") {
                    count++;
                    lastFile = it->path().filename().wstring();
                }
            }
            if (count != lastCount) {
                lastCount = count;
                progress(count, estimated, lastFile);
            }
        }
        Sleep(500);
    }

    if (reader.joinable()) reader.join();
    WaitForSingleObject(pi.hProcess, 5000);
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (progress) progress(lastCount, estimated, lastFile);
    return exitCode == 0;
}

bool PnpUtilRunner::ExportOne(const std::wstring& oemInf, const std::wstring& destDir) {
    if (!std::filesystem::exists(destDir)) return false;
    const std::wstring args = L"/export-driver " + oemInf + L" \"" + destDir + L"\"";
    std::wstring output;
    return Run(args, output);
}

int PnpUtilRunner::AddDrivers(const std::wstring& sourceDir) {
    const std::wstring args = L"/add-driver \"" + sourceDir + L"\\*.inf\" /subdirs /install";
    std::wstring output;
    if (!Run(args, output, 300000)) {
        return -1;
    }
    return 0;
}

} // namespace IceClean::Core::Driver
