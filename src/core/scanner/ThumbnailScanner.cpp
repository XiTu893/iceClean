#include "ThumbnailScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <shlobj.h>

namespace IceClean::Core::Scanner {

Models::ScanCategory ThumbnailScanner::Scan() {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    // 扫描 %LocalAppData%\Microsoft\Windows\Explorer\thumbcache_*
    std::wstring explorerPath = Utils::Win32Util::GetSpecialFolder(CSIDL_LOCAL_APPDATA);
    if (!explorerPath.empty()) {
        std::wstring thumbPath = explorerPath + L"\\Microsoft\\Windows\\Explorer";
        if (Utils::FileUtil::Exists(thumbPath)) {
            // 扫描 thumbcache_* 文件
            std::wstring searchPath = thumbPath;
            if (!searchPath.empty() && searchPath.back() != L'\\') {
                searchPath += L'\\';
            }
            searchPath += L"thumbcache_*";

            WIN32_FIND_DATAW findData{};
            HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        std::wstring fullPath = thumbPath + L"\\" + findData.cFileName;

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

            // 同时扫描 iconcache_* 文件
            std::wstring iconSearchPath = thumbPath + L"\\iconcache_*";
            hFind = FindFirstFileW(iconSearchPath.c_str(), &findData);
            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                        std::wstring fullPath = thumbPath + L"\\" + findData.cFileName;

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
    }

    return category;
}

bool ThumbnailScanner::IsAvailable() const {
    std::wstring localAppData = Utils::Win32Util::GetSpecialFolder(CSIDL_LOCAL_APPDATA);
    if (localAppData.empty()) return false;
    std::wstring thumbPath = localAppData + L"\\Microsoft\\Windows\\Explorer";
    return Utils::FileUtil::Exists(thumbPath);
}

} // namespace IceClean::Core::Scanner
