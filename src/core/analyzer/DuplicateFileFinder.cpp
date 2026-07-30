// winsock2.h MUST be included before windows.h to avoid redefinition errors
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>

#include "DuplicateFileFinder.h"

#include <windows.h>
#include <wincrypt.h>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <unordered_map>
#include <mutex>

namespace IceClean::Core::Analyzer {

std::vector<Models::DuplicateFileGroup> DuplicateFileFinder::Scan(
    const std::wstring& path,
    std::function<void(const Models::DuplicateScanProgress&)> progress,
    uint64_t minFileSize,
    int maxDepth)
{
    m_cancelled = false;
    std::vector<Models::DuplicateFileGroup> results;

    // 第一步：按文件大小分组
    std::unordered_map<uint64_t, std::vector<std::wstring>> sizeGroups;
    Models::DuplicateScanProgress progressInfo;

    GroupBySize(path, minFileSize, maxDepth, 0, sizeGroups, progressInfo, progress);

    if (m_cancelled) return results;

    // 第二步：对大小相同的文件计算哈希，找出真正重复的
    int totalSameSizeGroups = 0;
    for (const auto& [size, paths] : sizeGroups) {
        if (paths.size() > 1) totalSameSizeGroups++;
    }

    int processedGroups = 0;
    for (const auto& [size, paths] : sizeGroups) {
        if (m_cancelled) break;
        if (paths.size() <= 1) continue;

        processedGroups++;
        progressInfo.duplicateGroups = static_cast<int>(results.size());

        // 对同一大小的文件计算哈希
        std::unordered_map<std::wstring, std::vector<std::wstring>> hashGroups;
        for (const auto& filePath : paths) {
            if (m_cancelled) break;

            progressInfo.currentPath = filePath;
            if (progress) progress(progressInfo);

            auto hash = ComputeFileHash(filePath);
            if (!hash.empty()) {
                hashGroups[hash].push_back(filePath);
            }
        }

        // 哈希相同且路径数>1的为重复文件组
        for (auto& [hash, hashPaths] : hashGroups) {
            if (hashPaths.size() > 1) {
                Models::DuplicateFileGroup group;
                group.fileSize = size;
                group.fileHash = hash;
                group.filePaths = std::move(hashPaths);
                group.wastedSpace = size * (group.filePaths.size() - 1);
                results.push_back(std::move(group));
            }
        }
    }

    // 按浪费空间降序排序
    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.wastedSpace > b.wastedSpace;
    });

    return results;
}

std::vector<Models::DuplicateFileGroup> DuplicateFileFinder::ScanMultiple(
    const std::vector<std::wstring>& paths,
    std::function<void(const Models::DuplicateScanProgress&)> progress,
    uint64_t minFileSize,
    int maxDepth)
{
    std::vector<Models::DuplicateFileGroup> allResults;

    for (const auto& path : paths) {
        if (m_cancelled) break;
        auto results = Scan(path, progress, minFileSize, maxDepth);
        allResults.insert(allResults.end(), results.begin(), results.end());
    }

    // 合并不同路径扫描中的相同哈希组
    std::unordered_map<std::wstring, Models::DuplicateFileGroup> merged;
    for (auto& group : allResults) {
        auto& existing = merged[group.fileHash];
        if (existing.fileHash.empty()) {
            existing = std::move(group);
        } else {
            existing.filePaths.insert(existing.filePaths.end(),
                                       group.filePaths.begin(), group.filePaths.end());
            existing.wastedSpace = existing.fileSize * (existing.filePaths.size() - 1);
        }
    }

    allResults.clear();
    for (auto& [hash, group] : merged) {
        if (group.filePaths.size() > 1) {
            allResults.push_back(std::move(group));
        }
    }

    std::sort(allResults.begin(), allResults.end(), [](const auto& a, const auto& b) {
        return a.wastedSpace > b.wastedSpace;
    });

    return allResults;
}

int DuplicateFileFinder::DeleteDuplicates(
    std::vector<Models::DuplicateFileGroup>& groups,
    const std::vector<int>& groupsToDelete,
    std::function<void(int, int)> progress)
{
    int deletedCount = 0;
    int total = static_cast<int>(groupsToDelete.size());

    for (int i = 0; i < total; ++i) {
        int groupIdx = groupsToDelete[i];
        if (groupIdx < 0 || groupIdx >= static_cast<int>(groups.size())) continue;

        auto& group = groups[groupIdx];
        // 保留第一个文件，删除其余
        for (size_t j = 1; j < group.filePaths.size(); ++j) {
            try {
                std::filesystem::remove(group.filePaths[j]);
                deletedCount++;
            } catch (...) {}
        }

        if (progress) progress(i + 1, total);
    }

    return deletedCount;
}

int DuplicateFileFinder::MoveDuplicates(
    std::vector<Models::DuplicateFileGroup>& groups,
    const std::vector<int>& groupsToMove,
    const std::wstring& targetDir,
    std::function<void(int, int)> progress)
{
    int movedCount = 0;
    int total = static_cast<int>(groupsToMove.size());

    namespace fs = std::filesystem;
    fs::create_directories(targetDir);

    for (int i = 0; i < total; ++i) {
        int groupIdx = groupsToMove[i];
        if (groupIdx < 0 || groupIdx >= static_cast<int>(groups.size())) continue;

        auto& group = groups[groupIdx];
        for (size_t j = 1; j < group.filePaths.size(); ++j) {
            try {
                auto filename = fs::path(group.filePaths[j]).filename();
                auto destPath = fs::path(targetDir) / filename;

                // 避免目标文件已存在
                int counter = 1;
                while (fs::exists(destPath)) {
                    destPath = fs::path(targetDir) /
                        (filename.stem().wstring() + L"_" + std::to_wstring(counter) + filename.extension().wstring());
                    counter++;
                }

                fs::rename(group.filePaths[j], destPath);
                movedCount++;
            } catch (...) {}
        }

        if (progress) progress(i + 1, total);
    }

    return movedCount;
}

uint64_t DuplicateFileFinder::GetTotalWastedSpace(const std::vector<Models::DuplicateFileGroup>& groups) {
    uint64_t total = 0;
    for (const auto& group : groups) {
        total += group.wastedSpace;
    }
    return total;
}

void DuplicateFileFinder::GroupBySize(
    const std::wstring& path, uint64_t minFileSize, int maxDepth, int currentDepth,
    std::unordered_map<uint64_t, std::vector<std::wstring>>& sizeGroups,
    Models::DuplicateScanProgress& progressInfo,
    std::function<void(const Models::DuplicateScanProgress&)>& progress)
{
    namespace fs = std::filesystem;

    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (m_cancelled) return;

            if (entry.is_regular_file()) {
                auto fileSize = entry.file_size();
                if (fileSize >= minFileSize) {
                    sizeGroups[fileSize].push_back(entry.path().wstring());
                }
                progressInfo.scannedFiles++;
                if (progressInfo.scannedFiles % 500 == 0 && progress) {
                    progressInfo.currentPath = entry.path().wstring();
                    progress(progressInfo);
                }
            } else if (entry.is_directory() && (maxDepth < 0 || currentDepth < maxDepth)) {
                // 跳过系统关键目录
                auto dirName = entry.path().filename().wstring();
                std::wstring lowerDir = dirName;
                std::transform(lowerDir.begin(), lowerDir.end(), lowerDir.begin(), ::towlower);

                if (lowerDir == L"windows" || lowerDir == L"$recycle.bin" ||
                    lowerDir == L"system volume information" || lowerDir == L"programdata") {
                    continue;
                }

                GroupBySize(entry.path().wstring(), minFileSize, maxDepth, currentDepth + 1,
                            sizeGroups, progressInfo, progress);
            }
        }
    } catch (...) {}
}

std::wstring DuplicateFileFinder::ComputeFileHash(const std::wstring& filePath) {
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return L"";

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;

    if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        CloseHandle(hFile);
        return L"";
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);
        return L"";
    }

    constexpr DWORD BUFFER_SIZE = 65536;
    auto buffer = std::make_unique<BYTE[]>(BUFFER_SIZE);
    DWORD bytesRead = 0;

    // 对于大文件，只读前1MB来加速
    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);
    const int64_t maxReadSize = 1024 * 1024; // 1MB
    int64_t totalRead = 0;

    while (ReadFile(hFile, buffer.get(), BUFFER_SIZE, &bytesRead, nullptr) && bytesRead > 0) {
        if (!CryptHashData(hHash, buffer.get(), bytesRead, 0)) break;
        totalRead += bytesRead;
        if (fileSize.QuadPart > maxReadSize * 2 && totalRead >= maxReadSize) break;
    }

    DWORD hashLen = 32;
    BYTE hashData[32] = {};
    wchar_t hashStr[65] = {};

    if (CryptGetHashParam(hHash, HP_HASHVAL, hashData, &hashLen, 0)) {
        for (DWORD i = 0; i < hashLen; ++i) {
            swprintf_s(&hashStr[i * 2], 3, L"%02x", hashData[i]);
        }
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

    return hashStr;
}

} // namespace IceClean::Core::Analyzer
