#pragma once
#include "CleanerBase.h"
#include <vector>
#include <string>
#include <atomic>

namespace IceClean::Core::Cleaner {

// 注册表无效项
struct RegistryInvalidItem {
    std::wstring keyPath;       // 注册表键路径
    std::wstring valueName;     // 值名称（空表示键本身无效）
    std::wstring invalidValue;  // 无效的值数据
    std::wstring description;   // 描述
    enum class Type {
        InvalidUninstall,       // 无效卸载信息
        InvalidStartup,         // 无效启动项
        InvalidFileAssoc,       // 无效文件关联
        InvalidSharedDLL,       // 无效共享DLL
        InvalidFont,            // 无效字体引用
        InvalidHelpFile,        // 无效帮助文件引用
        InvalidAppPath,         // 无效应用程序路径
        InvalidCOM,             // 无效COM/ActiveX组件
        InvalidMUI,             // 无效MUI缓存
        InvalidEnvVar,          // 环境变量中的无效路径
        InvalidTrayNotify,      // 无效托盘通知缓存
        InvalidSound,           // 无效声音/事件关联
    };
    Type type;
};

class RegistryCleaner : public CleanerBase {
public:
    RegistryCleaner();

    std::wstring GetName() const override { return L"注册表清理器"; }

    Models::CleanResult Clean(const std::vector<std::wstring>& paths,
                               std::function<void(const Models::CleanProgress&)> progressCallback = nullptr,
                               const std::atomic<bool>* cancelFlag = nullptr) override;

    // 扫描无效注册表项
    std::vector<RegistryInvalidItem> ScanInvalidItems();

    // 清理指定的无效注册表项
    // items: 要清理的项列表
    // backupPath: 备份注册表的文件路径（.reg格式）
    Models::CleanResult Clean(const std::vector<RegistryInvalidItem>& items,
                               const std::wstring& backupPath = L"",
                               std::function<void(const Models::CleanProgress&)> progressCb = nullptr);

private:
    // 扫描无效的卸载信息
    void ScanInvalidUninstall(std::vector<RegistryInvalidItem>& items);

    // 扫描无效的启动项
    void ScanInvalidStartup(std::vector<RegistryInvalidItem>& items);

    // 扫描无效的共享DLL
    void ScanInvalidSharedDLL(std::vector<RegistryInvalidItem>& items);

    // 扫描无效的字体引用
    void ScanInvalidFonts(std::vector<RegistryInvalidItem>& items);

    // 扫描无效的帮助文件引用
    void ScanInvalidHelpFiles(std::vector<RegistryInvalidItem>& items);

    // 扫描无效的应用程序路径
    void ScanInvalidAppPaths(std::vector<RegistryInvalidItem>& items);

    // 扫描无效的COM/ActiveX组件
    void ScanInvalidCOM(std::vector<RegistryInvalidItem>& items);

    // 扫描无效的MUI缓存
    void ScanInvalidMUI(std::vector<RegistryInvalidItem>& items);

    // 扫描环境变量中的无效路径
    void ScanInvalidEnvVars(std::vector<RegistryInvalidItem>& items);

    // 扫描无效的托盘通知缓存
    void ScanInvalidTrayNotify(std::vector<RegistryInvalidItem>& items);

    // 扫描无效的声音/事件关联
    void ScanInvalidSound(std::vector<RegistryInvalidItem>& items);

    // 检查路径是否存在
    bool PathExists(const std::wstring& path) const;

    // 从注册表值中提取文件路径
    std::wstring ExtractFilePath(const std::wstring& value) const;

    // 备份注册表项到 .reg 文件
    bool BackupRegistryKey(const std::wstring& keyPath, const std::wstring& backupFile);
};

} // namespace IceClean::Core::Cleaner
