#pragma once
#include "models/InstalledSoftware.h"
#include <vector>
#include <string>
#include <functional>
#include <windows.h>

namespace IceClean::Core::Cleaner {

// 软件安装来源
enum class InstallSource {
    Registry,       // 注册表卸载项
    Store,          // Windows Store (UWP)
    Steam,          // Steam 游戏
    Portable,       // 便携软件检测
    Chocolatey,     // Chocolatey 包管理器
};

struct AppSourceInfo {
    InstallSource source;
    std::wstring identifier;
    std::wstring displayName;
    std::wstring installLocation;
    uint64_t estimatedSize = 0;
};

class SoftwareUninstaller {
public:
    // 获取所有已安装软件列表
    std::vector<IceClean::Models::InstalledSoftware> GetInstalledSoftware();

    // 从多种来源检测已安装软件
    std::vector<AppSourceInfo> GetAllApps();

    // 获取 Windows Store (UWP) 应用
    std::vector<AppSourceInfo> GetStoreApps();

    // 卸载 UWP 应用
    bool UninstallStoreApp(const std::wstring& packageFullName);

    // 卸载指定软件
    bool Uninstall(const IceClean::Models::InstalledSoftware& software);

    // 静默卸载（如果支持 /S 或 /silent 参数）
    bool SilentUninstall(const IceClean::Models::InstalledSoftware& software);

    // 检查软件是否正在运行
    bool IsSoftwareRunning(const std::wstring& installLocation) const;

    // 清理卸载残留
    std::vector<std::wstring> ScanResidualFiles(const IceClean::Models::InstalledSoftware& software);

    // 删除残留文件
    bool CleanResidual(const std::vector<std::wstring>& residualPaths);

    // 强制卸载
    struct ForceUninstallResult {
        bool success = false;
        bool processKilled = false;
        bool directoryRemoved = false;
        bool registryCleaned = false;
        std::wstring message;
    };
    ForceUninstallResult ForceUninstall(const IceClean::Models::InstalledSoftware& software);

private:
    // 从注册表读取安装信息
    void ReadUninstallEntries(HKEY rootKey, const std::wstring& subKey,
                              std::vector<IceClean::Models::InstalledSoftware>& items);

    // 解析估计大小
    uint64_t ParseEstimatedSize(const std::wstring& sizeStr) const;

    // 格式化安装日期
    std::wstring FormatInstallDate(const std::wstring& rawDate) const;

    // 检查是否为系统组件
    bool IsSystemComponent(HKEY rootKey, const std::wstring& subKey) const;

    // 检查是否为Windows更新
    bool IsWindowsUpdate(const std::wstring& displayName) const;
};

} // namespace IceClean::Core::Cleaner
