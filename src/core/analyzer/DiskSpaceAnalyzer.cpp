#include "DiskSpaceAnalyzer.h"
#include <algorithm>

namespace IceClean::Core::Analyzer {

DiskSpaceAnalyzer::DiskSpaceAnalyzer() = default;

const std::vector<std::wstring>& DiskSpaceAnalyzer::GetProtectedFolderNames() {
    static const std::vector<std::wstring> names = {
        L"system volume information",
        L"$recycle.bin",
        L"recovery",
        L"system32",
        L"syswow64",
    };
    return names;
}

bool DiskSpaceAnalyzer::ShouldSkip(const std::wstring& folderName, DWORD attributes) const {
    // 跳过联接链接和符号链接
    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        return true;
    }

    // 跳过系统保护目录
    std::wstring lowerName = folderName;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), towlower);

    for (const auto& skip : GetProtectedFolderNames()) {
        if (lowerName == skip) return true;
    }

    return false;
}

void DiskSpaceAnalyzer::ScanDirectory(const std::wstring& path,
                                        Models::DiskNode& parentNode,
                                        std::function<void(const ScanProgress&)>& progressCallback,
                                        ScanProgress& progress) {
    if (cancelled_) return;

    std::wstring searchPath = path;
    if (!searchPath.empty() && searchPath.back() != L'\\') {
        searchPath += L'\\';
    }
    searchPath += L"*";

    WIN32_FIND_DATAW findData{};
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (cancelled_) break;

        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0) {
            continue;
        }

        std::wstring fullPath = path;
        if (!fullPath.empty() && fullPath.back() != L'\\') fullPath += L'\\';
        fullPath += findData.cFileName;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // 跳过保护目录
            if (ShouldSkip(findData.cFileName, findData.dwFileAttributes)) {
                continue;
            }

            auto childNode = std::make_shared<Models::DiskNode>();
            childNode->name = findData.cFileName;
            childNode->fullPath = fullPath;
            childNode->isDirectory = true;
            childNode->size = 0;

            // 递归扫描子目录
            ScanDirectory(fullPath, *childNode, progressCallback, progress);

            // 计算子目录总大小
            childNode->size = childNode->GetTotalSize();

            parentNode.children.push_back(childNode);
            ++progress.scannedDirs;
        } else {
            ULARGE_INTEGER fileSize;
            fileSize.LowPart = findData.nFileSizeLow;
            fileSize.HighPart = findData.nFileSizeHigh;

            auto childNode = std::make_shared<Models::DiskNode>();
            childNode->name = findData.cFileName;
            childNode->fullPath = fullPath;
            childNode->isDirectory = false;
            childNode->size = fileSize.QuadPart;

            parentNode.children.push_back(childNode);
            ++progress.scannedFiles;
        }

        // 定期报告进度
        if (progressCallback && (progress.scannedDirs + progress.scannedFiles) % 100 == 0) {
            progress.currentPath = fullPath;
            progress.isRunning = true;
            progressCallback(progress);
        }

    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
}

std::shared_ptr<Models::DiskNode> DiskSpaceAnalyzer::Scan(
    const std::wstring& path,
    std::function<void(const ScanProgress&)> progressCallback) {

    cancelled_ = false;

    auto rootNode = std::make_shared<Models::DiskNode>();
    rootNode->name = path;
    rootNode->fullPath = path;
    rootNode->isDirectory = true;
    rootNode->size = 0;

    ScanProgress progress;
    progress.currentPath = path;
    progress.isRunning = true;

    ScanDirectory(path, *rootNode, progressCallback, progress);

    // 计算根节点总大小
    rootNode->size = rootNode->GetTotalSize();

    if (!cancelled_) {
        result_ = rootNode;
    }

    return rootNode;
}

void DiskSpaceAnalyzer::Cancel() {
    cancelled_ = true;
}

std::shared_ptr<Models::DiskNode> DiskSpaceAnalyzer::GetResult() const {
    return result_;
}

} // namespace IceClean::Core::Analyzer
