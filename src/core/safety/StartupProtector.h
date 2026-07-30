#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include "models/StartupItem.h"

namespace IceClean::Core::Safety {

// 启动项变更记录
struct StartupChangeRecord {
    std::wstring itemName;         // 启动项名称
    std::wstring itemPath;         // 路径
    enum class ChangeType {
        Added,      // 新增启动项
        Removed,    // 删除启动项
        Modified,   // 修改启动项
        Enabled,    // 启用
        Disabled    // 禁用
    };
    ChangeType changeType = ChangeType::Modified;
    FILETIME timestamp;
};

// 启动保护器 - 监控启动项变更
class StartupProtector {
public:
    // 扫描当前启动项并建立基线
    void BuildBaseline();

    // 检测自上次基线以来的变更
    std::vector<StartupChangeRecord> DetectChanges();

    // 获取当前启动项列表（用于对比）
    std::vector<IceClean::Models::StartupItem> GetCurrentStartupItems();

    // 锁定/解锁启动项（禁止新增启动项）
    static bool IsStartupLocked();
    static void SetStartupLocked(bool locked);

    // 阻止指定启动项
    bool BlockStartupItem(const std::wstring& name, const std::wstring& path);

    // 恢复被阻止的启动项
    bool UnblockStartupItem(const std::wstring& name);

    // 获取被阻止的启动项列表
    std::vector<std::pair<std::wstring, std::wstring>> GetBlockedItems();

    // 获取变更类型显示名
    static std::wstring GetChangeTypeName(StartupChangeRecord::ChangeType type);

private:
    // 保存/读取基线
    void SaveBaseline(const std::vector<IceClean::Models::StartupItem>& items);
    std::vector<IceClean::Models::StartupItem> LoadBaseline();

    // 获取配置文件路径
    static std::wstring GetConfigFilePath();
};

} // namespace IceClean::Core::Safety
