# 深色主题可用化 & 细节打磨实施计划

## 当前状态总结

### 已完成的升级任务
- 阶段1.3 导航精简重组 (13→9项) ✅
- 阶段2.3 强制卸载/强制删除 ✅
- 阶段2.1 软件专清扫描器 (SoftwareCacheScanner + DeepCleanPanel第4标签页) ✅
- 阶段1.1 统一视觉规范 (ThemeManager字体工厂+按钮尺寸常量) ✅
- 阶段1.4 首页增强 (累计统计+C盘预警+6快捷卡片) ✅
- 阶段2.2 软件升级检测 (SoftwareUpdateChecker + UninstallPanel可升级标签页) ✅
- 阶段3 交互优化 - Toggle内存泄漏修复 ✅

### 待完成任务
1. **阶段1.2 深色主题可用化** — 有编译错误 + 所有面板硬编码颜色(~100处)
2. **阶段4 细节打磨** — UsageStats持久化 + C盘监控定时器 + 注册表自动备份 + 操作确认增强

---

## 任务1: 修复 ThemeType 编译错误

**文件**: `src/gui/panels/SettingsPanel.cpp` 第99行

**问题**: `static_cast<ThemeManager::ThemeType>` — `ThemeType` 是 `IceClean::Gui` 命名空间的顶层枚举，不是 `ThemeManager` 的嵌套类型

**修复**: 改为 `static_cast<ThemeType>`（SettingsPanel.cpp 在 `IceClean::Gui` 命名空间内，可直接访问）

---

## 任务2: 深色主题可用化 — 面板颜色替换

### 2.1 改进 ThemeManager::ApplyTheme

**文件**: `src/gui/controls/ThemeManager.cpp`

当前 `ApplyTheme` 过于粗糙（所有按钮统一设为 surface+textPrimary），需要改进：
- 通过控件名称前缀区分按钮语义：`btn_primary_*` → 品牌色, `btn_danger_*` → 危险色, `btn_success_*` → 成功色
- 增加 `wxCheckBox`、`wxChoice`、`wxNotebook`、`wxScrolledWindow` 等控件类型处理
- 增加 `wxListCtrl` 的交替行颜色

### 2.2 各面板硬编码颜色替换

**核心策略**: 在每个面板的 `CreateUI` 方法开头获取 `const auto& colors = ThemeManager::Instance().GetColors();`，然后用语义颜色替换硬编码值。

**颜色映射表**:

| 硬编码值 | 语义颜色 | ThemeColors字段 |
|----------|---------|----------------|
| `*wxWHITE` (背景) | 背景色 | `colors.background` |
| `*wxWHITE` (面板/卡片) | 表面色 | `colors.surface` |
| `wxColour(0x33,0x33,0x33)` | 主文字 | `colors.textPrimary` |
| `wxColour(0x66,0x66,0x66)` | 次文字 | `colors.textSecondary` |
| `wxColour(0x99,0x99,0x99)` | 辅助文字 | `colors.textDisabled` |
| `wxColour(0x00,0x78,0xD4)` | 品牌色 | `colors.accent` |
| `wxColour(0xE8,0x11,0x23)` | 危险色 | `colors.danger` |
| `wxColour(0x10,0x7C,0x10)` / `wxColour(0x10,0xB9,0x81)` | 成功色 | `colors.success` |
| `wxColour(0xFF,0x8C,0x00)` | 警告色 | `colors.warning` |
| `wxColour(0xCC,0xCC,0xCC)` | 边框色 | `colors.border` |

**涉及文件** (按硬编码数量排序):
1. `src/gui/panels/StartupPanel.cpp` (~25处)
2. `src/gui/panels/DashboardPanel.cpp` (~25处)
3. `src/gui/panels/SettingsPanel.cpp` (~22处)
4. `src/gui/panels/DeepCleanPanel.cpp` (~16处)
5. `src/gui/panels/UninstallPanel.cpp` (~12处)

### 2.3 自绘控件响应主题变更

**文件**: `src/gui/controls/NavSidebar.cpp`、`src/gui/controls/CircularProgress.cpp`

自绘控件需要在 `ThemeManager::RegisterChangeCallback` 中注册回调，重绘时使用 `ThemeManager::Instance().GetColors()` 获取当前主题颜色。

---

## 任务3: 阶段4 细节打磨

### 3.1 UsageStats 持久化模块

**新增文件**:
- `src/core/safety/UsageStats.h` — 累计统计管理器
- `src/core/safety/UsageStats.cpp` — 实现

**功能**:
- 单例模式，管理 `%APPDATA%\IceClean\usage_stats.json`
- `RecordClean(uint64_t bytes)` — 记录清理量
- `GetTotalCleanedBytes() -> uint64_t` — 获取累计清理量
- `GetCleanCount() -> int` — 获取清理次数
- `GetLastCleanTime() -> std::string` — 获取上次清理时间
- 线程安全（std::mutex）

**修改文件**:
- `src/gui/panels/DashboardPanel.cpp` — `LoadCumulativeStats()` 改为调用 UsageStats
- 各清理面板 — 清理完成后调用 `UsageStats::Instance().RecordClean(bytes)`

### 3.2 C盘空间监控定时器

**修改文件**: `src/gui/panels/DashboardPanel.h/.cpp`

**功能**:
- 添加 `wxTimer m_diskTimer`，每60秒检查一次C盘空间
- 空间不足时更新预警标签
- 面板销毁时停止定时器

### 3.3 注册表自动备份

**修改文件**: `src/core/cleaner/RegistryCleaner.cpp` 或相关清理器

**功能**:
- 清理注册表前，导出相关键到 `%APPDATA%\IceClean\registry_backup\`
- 使用 `reg export` 命令备份
- 保留最近5次备份，自动清理旧备份

### 3.4 操作确认增强

**修改文件**: 各清理面板的确认对话框

**功能**:
- 危险操作确认对话框增加3秒倒计时，倒计时结束前"确认"按钮禁用
- 创建通用 `SafeConfirmDialog` 对话框类

---

## 实施顺序

1. **修复 ThemeType 编译错误** (1分钟)
2. **改进 ApplyTheme** (10分钟)
3. **替换5个面板硬编码颜色** (30分钟，每个面板6分钟)
4. **自绘控件响应主题变更** (5分钟)
5. **UsageStats 持久化模块** (10分钟)
6. **C盘空间监控定时器** (5分钟)
7. **注册表自动备份** (10分钟)
8. **操作确认增强** (10分钟)

## 验证步骤

1. 编译通过无错误
2. 切换深色/浅色主题，所有面板颜色正确变化
3. 累计清理统计在重启后保留
4. C盘空间预警定时更新
5. 注册表清理前自动备份
6. 危险操作确认有倒计时保护
