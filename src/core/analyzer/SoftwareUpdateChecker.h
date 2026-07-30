#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace IceClean::Core::Analyzer {

// 可升级软件信息
struct UpdatableSoftware {
    std::wstring name;           // 软件名称
    std::wstring currentVersion; // 当前版本
    std::wstring latestVersion;  // 最新版本
    std::wstring downloadUrl;    // 下载链接
    std::wstring publisher;      // 发布者
};

// 软件升级检测器
class SoftwareUpdateChecker {
public:
    // 检查可升级软件
    std::vector<UpdatableSoftware> CheckUpdates();

private:
    // 从本地JSON配置加载已知软件版本信息
    std::vector<UpdatableSoftware> LoadKnownVersions();

    // 比较版本号（返回: 1表示v1>v2, -1表示v1<v2, 0表示相等）
    static int CompareVersions(const std::wstring& v1, const std::wstring& v2);
};

} // namespace IceClean::Core::Analyzer
