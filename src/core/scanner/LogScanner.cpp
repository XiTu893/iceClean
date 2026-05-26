#include "LogScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Scanner {

Models::ScanCategory LogScanner::Scan(const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    // 扫描 C:\Windows\Logs
    std::wstring logsPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\Logs");
    if (Utils::FileUtil::Exists(logsPath)) {
        ScanDirectory(logsPath, L"*.log", true, true, category, stopFlag, progressCb);
        if (ShouldStop(stopFlag)) return category;
        ScanDirectory(logsPath, L"*.etl", true, true, category, stopFlag, progressCb);
        if (ShouldStop(stopFlag)) return category;
    }

    // 扫描 Windows 事件日志 .evtx 文件
    // 注意：.evtx 文件通常被系统锁定，但我们仍然报告大小
    std::wstring winevtPath = L"C:\\Windows\\System32\\winevt\\Logs";
    if (Utils::FileUtil::Exists(winevtPath) && !ShouldStop(stopFlag)) {
        // 扫描 .evtx 文件（不跳过锁定文件，因为需要报告大小）
        std::wstring searchPath = winevtPath;
        if (!searchPath.empty() && searchPath.back() != L'\\') {
            searchPath += L'\\';
        }
        searchPath += L"*.evtx";

        WIN32_FIND_DATAW findData{};
        HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (ShouldStop(stopFlag)) {
                    FindClose(hFind);
                    return category;
                }
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring fullPath = winevtPath + L"\\" + findData.cFileName;

                    Models::ScanFileItem item;
                    item.path = fullPath;

                    ULARGE_INTEGER fileSize;
                    fileSize.LowPart = findData.nFileSizeLow;
                    fileSize.HighPart = findData.nFileSizeHigh;
                    item.size = fileSize.QuadPart;

                    item.lastWriteTime = findData.ftLastWriteTime;
                    item.selected = true;

                    category.items.push_back(item);
                    category.totalSize += item.size;
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }

    // 扫描 CBS 日志
    std::wstring cbsPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\Logs\\CBS");
    if (Utils::FileUtil::Exists(cbsPath) && !ShouldStop(stopFlag)) {
        ScanDirectory(cbsPath, L"*.log", false, true, category, stopFlag, progressCb);
    }

    // 扫描 DISM 日志
    std::wstring dismPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\Logs\\DISM");
    if (Utils::FileUtil::Exists(dismPath) && !ShouldStop(stopFlag)) {
        ScanDirectory(dismPath, L"*.log", false, true, category, stopFlag, progressCb);
    }

    return category;
}

bool LogScanner::IsAvailable() const {
    std::wstring logsPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\Logs");
    return Utils::FileUtil::Exists(logsPath);
}

} // namespace IceClean::Core::Scanner
