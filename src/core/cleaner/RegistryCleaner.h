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
    enum class Type { InvalidUninstall, InvalidStartup, InvalidFileAssoc };
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

    // 检查路径是否存在
    bool PathExists(const std::wstring& path) const;

    // 从注册表值中提取文件路径
    std::wstring ExtractFilePath(const std::wstring& value) const;

    // 备份注册表项到 .reg 文件
    bool BackupRegistryKey(const std::wstring& keyPath, const std::wstring& backupFile);
};

} // namespace IceClean::Core::Cleaner
