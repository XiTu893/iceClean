# UI 界面与交互体验提升方案

## 一、当前问题分析

### 1.1 布局问题

| 问题 | 描述 | 影响 |
|------|------|------|
| 侧边栏即将过载 | 现有 10 项，新增 7 个模块后将达 17 项，200px 宽度无法容纳 | 用户需滚动查找，核心功能被淹没 |
| 功能层级不清晰 | 部分面板使用了 wxNotebook 子标签（如迁移面板含 3 个子页），部分独立为导航项 | 用户难以建立心智模型 |
| 全新模块无归属 | WindowsDebloater、PrivacyOptimizer、SystemFileManager 等 7 个模块尚无对应面板 | 用户无法访问新功能 |

### 1.2 交互响应问题

| 问题 | 描述 | 影响 |
|------|------|------|
| 扫描进度粒度粗 | ScannerAggregator 按扫描器粒度汇报进度，每完成一个扫描器才更新一次 | 长时间扫描中用户看不到具体进展 |
| 清理进度不精确 | 清理进度仅依赖 `currentItem/totalItems`，部分操作为硬编码百分比 | 进度条可能出现停滞或跳跃 |
| 缺少操作取消确认 | 强制停止使用 3 秒超时后 detach 线程，可能导致资源未释放 | 存在句柄泄漏风险 |
| 缺少多操作队列 | `m_workerMutex` 串行化所有后台操作，一个操作未完成时无法启动另一操作 | 用户体验割裂 |

### 1.3 进度不清晰问题

| 问题 | 描述 | 影响 |
|------|------|------|
| CleanProgressDialog 信息有限 | 仅显示类别名+文件名+已清理大小，缺少剩余时间/速度/进度百分比数值 | 用户无法预判还要等多久 |
| 无全局进度指示 | 仅清理和迁移有进度对话框，其他操作（启动优化、Debloat等）无进度反馈 | 用户怀疑程序卡死 |
| 后台操作无状态栏 | 长时间后台操作时，主界面无任何提示 | 用户可能误关闭程序 |

### 1.4 卡死风险

| 场景 | 风险等级 | 说明 |
|------|----------|------|
| `ScannerAggregator::ScanAll` 内扫描文件 | 🟡 低 | 已在 std::thread 中运行，但文件系统慢时可能长时间阻塞线程 |
| `StartupOptimizer::GetStartupItems` | 🟡 中 | 后台线程 + CallAfter 回调，但注册表高延迟时 UI 仍可能短暂卡顿 |
| WinSxS/CompactOS 清理 | 🔴 高 | DISM 操作可能耗时数分钟，当前仅在后台线程中运行，无子进度 |
| 大文件迁移 | 🔴 高 | 文件复制可能耗时很长，进度按文件粒度汇报，大文件内部无进度 |

---

## 二、提升方案

### 2.1 导航重组 —— 解决"侧边栏过载"

#### 现状 → 目标

```
当前 (10 项)                           目标 (13 项，含分隔线)
┌──────────────┐                      ┌──────────────┐
│ 🏠 首页       │                      │ 🏠 首页       │
│ 🧹 深度清理    │                      │ 🧹 深度清理    │
│ 📦 智能迁移    │                      │ 📦 智能迁移    │
│ ⚡ 加速优化    │                      │ ⚡ 加速优化    │
│ 📱 软件管理    │                      │ 📱 软件管理    │
│ ⭐ 软件推荐    │                      │ ⭐ 软件推荐    │
│ 🛡️ 安全防护   │                      │ 🛡️ 安全防护   │
│ 🌐 网络优化    │                      │ 🌐 网络优化    │
│ ⚙️ 设置       │                      │ ──────────── │
│ ℹ️ 关于       │                      │ 📊 磁盘分析    │
└──────────────┘                      │ 🗃️ 文件分类    │
                                      │ 📥 下载管理    │
                                      │ ⚙️ 设置       │
                                      │ ℹ️ 关于+硬件   │
                                      └──────────────┘
```

#### 导航分组方案

采用 **分隔线分组** 将导航分为三大区域：

- **核心功能**（上组）：首页、深度清理、智能迁移、加速优化
- **管理工具**（中组）：软件管理、软件推荐、安全防护、网络优化
- **实用工具**（下组带分隔线）：磁盘分析、文件分类、下载管理、设置、关于+硬件信息

#### 具体分配

| 导航项 | 面板构成 | 说明 |
|--------|---------|------|
| 🏠 首页 | DashboardPanel | 不变 |
| 🧹 深度清理 | wxNotebook: 系统清理/隐私清理/注册表清理/软件专清/AppData专清 | 新增 AppData 子标签 |
| 📦 智能迁移 | MigrationPanel | 不变 |
| ⚡ 加速优化 | wxNotebook: 启动管理/服务优化/Windows组件精简/隐私策略/系统文件 | 新增去组件/隐私策略/系统文件三子标签 |
| 📱 软件管理 | UninstallPanel + 软件升级管理 | 不变 |
| ⭐ 软件推荐 | SoftwareRecommendPanel | 不变 |
| 🛡️ 安全防护 | SecurityPanel | 不变(已含恶意软件扫描) |
| 🌐 网络优化 | wxNotebook: 网络优化/驱动管理 | 不变 |
| 📊 磁盘分析 | DiskAnalyzerPanel | 从智能迁移拆出独立导航项 |
| 🗃️ 文件分类 | FileTypeAnalyzerPanel (新建) | 文件类型统计与报告 |
| 📥 下载管理 | DownloadManagerPanel (新建) | 下载文件夹管理 |
| ⚙️ 设置 | SettingsPanel | 不变 |
| ℹ️ 关于+硬件 | wxNotebook: 关于/硬件信息 | 新增硬件信息子标签 |

### 2.2 统一线程与进度框架 —— 解决"卡死"与"进度不清晰"

#### 2.2.1 ProgressReporter 统一进度接口

新增 `ProgressReporter` 类，所有长时间操作统一使用：

```cpp
class ProgressReporter {
public:
    // 报告进度 (0-100)
    void ReportProgress(int percent, const std::wstring& stage, const std::wstring& detail);

    // 报告子进度 (某一步内的 0-100)
    void ReportSubProgress(int subPercent, const std::wstring& subDetail);

    // 检查是否被取消
    bool IsCancelled() const;

    // 取消操作
    void Cancel();

    // 获取统计信息
    uint64_t GetProcessedBytes() const;
    uint64_t GetTotalBytes() const;
};
```

所有后台操作函数签名统一为：

```cpp
// 旧
ScanResult scan(const std::atomic<bool>* stopFlag, ScanProgressCallback cb);

// 新
ScanResult scan(ProgressReporter& reporter);
```

#### 2.2.2 通用进度对话框 `UnifiedProgressDialog`

新建通用进度对话框取代 `CleanProgressDialog` 和 `MigrationProgressDlg`，支持：

- 主进度条（0-100%）+ 百分比数值
- 阶段名称（如 "正在清理系统缓存"）
- 详细描述（如 "C:\Users\xxx\AppData\Local\Temp\tmp123.tmp"）
- 子进度条（某阶段内的内部进度）
- 速度估算（MB/s）
- 剩余时间估算（自适应：<1分钟 → 秒，<1小时 → 分钟，>1小时 → 小时）
- 已处理/总量统计（文件数、大小）
- 取消按钮 + 取消确认对话框

#### 2.2.3 工作队列管理器 `WorkerQueueManager`

取代当前单一的 `m_workerThread` + `m_workerMutex` 模式：

```cpp
class WorkerQueueManager {
public:
    // 提交后台任务，返回 taskId
    int SubmitTask(std::function<void(ProgressReporter&)> task, const std::wstring& taskName);

    // 取消指定任务
    void CancelTask(int taskId);

    // 取消所有任务
    void CancelAll();

    // 获取任务状态
    TaskStatus GetTaskStatus(int taskId) const;

    // 获取当前进度
    int GetTaskProgress(int taskId) const;

    // 任务完成回调
    std::function<void(int taskId, bool success)> onTaskComplete;
};
```

**任务队列（FIFO）** 确保多个操作排队执行而非冲突。

#### 2.2.4 主界面状态栏指示

在底部添加状态栏（可折叠/浮动）：

- 空闲时显示 "就绪" + 上次操作时间
- 有后台操作时显示任务名称 + 进度百分比 + 取消按钮
- 支持同时显示多个排队任务

### 2.3 新面板设计

#### 2.3.1 AppData 专清标签页（DeepCleanPanel 第5子标签）

- 复用现有 `wxListCtrl` 风格
- 显示 9 个子类别（LocalTemp/BrowserCache/ElectronCache/...）
- 全选/反选按钮
- 单次扫描，分类展示结果
- 清理时复用现有清理进度对话框

#### 2.3.2 Windows 组件精简 + 隐私策略 + 系统文件标签页（加速优化面板）

- WindowsDebloater 标签页：
  - 预设选择下拉框（推荐/最小）
  - wxCheckListBox 列出所有可精简项（按类别分组）
  - 全选/反选/仅推荐 按钮
  - 精简按钮 → 显示进度对话框
- 隐私策略标签页：
  - 预设选择下拉框（最大/终极）
  - wxCheckListBox 列出 20+ 隐私策略项
  - 应用按钮 → 注册表写入进度
- 系统文件标签页：
  - 状态卡片：hiberfil.sys（大小/启用状态）/ pagefile.sys / Windows.old
  - 操作按钮：禁用/调整大小（hiberfil）/ 清理

#### 2.3.3 磁盘分析独立导航项 + 文件分类面板 + 下载管理面板

- **磁盘分析**：从智能迁移拆出，独立展示（保持现有 DiskAnalyzerPanel 不变）
- **文件分类面板** (FileTypeAnalyzerPanel)：
  - 选择要分析的目录
  - "开始分析"按钮
  - 分析中显示进度
  - 结果表格：类别 | 文件数 | 总大小 | 占比
  - 导出 HTML/TXT 报告按钮
- **下载管理面板** (DownloadManagerPanel)：
  - 选择下载目录（默认浏览器下载目录）
  - "扫描"按钮
  - 结果按类别分组显示（安装包/压缩包/文档/图片/...）
  - 非活跃文件高亮（超过30天无访问）
  - 批量清理/迁移按钮

#### 2.3.4 关于+硬件信息组合标签页

- 关于：原 AboutPanel 不变
- 硬件信息标签页：
  - 分组信息卡片（CPU/GPU/内存/磁盘/主板/OS）
  - 只读显示，无操作按钮
  - "导出报告"按钮

### 2.4 交互细节优化

| 改进项 | 实现方案 |
|--------|---------|
| 扫描进度细化 | 扫描器内按目录汇报进度，每 500ms 节流 |
| 清理进度精确化 | 操作前统计总文件数得到精确 totalItems，不支持预统计的操作使用加权进度 |
| 取消确认 | 取消时弹确认对话框 + 自动回滚已部分执行的操作 |
| 操作安全锁 | 共享状态使用 `std::shared_mutex`，读多写少场景优化性能 |
| 后台操作提示 | 操作开始时在状态栏显示图标+文字提示 |
| 操作完成通知 | 完成后托盘区弹出 balloon 通知（Win32原生） |
| 窗口响应保障 | 所有长时间操作确保 100ms 内至少处理一次 wxPendingEvents |
| 键盘快捷键 | Ctrl+S 开始扫描，Ctrl+C 取消，Ctrl+1-9 切换导航 |

### 2.5 实现优先级

| 优先级 | 内容 | 工作量 |
|--------|------|--------|
| P0 | 通用进度对话框 + ProgressReporter | 2天 |
| P1 | 导航重组（分隔线+新增项） | 1天 |
| P1 | 加速优化面板扩展（Debloat/隐私/系统文件） | 3天 |
| P2 | 文件分类面板 + 下载管理面板 | 3天 |
| P2 | 关于+硬件信息组合 | 1天 |
| P3 | 状态栏 + 工作队列管理器 | 2天 |
| P3 | 键盘快捷键 + 托盘通知 | 1天 |

---

## 三、防止卡死的具体措施

### 3.1 线程模型

```
┌─────────────────────────────────────────────┐
│                 主线程 (UI)                    │
│  wxWidgets 事件循环 + wxSimplebook 页面切换    │
│  - 所有控件创建/更新必须在主线程                │
│  - CallAfter() / wxQueueEvent() 通知主线程     │
└──────────┬──────────────────────────────────┘
           │ 提交任务
           ▼
┌─────────────────────────────────────────────┐
│              工作队列 (单线程)                 │
│  FIFO 队列 + 顺序执行                         │
│  - 每个任务持有一个 ProgressReporter           │
│  - 任务间不共享可变状态                        │
└─────────────────────────────────────────────┘
```

### 3.2 防卡死清单

- [x] 所有 I/O 密集型操作在 `std::thread` 中运行
- [x] UI 更新通过 `CallAfter()` / `wxQueueEvent()` 派发
- [x] 进度回调每 100ms 节流一次（防止高频更新压垮 UI 队列）
- [x] 取消操作使用 `std::atomic<bool>` 检查点模式
- [x] 大文件操作可被取消（定期检查取消标志）
- [x] 窗口关闭时等待后台操作完成（最多 5 秒超时后 detach）
- [ ] 禁用窗口关闭按钮在后台操作运行时（已实现部分）

### 3.3 异常安全

- 所有后台操作使用 RAII 确保资源释放
- 操作日志记录每个步骤，失败时可追溯
- 取消时已完成的子操作不回滚（用户确认）

---

## 四、实现状态

### 已实现 ✅

#### P0 — 进度框架
- [x] `core/utils/ProgressReporter.h/.cpp` — 统一进度报告接口，含进度/速度/ETA/取消
- [x] `gui/dialogs/UnifiedProgressDialog.h/.cpp` — 通用进度对话框：主进度+百分比、子进度、速度、ETA、已处理数/大小、取消确认
- [x] `gui/Events.h/.cpp` — 新增 `wxEVT_OPERATION_PROGRESS_UPDATE` / `wxEVT_OPERATION_COMPLETE` / `wxEVT_OPERATION_CANCEL`

#### P1 — 导航重组
- [x] `gui/controls/NavSidebar.h/.cpp` — 10→13 项，新增 2 条分隔线分三组，3 个新图标（磁盘/文件/下载）
- [x] `gui/MainWindow.h/.cpp` — 完全重构：13 页内容书，磁盘分析独立，新增文件分类/下载管理页

#### P1 — 加速优化扩展
- [x] `gui/panels/WindowsDebloaterPanel.h/.cpp` — 预设下拉框 + 分类 CheckListBox + 全选/反选/推荐 + 批量应用（ProgressReporter 进度）
- [x] `gui/panels/PrivacyOptimizerPanel.h/.cpp` — 预设下拉框 + 分类 CheckListBox + 安全级别标识 + 批量应用
- [x] `gui/panels/SystemFileManagerPanel.h/.cpp` — 5 组系统文件卡片（hiberfil/pagefile/Windows.old/回收站/WinSxS）+ 操作按钮 + UnifiedProgressDialog

#### P2 — 文件分类 + 下载管理
- [x] `gui/panels/FileTypeAnalyzerPanel.h/.cpp` — 目录选择 + 扫描 + 结果表格（类别/扩展名/文件数/大小/占比）+ 导出 HTML/TXT
- [x] `gui/panels/DownloadManagerPanel.h/.cpp` — 默认下载路径 + 扫描 + 筛选（全部/安装包/非活跃30天）+ 清理/迁移

#### P2 — 硬件信息
- [x] `gui/panels/HardwareInfoPanel.h/.cpp` — 7 组信息卡片（CPU/GPU/内存/磁盘/主板/OS/运行时间），后台线程加载

#### P3 — 状态栏
- [x] `gui/MainWindow.cpp` — 3 字段状态栏（状态/时间/版本），SetStatusBusy/SetStatusIdle

#### P3 — 工作队列管理器
- [x] `core/utils/WorkerQueueManager.h/.cpp` — FIFO 任务队列，支持取消、进度报告、状态查询

#### P3 — 键盘快捷键
- [x] `gui/MainWindow.cpp` — Ctrl+S 扫描、Ctrl+C 停止、Ctrl+1-9/0 导航切换

### 待实现 🔲
- [ ] 托盘通知（操作完成 balloon 提示）
- [ ] AppData 专清标签页集成到 DeepCleanPanel
