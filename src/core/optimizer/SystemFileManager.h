#pragma once
#include <string>
#include <cstdint>
#include <windows.h>

namespace IceClean::Core::Optimizer {

// 系统文件信息
struct SystemFileInfo {
    std::wstring name;
    std::wstring path;
    uint64_t sizeBytes = 0;
    std::wstring sizeDisplay;
    bool canManage = false;    // 是否可以管理（禁用/缩小/删除）
    bool isActive = false;     // 当前是否启用
    std::wstring description;
    std::wstring actionLabel;   // 操作按钮文字
};

// 系统文件管理结果
struct SystemFileActionResult {
    bool success = false;
    uint64_t freedBytes = 0;
    std::wstring message;
};

class SystemFileManager {
public:
    // 获取所有可管理的系统文件列表
    SystemFileInfo GetHibernationFileInfo();
    SystemFileInfo GetPageFileInfo();
    SystemFileInfo GetWindowsOldInfo();
    SystemFileInfo GetRecycleBinInfo();

    // ── 休眠文件 ──
    // 关闭休眠并删除 hiberfil.sys
    SystemFileActionResult DisableHibernation();
    // 查询休眠状态
    bool IsHibernationEnabled() const;
    // 获取休眠文件大小
    uint64_t GetHibernationFileSize() const;
    // 缩小休眠文件（仅保留快速启动所需）
    SystemFileActionResult SetHibernationSize(DWORD percent);

    // ── 虚拟内存 ──
    // 查询虚拟内存当前配置
    bool GetPageFileInfo(uint64_t& totalSize, uint64_t& recommendedSize) const;
    // 调整为推荐大小（通过注册表）
    SystemFileActionResult OptimizePageFile();

    // ── Windows.old ──
    // Windows.old 是否存在
    bool HasWindowsOld() const;
    // 获取 Windows.old 大小
    uint64_t GetWindowsOldSize() const;
    // 通过 DISM 清理 Windows.old
    SystemFileActionResult CleanWindowsOld();

    // ── 回收站 ──
    // 获取回收站大小
    uint64_t GetRecycleBinSize() const;
    // 清空回收站
    SystemFileActionResult EmptyRecycleBin();

    // ── 系统更新缓存（DISM） ──
    // 清理 Windows 更新缓存（WinSxS 清理）
    SystemFileActionResult CleanWinSxS();

    // ── 工具方法 ──
    static std::wstring FormatSize(uint64_t bytes);

private:
    // 以管理员身份运行命令
    bool RunAdminCommand(const std::wstring& cmd, DWORD timeoutMs = 60000) const;
    // 读取命令输出
    std::wstring RunCommandWithOutput(const std::wstring& cmd) const;
};

} // namespace IceClean::Core::Optimizer
