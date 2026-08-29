#pragma once
#include "IMigrator.h"
#include <cstdint>
#include <functional>
#include <vector>
#include <atomic>
#include <windows.h>

namespace IceClean::Core::Migrator {

class LargeFolderDetector {
public:
    // 进度回调：当前遍历目录 / 已发现条目数 / 已发现累计字节
    using ProgressCallback = std::function<void(const std::wstring& path,
                                                int foundCount,
                                                uint64_t foundBytes)>;

    // 构造函数
    // minSizeMB: 最小文件夹大小阈值(MB)，默认500MB
    explicit LargeFolderDetector(uint64_t minSizeMB = 500);

    // 检测系统盘上的大文件夹（等价于 DetectAt(GetSystemDrive())）
    std::vector<Models::MigrationItem> Detect(ProgressCallback progressCallback = nullptr);

    // 检测指定根目录下的大文件夹（可注入根目录，供测试/定向扫描使用）
    std::vector<Models::MigrationItem> DetectAt(const std::wstring& rootPath,
                                                ProgressCallback progressCallback = nullptr);

    // 取消检测
    void Cancel();

private:
    uint64_t minSizeBytes_;
    std::atomic<bool> cancelled_{false};

    // 需要跳过的系统文件夹名称(小写)
    static const std::vector<std::wstring>& GetSkippedFolderNames();

    // 判断文件夹是否应该跳过
    bool ShouldSkip(const std::wstring& folderName, DWORD attributes) const;

    // 递归统计目录大小，每进入一个子目录即通过 cb 上报当前路径与累计字节
    // （流式测量：替代一次性 GetFolderSize，让 UI 在大目录统计期间也有持续细节反馈）
    // items: 已发现条目（用于上报全局计数/字节）；baseBytes: items 字节 + 外层已累计
    // outTotal: 返回本目录子树的合计字节
    void MeasureDirectory(const std::wstring& dir,
                          const std::vector<Models::MigrationItem>& items,
                          uint64_t baseBytes,
                          ProgressCallback& cb,
                          uint64_t& outTotal);

    static uint64_t SumItemBytes(const std::vector<Models::MigrationItem>& items);

    // 递归扫描目录
    void ScanDirectory(const std::wstring& path,
                       std::vector<Models::MigrationItem>& results,
                       ProgressCallback& progressCallback);
};

} // namespace IceClean::Core::Migrator
