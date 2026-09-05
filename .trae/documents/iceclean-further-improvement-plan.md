# IceClean 进一步对标提升计划

## 一、现状总结

### 已完整实现
- 12类扫描器 + 7类清理器 + 6类迁移器 + 3类优化器 + 3类分析器
- 健康评分、圆形进度条动画、卡片交互、导航图标、进程管理
- 三级安全标识、白名单、还原点、操作日志

### 关键缺陷（代码探索发现）
1. **ScheduledTaskOptimizer 后端已实现但UI未集成** — StartupPanel 缺少"计划任务"标签页
2. **MigrationProgressDlg 未实际连接** — 迁移进度对话框不更新
3. **DiskAnalyzerPanel 右键"删除/迁移"未绑定** — 只实现了"打开位置"
4. **DiskAnalyzerPanel 文件类型筛选无效** — BuildTreemap未使用筛选状态
5. **清理过程无进度反馈** — StartClean传入空回调
6. **快捷卡片全部触发相同扫描** — 无法区分清理类别
7. **白名单UI与WhitelistProvider不同步** — 用户添加的白名单不生效
8. **进程管理不自动加载** — 需手动点刷新
9. **管理员权限未自动提升** — 启动时不检查

---

## 二、提升计划（按优先级排序，聚焦切实有效）

### 第一优先级：修复已有功能的缺陷（让已有功能真正可用）

#### 1.1 StartupPanel 集成计划任务优化
- **文件**: `StartupPanel.h/.cpp`
- **问题**: `ScheduledTaskOptimizer` 后端完整实现，但UI无入口
- **方案**: 在StartupPanel的wxNotebook中新增"计划任务"标签页
  - 调用 `ScheduledTaskOptimizer::GetDisablableTasks()` 获取可禁用的开机计划任务
  - 复用现有的Toggle开关组件
  - 优化时一并处理计划任务的禁用

#### 1.2 迁移进度对话框实际连接
- **文件**: `MigrationPanel.cpp`、`MainWindow.cpp`
- **问题**: MigrationProgressDlg存在但未连接进度回调
- **方案**: 在 `StartMigration` 中将进度回调连接到 MigrationProgressDlg
  - 使用 `wxEVT_MIGRATE_PROGRESS` 事件更新对话框
  - 对话框取消按钮触发迁移取消

#### 1.3 DiskAnalyzerPanel 右键菜单补全
- **文件**: `DiskAnalyzerPanel.cpp`
- **问题**: 右键"删除"和"迁移"菜单项未绑定处理函数
- **方案**:
  - "删除": 调用 `ShellFileOperation(FO_DELETE)` 移到回收站
  - "迁移": 弹出目标驱动器选择，调用 FolderMigrator 执行迁移

#### 1.4 清理进度反馈
- **文件**: `MainWindow.cpp`、新增 `CleanProgressDialog.h/.cpp`
- **问题**: 清理过程无任何UI反馈
- **方案**: 新建清理进度对话框，包含：
  - 当前清理类别
  - 已清理文件数和已释放空间
  - 进度条
  - 取消按钮

#### 1.5 白名单持久化与同步
- **文件**: `SettingsPanel.cpp`、`WhitelistProvider.h/.cpp`、`JsonUtil.cpp`
- **问题**: 用户在设置中添加的白名单路径不保存、不生效
- **方案**:
  - `SaveSettings` 中保存用户白名单到 settings.json
  - `WhitelistProvider` 启动时从 settings.json 加载用户白名单
  - `IsWhitelisted` 同时检查硬编码白名单和用户白名单

### 第二优先级：补齐业界标配功能

#### 2.1 管理员权限自动提升
- **文件**: `MainWindow.cpp`（或新增 `App.cpp` 入口）
- **问题**: 很多功能（服务管理、进程终止、注册表修改）需要管理员权限
- **方案**: 启动时检测 `IsRunningAsAdmin()`，若非管理员则通过 `ShellExecuteW` 以 `runas` 重新启动
  - 添加应用清单文件(requireAdministrator)

#### 2.2 弹窗拦截
- **新增文件**: `core/interceptor/PopupInterceptor.h/.cpp`、`gui/panels/PopupInterceptorPanel.h/.cpp`
- **参考**: 火绒弹窗拦截（口碑杀手锏）
- **方案**:
  - 使用 Windows Event Hook (`SetWinEventHook`) 监控窗口创建事件
  - 维护已知弹窗规则库（进程名+窗口类名+标题关键词）
  - 检测到匹配弹窗时自动 `PostMessage(WM_CLOSE)`
  - 拦截记录可视化展示
  - 用户可自定义拦截规则
- **UI**: 新增导航项"拦截"或集成到加速面板

#### 2.3 大文件/重复文件查找
- **新增文件**: `core/scanner/DuplicateFileScanner.h/.cpp`、`gui/panels/DuplicateFilePanel.h/.cpp`
- **方案**:
  - 大文件：基于现有 DiskSpaceAnalyzer，按大小排序展示 Top N 大文件
  - 重复文件：按文件大小分组 → SHA256内容哈希比对
  - 智能选择保留（最新修改优先）
  - 支持预览和删除

#### 2.4 软件卸载管理
- **新增文件**: `core/uninstaller/SoftwareEnumerator.h/.cpp`、`gui/panels/SoftwarePanel.h/.cpp`
- **方案**:
  - 从注册表 `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall` 读取已安装程序
  - 显示名称/发布者/大小/安装日期
  - 按大小排序，标识占用空间大的软件
  - 调用系统卸载程序 (`UninstallString`)
  - 卸载后扫描残留文件/注册表

### 第三优先级：体验优化

#### 3.1 进程管理自动加载
- **文件**: `StartupPanel.cpp`
- **问题**: 切换到进程管理标签页时列表为空，需手动刷新
- **方案**: 在 wxNotebook 页面切换事件中自动调用 `LoadProcessList()`

#### 3.2 快捷卡片区分扫描
- **文件**: `DashboardPanel.cpp`
- **问题**: 4个快捷卡片全部触发相同的一键扫描
- **方案**: 每个卡片触发针对性扫描（仅扫描对应类别）

#### 3.3 系统托盘快捷菜单
- **文件**: `MainWindow.cpp`
- **方案**: 托盘图标右键菜单添加"一键清理"和"打开主窗口"

#### 3.4 DiskAnalyzerPanel 文件类型筛选修复
- **文件**: `DiskAnalyzerPanel.cpp`
- **方案**: BuildTreemap 中根据筛选状态过滤节点

---

## 三、实施步骤

### 阶段一：修复缺陷（1.1-1.5）
1. StartupPanel 新增"计划任务"标签页
2. 迁移进度对话框连接
3. DiskAnalyzerPanel 右键菜单补全
4. 清理进度对话框
5. 白名单持久化与同步

### 阶段二：补齐功能（2.1-2.4）
6. 管理员权限自动提升
7. 弹窗拦截模块
8. 大文件/重复文件查找
9. 软件卸载管理

### 阶段三：体验优化（3.1-3.4）
10. 进程管理自动加载
11. 快捷卡片区分扫描
12. 系统托盘快捷菜单
13. 文件类型筛选修复

---

## 四、验证步骤

每个阶段完成后：
1. 编译通过（`build_local.bat`）
2. 手动验证各改动效果
3. 确保现有功能不受影响
