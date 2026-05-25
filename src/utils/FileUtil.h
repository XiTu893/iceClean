#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <windows.h>

namespace IceClean::Utils {

class FileUtil {
public:
    // 递归计算文件夹大小
    static uint64_t GetFolderSize(const std::wstring& path);

    // 递归扫描文件夹中的文件
    static void ScanFiles(const std::wstring& path, const std::wstring& pattern,
                          std::vector<std::wstring>& files, bool recursive = true);

    // 删除文件(到回收站)
    static bool DeleteFileToRecycleBin(const std::wstring& path);

    // 永久删除文件
    static bool DeleteFilePermanently(const std::wstring& path);

    // 递归删除文件夹
    static bool DeleteFolder(const std::wstring& path);

    // 检查文件/文件夹是否存在
    static bool Exists(const std::wstring& path);

    // 检查路径是否为目录
    static bool IsDirectory(const std::wstring& path);

    // 获取文件大小
    static uint64_t GetFileSize(const std::wstring& path);

    // 创建目录(递归)
    static bool CreateDirectoryRecursive(const std::wstring& path);

    // 复制文件夹(递归)
    static bool CopyFolder(const std::wstring& source, const std::wstring& target,
                          std::function<bool(uint64_t, uint64_t)> progressCallback = nullptr);

    // 移动文件夹
    static bool MoveFolder(const std::wstring& source, const std::wstring& target);

    // 获取文件/文件夹数量
    static int GetFileCount(const std::wstring& path, bool recursive = true);
};

} // namespace IceClean::Utils
