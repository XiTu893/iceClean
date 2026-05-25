#include "FileUtil.h"
#include <shlobj.h>
#include <algorithm>

namespace IceClean::Utils {

uint64_t FileUtil::GetFolderSize(const std::wstring& path) {
    uint64_t totalSize = 0;

    std::wstring searchPath = path;
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath += L'\\';
    }
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring fullPath = path;
        if (!fullPath.empty() && fullPath.back() != L'\\') {
            fullPath += L'\\';
        }
        fullPath += findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 跳过联接点和符号链接以避免无限递归
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                totalSize += GetFolderSize(fullPath);
            }
        } else {
            ULARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            totalSize += fileSize.QuadPart;
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return totalSize;
}

void FileUtil::ScanFiles(const std::wstring& path, const std::wstring& pattern,
                         std::vector<std::wstring>& files, bool recursive) {
    std::wstring searchPath = path;
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath += L'\\';
    }
    searchPath += pattern;

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring fullPath = path;
        if (!fullPath.empty() && fullPath.back() != L'\\') {
            fullPath += L'\\';
        }
        fullPath += findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive &&
                !(findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                ScanFiles(fullPath, pattern, files, recursive);
            }
        } else {
            files.push_back(fullPath);
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

bool FileUtil::DeleteFileToRecycleBin(const std::wstring& path) {
    // SHFileOperation要求路径以双\0结尾
    std::wstring doubleNullPath = path + L'\0';

    SHFILEOPSTRUCTW op = {};
    op.wFunc = FO_DELETE;
    op.pFrom = doubleNullPath.c_str();
    op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;

    int result = SHFileOperationW(&op);
    return result == 0;
}

bool FileUtil::DeleteFilePermanently(const std::wstring& path) {
    if (path.empty()) return false;

    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;

    // 移除只读属性
    if (attrs & FILE_ATTRIBUTE_READONLY) {
        SetFileAttributesW(path.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
    }

    if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
        return DeleteFolder(path);
    }

    return DeleteFileW(path.c_str()) != 0;
}

bool FileUtil::DeleteFolder(const std::wstring& path) {
    // 先删除子文件和子目录
    std::wstring searchPath = path;
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath += L'\\';
    }
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        // 目录可能已经为空或不存在，尝试直接删除
        return RemoveDirectoryW(path.c_str()) != 0;
    }

    do {
        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring fullPath = path;
        if (!fullPath.empty() && fullPath.back() != L'\\') {
            fullPath += L'\\';
        }
        fullPath += findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
            SetFileAttributesW(fullPath.c_str(),
                findData.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
        }

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            DeleteFolder(fullPath);
        } else {
            DeleteFileW(fullPath.c_str());
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return RemoveDirectoryW(path.c_str()) != 0;
}

bool FileUtil::Exists(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
}

bool FileUtil::IsDirectory(const std::wstring& path) {
    DWORD attrs = GetFileAttributesW(path.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

uint64_t FileUtil::GetFileSize(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA attrData{};
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attrData)) {
        return 0;
    }

    ULARGE_INTEGER fileSize;
    fileSize.LowPart = attrData.nFileSizeLow;
    fileSize.HighPart = attrData.nFileSizeHigh;
    return fileSize.QuadPart;
}

bool FileUtil::CreateDirectoryRecursive(const std::wstring& path) {
    if (path.empty()) return false;
    if (Exists(path)) return IsDirectory(path);

    // 找到父目录
    std::wstring parent = path;
    while (!parent.empty() && parent.back() == L'\\') {
        parent.pop_back();
    }

    auto lastSlash = parent.find_last_of(L'\\');
    if (lastSlash != std::wstring::npos && lastSlash > 0) {
        parent = parent.substr(0, lastSlash);
        if (!Exists(parent)) {
            CreateDirectoryRecursive(parent);
        }
    }

    return CreateDirectoryW(path.c_str(), nullptr) != 0 ||
           GetLastError() == ERROR_ALREADY_EXISTS;
}

bool FileUtil::CopyFolder(const std::wstring& source, const std::wstring& target,
                          std::function<bool(uint64_t, uint64_t)> progressCallback) {
    if (!Exists(source)) return false;

    // 创建目标目录
    if (!Exists(target)) {
        if (!CreateDirectoryRecursive(target)) return false;
    }

    uint64_t totalCopied = 0;
    uint64_t totalSize = GetFolderSize(source);

    std::wstring searchPath = source;
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath += L'\\';
    }
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    bool success = true;

    do {
        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring srcPath = source;
        if (!srcPath.empty() && srcPath.back() != L'\\') srcPath += L'\\';
        srcPath += findData.cFileName;

        std::wstring dstPath = target;
        if (!dstPath.empty() && dstPath.back() != L'\\') dstPath += L'\\';
        dstPath += findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                if (!CopyFolder(srcPath, dstPath, progressCallback)) {
                    success = false;
                    break;
                }
            }
        } else {
            if (!CopyFileW(srcPath.c_str(), dstPath.c_str(), FALSE)) {
                success = false;
                break;
            }

            ULARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;
            totalCopied += fileSize.QuadPart;

            if (progressCallback) {
                if (!progressCallback(totalCopied, totalSize)) {
                    success = false;
                    break;
                }
            }
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return success;
}

bool FileUtil::MoveFolder(const std::wstring& source, const std::wstring& target) {
    // 先尝试直接重命名(同盘移动最快)
    if (MoveFileW(source.c_str(), target.c_str())) {
        return true;
    }

    // 跨盘移动：先复制再删除
    if (!CopyFolder(source, target)) {
        return false;
    }

    return DeleteFolder(source);
}

int FileUtil::GetFileCount(const std::wstring& path, bool recursive) {
    int count = 0;

    std::wstring searchPath = path;
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath += L'\\';
    }
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;

    do {
        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (recursive &&
                !(findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                std::wstring subPath = path;
                if (!subPath.empty() && subPath.back() != L'\\') subPath += L'\\';
                subPath += findData.cFileName;
                count += GetFileCount(subPath, recursive);
            }
        } else {
            ++count;
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return count;
}

} // namespace IceClean::Utils
