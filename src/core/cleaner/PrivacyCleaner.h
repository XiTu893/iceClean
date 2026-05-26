#pragma once
#include "CleanerBase.h"
#include <vector>
#include <string>

namespace IceClean::Core::Cleaner {

// 隐私数据类型
enum class PrivacyType {
    Cookies,
    History,
    FormData
};

// 浏览器隐私数据路径信息
struct BrowserPrivacyPaths {
    std::wstring browserName;
    std::wstring cookiesPath;      // Cookies 文件路径
    std::wstring historyPath;      // History 文件路径
    std::wstring formDataPath;     // Web Data / formhistory.sqlite 路径
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
};

} // namespace IceClean::Core::Cleaner
