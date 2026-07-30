#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace IceClean::Core::Optimizer {

// 右键菜单项信息
struct ContextMenuItem {
    std::wstring keyPath;          // 注册表路径
    std::wstring valueName;        // 值名称
    std::wstring command;          // 执行命令
    std::wstring displayText;      // 显示文本
    std::wstring iconPath;         // 图标路径(可选)
    enum class Location {
        File,          // 文件右键
        Folder,        // 文件夹右键
        Desktop,       // 桌面右键
        Drive,         // 驱动器右键
        Directory,     // 目录背景右键
        Unknown
    };
    Location location = Location::Unknown;
    bool isSystem = false;        // 是否系统自带
    bool isEnabled = true;        // 是否启用
};

class ContextMenuManager {
public:
    // 扫描所有右键菜单项
    std::vector<ContextMenuItem> ScanContextMenu();

    // 禁用右键菜单项(重命名为前缀 _disabled_)
    bool DisableItem(const ContextMenuItem& item);

    // 启用右键菜单项(恢复原名)
    bool EnableItem(const ContextMenuItem& item);

    // 删除右键菜单项
    bool DeleteItem(const ContextMenuItem& item);

    // 获取位置类型的显示名称
    static std::wstring GetLocationName(ContextMenuItem::Location location);

private:
    // 扫描指定位置的右键菜单（shell 子键）
    void ScanShellMenu(HKEY rootKey, const std::wstring& basePath,
                       ContextMenuItem::Location location,
                       std::vector<ContextMenuItem>& items);

    // 扫描指定位置的 COM 扩展右键菜单（shellex\ContextMenuHandlers 子键）
    void ScanContextMenuHandlers(HKEY rootKey, const std::wstring& basePath,
                                 ContextMenuItem::Location location,
                                 std::vector<ContextMenuItem>& items);

    // 解析位置类型
    ContextMenuItem::Location DetermineLocation(const std::wstring& keyPath) const;

    // 检查是否为系统项
    bool IsSystemItem(const std::wstring& keyPath, const std::wstring& displayText) const;
};

} // namespace IceClean::Core::Optimizer
