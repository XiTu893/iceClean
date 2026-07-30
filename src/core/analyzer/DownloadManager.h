#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace IceClean::Core::Analyzer {

struct DownloadItem {
    std::wstring path;
    std::wstring name;
    std::wstring extension;
    uint64_t size = 0;
    FILETIME lastAccessTime;
    FILETIME lastWriteTime;
    std::wstring category;
    bool isInstaller = false;
    bool isOld = false;       // 超过设定天数未访问
};

struct DownloadCleanResult {
    int cleanedCount = 0;
    int movedCount = 0;
    uint64_t freedBytes = 0;
    std::vector<std::wstring> failedItems;
};

class DownloadManager {
public:
    // 扫描下载文件夹
    std::vector<DownloadItem> ScanDownloads(const std::wstring& downloadPath);

    // 获取安装包类文件（exe/msi/iso/zip 可能为安装包）
    std::vector<DownloadItem> GetInstallers(const std::vector<DownloadItem>& items);

    // 获取长时间未访问文件（> days 天）
    std::vector<DownloadItem> GetInactiveFiles(const std::vector<DownloadItem>& items, int days);

    // 按类别分组
    std::vector<std::pair<std::wstring, std::vector<DownloadItem>>> GroupByCategory(
        const std::vector<DownloadItem>& items);

    // 删除指定文件
    DownloadCleanResult CleanItems(const std::vector<DownloadItem>& items,
                                   std::function<void(int, int)> progressCallback = nullptr);

    // 移动文件到目标路径
    DownloadCleanResult MoveItems(const std::vector<DownloadItem>& items,
                                   const std::wstring& targetPath,
                                   std::function<void(int, int)> progressCallback = nullptr);

    // 获取文件分类
    static std::wstring GetFileCategory(const std::wstring& extension);

    // 判断是否为安装包
    static bool IsInstallerFile(const std::wstring& extension);

private:
    bool IsFileInUse(const std::wstring& path) const;
};

} // namespace IceClean::Core::Analyzer
