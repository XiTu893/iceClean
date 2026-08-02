#pragma once
#include "CleanerBase.h"
#include <vector>
#include <string>

namespace IceClean::Core::Cleaner {

// 隐私数据类型
enum class PrivacyType {
    Cookies,
    History,
    FormData,
    Cache,              // 浏览器缓存
    Session,            // 会话/标签页数据
    Passwords,          // 保存的密码（谨慎）
    RecentDocs,         // 最近文档记录
    RunHistory,         // 运行对话框历史
    SearchHistory,      // 搜索历史
    ClipboardHistory,   // 剪贴板历史
    JumpList,           // 跳转列表
    ThumbnailCache,     // 缩略图缓存隐私
    OfficeRecent,       // Office最近文件
    ArchiveHistory,     // 压缩软件历史(WinRAR/7-Zip)
    DownloadHistory,    // 下载器历史(迅雷/IDM)
};

// 浏览器隐私数据路径信息
struct BrowserPrivacyPaths {
    std::wstring browserName;
    std::wstring cookiesPath;      // Cookies 文件路径
    std::wstring historyPath;      // History 文件路径
    std::wstring formDataPath;     // Web Data / formhistory.sqlite 路径
    std::wstring cachePath;        // 缓存目录路径
    std::wstring sessionPath;      // 会话数据路径(Current Session/Current Tabs)
    std::wstring loginDataPath;    // Login Data 路径(保存的密码)
    std::wstring downloadPath;     // 下载历史路径
    std::wstring processName;      // 浏览器进程名（用于检测是否运行）
};

class PrivacyCleaner : public CleanerBase {
public:
    PrivacyCleaner();

    // 实现 ICleaner 接口
    std::wstring GetName() const override { return L"隐私数据清理"; }
    Models::CleanResult Clean(const std::vector<std::wstring>& paths,
                               std::function<void(const Models::CleanProgress&)> progressCallback = nullptr,
                               const std::atomic<bool>* cancelFlag = nullptr) override;

    // 清理指定类型的隐私数据
    Models::CleanResult CleanPrivacy(const std::vector<PrivacyType>& privacyTypes,
                                      std::function<void(const Models::CleanProgress&)> progressCb = nullptr);

private:
    std::vector<BrowserPrivacyPaths> GetBrowserPaths() const;
    bool IsBrowserRunning(const std::wstring& processName) const;
    bool DeletePrivacyFile(const std::wstring& path);

    // 获取系统隐私数据文件路径列表
    std::vector<std::wstring> GetSystemPrivacyPaths(PrivacyType type) const;

    // 获取应用程序隐私数据文件路径列表
    std::vector<std::wstring> GetAppPrivacyPaths(PrivacyType type) const;
};

} // namespace IceClean::Core::Cleaner
