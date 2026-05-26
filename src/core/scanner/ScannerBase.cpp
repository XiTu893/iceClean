#include "ScannerBase.h"
#include <algorithm>

namespace IceClean::Core::Scanner {

uint64_t ScannerBase::CalculateTotalSize(const std::vector<Models::ScanFileItem>& items) {
    uint64_t total = 0;
    for (const auto& item : items) {
        total += item.size;
    }
    return total;
}

bool ScannerBase::ShouldStop(const std::atomic<bool>* stopFlag) {
    return stopFlag && stopFlag->load(std::memory_order_acquire);
}

void ScannerBase::ScanDirectory(const std::wstring& directoryPath,
                                 const std::wstring& pattern,
                                 bool recursive,
                                 bool skipLocked,
                                 Models::ScanCategory& category,
                                 const std::atomic<bool>* stopFlag,
                                 ScanProgressCallback progressCb) {
    // 检查停止标志
    if (ShouldStop(stopFlag)) return;

    // 展开环境变量
    std::wstring expandedPath = Utils::Win32Util::ExpandEnvVars(directoryPath);

    // 检查目录是否存在
    if (!Utils::FileUtil::Exists(expandedPath)) return;
    if (!Utils::FileUtil::IsDirectory(expandedPath)) return;

    // 构建搜索路径
    std::wstring searchPath = expandedPath;
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath += L'\\';
    }
    searchPath += pattern;

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    int filesScanned = 0;

    do {
        // 每次迭代都检查停止标志（atomic load 开销极小）
        if (ShouldStop(stopFlag)) {
            FindClose(hFind);
            return;
        }

        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring fullPath = expandedPath;
        if (!fullPath.empty() && fullPath.back() != L'\\') {
            fullPath += L'\\';
        }
        fullPath += findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 递归扫描子目录
            if (recursive &&
                !(findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                ScanDirectory(fullPath, pattern, recursive, skipLocked, category, stopFlag, progressCb);
            }
        } else {
            // 跳过白名单路径
            if (IsWhitelisted(fullPath)) continue;

            // 跳过被锁定的文件
            if (skipLocked && IsFileLocked(fullPath)) continue;

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

            filesScanned++;

            // 每10个文件调用一次进度回调，传递当前文件路径
            if (progressCb && (filesScanned % 10 == 0)) {
                progressCb(filesScanned, fullPath);
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);

    // 最终进度回调（处理不足10个文件的尾部）
    if (progressCb && filesScanned > 0 && (filesScanned % 10 != 0)) {
        progressCb(filesScanned, L"");
    }
}

bool ScannerBase::IsWhitelisted(const std::wstring& path) const {
    return Safety::WhitelistProvider::IsWhitelisted(path);
}

bool ScannerBase::IsFileLocked(const std::wstring& path) {
    // 尝试以独占方式打开文件，如果失败则说明文件被锁定
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        0,  // 独占访问
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        // 无法打开 - 文件可能被锁定
        DWORD err = GetLastError();
        return (err == ERROR_SHARING_VIOLATION ||
                err == ERROR_LOCK_VIOLATION ||
                err == ERROR_ACCESS_DENIED);
    }

    CloseHandle(hFile);
    return false;
}

} // namespace IceClean::Core::Scanner
