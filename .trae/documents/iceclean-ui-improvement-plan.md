# IceClean UI交互性、美工、功能提升计划

## 一、现状分析

### 当前优势
- 架构清晰：左侧深色导航栏 + 右侧内容区，7个功能面板
- 自绘控件：CircularProgress、CardPanel、NavSidebar、SafetyBadge、Toggle开关、Treemap
- 安全机制完善：三级安全标识、白名单、还原点、操作日志
- 核心功能齐全：12类扫描器、5类清理器、6类迁移器、3类优化器

### 关键短板（对标业界后）

| 维度 | 当前状态 | 业界标准 |
|------|---------|---------|
| **导航图标** | 彩色圆圈+单字母，极简陋 | SVG图标/图标字体，专业感强 |
| **动画效果** | 完全无动画，切换生硬 | 进度动画、悬停微交互、过渡效果 |
| **健康评分** | 无 | 360/腾讯/CCleaner均有系统健康评分 |
| **清理进度** | 清理过程无进度反馈，仅完成后弹MessageBox | 实时进度条+已释放空间 |
| **累计统计** | 无累计清理统计 | "累计释放XX GB"展示 |
| **软件卸载** | 无 | CCleaner/360/Wise Care均有 |
| **重复文件** | 无 | CCleaner/360/柠檬清理均有 |
| **定时清理** | 无 | CCleaner/Wise Care支持 |
| **暗色主题** | 仅侧边栏深色，内容区纯白 | CCleaner 7支持亮/暗切换 |
| **卡片交互** | 无悬停/点击反馈 | 悬停抬升、点击缩放 |
| **通知提醒** | 无磁盘空间不足提醒 | 腾讯管家/柠檬清理有 |
| **Tooltip** | 控件无悬停提示 | 业界标配 |

---

## 二、提升计划（按优先级排序，聚焦切实有效）

### 第一优先级：视觉美工提升（用户第一印象）

#### 1.1 导航栏图标升级
- **文件**: `NavSidebar.cpp`
- **问题**: 当前使用彩色圆圈+单字母（H/C/M/A/S/G），非常粗糙
- **方案**: 使用 Unicode 符号作为图标替代（wxWidgets原生支持，无需外部资源）：
  - 首页: ⌂ 或 🏠 → 使用自绘几何图形（房子轮廓）
  - 清理: 🧹 → 自绘扫帚/刷子图标
  - 迁移: 📦 → 自绘箭头+盒子
  - 加速: ⚡ → 自绘闪电
  - 分析: 📊 → 自绘饼图
  - 设置: ⚙ → 自绘齿轮
- **实现**: 在 `OnPaint` 中用 `wxGraphicsContext` 绘制简洁的线性图标（24x24），替代当前的圆圈+字母

#### 1.2 卡片交互增强
- **文件**: `CardPanel.cpp`
- **问题**: 无悬停/点击视觉反馈，阴影太弱
- **方案**:
  - 悬停：阴影加深+偏移增大（模拟抬升），边框变蓝
  - 点击：阴影缩小+偏移减小（模拟按下）
  - 阴影增强：偏移从2px→4px，透明度从10%→20%
- **实现**: 添加 `wxEVT_ENTER_WINDOW`/`wxEVT_LEAVE_WINDOW`/`wxEVT_LEFT_DOWN`/`wxEVT_LEFT_UP` 事件处理

#### 1.3 圆形进度条动画
- **文件**: `CircularProgress.cpp`
- **问题**: 进度值瞬间跳变，无平滑过渡
- **方案**: 添加 `wxTimer` 驱动的插值动画，SetValue时从当前值平滑过渡到目标值（200ms）
- **实现**: 新增 `m_animTimer`、`m_animStartValue`、`m_animEndValue`，Timer回调中线性插值

#### 1.4 扫描中旋转动画
- **文件**: `CircularProgress.cpp`
- **问题**: 扫描过程中进度条静止不动（只在进度更新时跳变）
- **方案**: 添加 indeterminate 模式——扫描时在进度弧末端显示旋转光点效果
- **实现**: 新增 `SetIndeterminate(bool)` 方法，启用后启动旋转动画Timer

### 第二优先级：交互体验提升

#### 2.1 系统健康评分
- **文件**: `DashboardPanel.cpp`
- **问题**: 仅显示磁盘使用率百分比，无综合健康评估
- **方案**: 在C盘健康状态区域添加健康评分（0-100分），综合以下因素：
  - 磁盘使用率（权重30%）：>90%扣分多
  - 垃圾文件量（权重25%）：上次扫描结果
  - 启动项数量（权重20%）：过多扣分
  - 可迁移大文件（权重15%）：有则扣分
  - 休眠文件（权重10%）：存在则扣分
- **显示**: 评分数字+颜色（>80绿、60-80橙、<60红）+ 评分说明文字
- **实现**: 新增 `CalculateHealthScore()` 方法，在 `InitializeApp` 和扫描完成后调用

#### 2.2 清理进度反馈
- **文件**: `MainWindow.cpp`、新增 `CleanProgressDialog`
- **问题**: 清理过程无进度反馈，仅完成后弹MessageBox
- **方案**: 清理时显示进度对话框，包含：
  - 当前正在清理的文件/类别
  - 已释放空间实时计数
  - 进度条
  - 取消按钮
- **实现**: 新建 `CleanProgressDialog`（wxDialog），在 `FileCleaner::Clean` 回调中更新

#### 2.3 累计清理统计
- **文件**: `DashboardPanel.cpp`、`OperationLogger`
- **问题**: 无累计清理总量展示
- **方案**: 在首页添加累计统计卡片，显示：
  - 累计释放空间总量
  - 累计清理次数
  - 上次清理时间
- **实现**: `OperationLogger` 新增 `GetTotalCleanedSize()` 和 `GetCleanCount()` 方法

#### 2.4 Tooltip提示
- **文件**: 各面板控件
- **问题**: 控件无悬停提示信息
- **方案**: 为关键控件添加 `SetToolTip()`：
  - 扫描按钮: "扫描C盘垃圾文件"
  - 停止按钮: "停止当前扫描"
  - 各快捷卡片: 描述性提示
  - 安全等级标识: 解释各等级含义
  - Toggle开关: "启用/禁用此启动项"

### 第三优先级：功能补齐

#### 3.0 非系统进程分析与优化（用户强烈需求）
- **新增文件**: `core/analyzer/ProcessAnalyzer.h/.cpp`
- **问题**: 系统中存在QQProtect.exe等第三方顽固进程，当前启动加速面板只显示注册表启动项和Windows服务，无法发现和禁用这些进程
- **功能**:
  - 枚举所有运行中的进程（CreateToolhelp32Snapshot）
  - 判断进程是否为系统自带（比对 %SystemRoot% 路径 + 系统进程白名单）
  - 标识第三方进程：名称/路径/公司名/内存占用/PID/是否可安全终止
  - 安全等级分类：
    - 🟢 安全终止：用户安装的第三方软件守护进程（QQProtect、AliProtect等）
    - 🟡 谨慎终止：可能有用的后台程序（OneDrive、WeChat等）
    - 🔴 不可终止：系统关键进程（csrss.exe、lsass.exe、svchost.exe等）
  - 一键终止选中进程 + 禁用其自启动
  - 终止失败时自动升级到 SeDebugPrivilege / SYSTEM 身份强制终止
- **集成位置**: 在启动加速面板(StartupPanel)中新增"进程管理"标签页
- **数据源**:
  - 进程列表: `CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS)`
  - 进程路径: `OpenProcess` + `QueryFullProcessImageNameW`
  - 公司名: 版本信息 `GetFileVersionInfoW`
  - 系统进程判断: 路径在 `%SystemRoot%` 下 + 内置白名单
- **UI**: 三列布局——进程名+公司名 | 内存占用 | 安全等级+操作按钮（终止/禁用自启动）

#### 3.1 软件卸载管理
- **新增文件**: `SoftwareUninstallPanel.h/.cpp`、`core/uninstaller/SoftwareEnumerator.h/.cpp`
- **功能**:
  - 列出已安装程序（名称/发布者/大小/安装日期）
  - 按大小排序，标识占用空间大的软件
  - 调用系统卸载程序
  - 卸载后扫描残留文件/注册表
- **数据源**: 注册表 `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall` + `HKCU` 同路径
- **UI**: 列表+排序+搜索+卸载按钮

#### 3.2 重复文件查找
- **新增文件**: `DuplicateFilePanel.h/.cpp`、`core/scanner/DuplicateFileScanner.h/.cpp`
- **功能**:
  - 按文件内容哈希（SHA256）检测重复文件
  - 按大小分组展示
  - 智能选择保留哪个（最新修改/最大/指定路径优先）
  - 删除重复文件（保留一份）
- **UI**: 分组列表+预览+勾选删除

#### 3.3 定时清理
- **文件**: `SettingsPanel.cpp`、新增 `core/scheduler/CleanScheduler.h/.cpp`
- **功能**:
  - 设置自动清理计划（每天/每周/每月）
  - 设置清理级别（快速/标准/深度）
  - 使用Windows计划任务实现
- **UI**: 在设置面板添加"定时清理"分区

#### 3.4 磁盘空间不足提醒
- **文件**: `MainWindow.cpp`、新增 `core/monitor/DiskSpaceMonitor.h/.cpp`
- **功能**:
  - 后台监控C盘剩余空间
  - 低于阈值（如5GB/10%）时弹出通知
  - 一键清理建议
- **实现**: `wxTimer` 定时检查（5分钟间隔），低空间时 `wxNotificationMessage` 弹出通知

---

## 三、实施步骤

### 阶段一：视觉美工（1.1-1.4）
1. NavSidebar 图标升级 — 自绘线性图标替代圆圈+字母
2. CardPanel 交互增强 — 悬停抬升+点击反馈+阴影增强
3. CircularProgress 动画 — 进度平滑过渡+indeterminate旋转
4. 扫描中旋转光点效果

### 阶段二：交互体验（2.1-2.4）
5. DashboardPanel 健康评分
6. 清理进度对话框
7. 累计清理统计
8. Tooltip提示

### 阶段三：功能补齐（3.0-3.4）
9. 非系统进程分析与优化（StartupPanel新增进程管理标签页）
10. 软件卸载管理面板
11. 重复文件查找面板
12. 定时清理设置
13. 磁盘空间不足提醒

---

## 四、设计决策

| 决策点 | 选择 | 理由 |
|--------|------|------|
| 图标方案 | 自绘线性图标（wxGraphicsContext） | 无需外部资源文件，保持单文件分发，wxWidgets原生支持 |
| 动画方案 | wxTimer插值动画 | wxWidgets最简单的动画方式，无需第三方库 |
| 健康评分算法 | 加权评分（5个维度） | 简单有效，维度覆盖全面 |
| 卸载功能 | 调用系统卸载+残留扫描 | 最安全的方式，不直接删除程序文件 |
| 重复文件检测 | SHA256内容哈希 | 最准确，避免误判 |
| 定时清理 | Windows计划任务 | 程序不运行时也能执行 |
| 磁盘提醒 | wxTimer+wxNotificationMessage | 轻量级，无需额外依赖 |

---

## 五、验证步骤

每个阶段完成后：
1. 编译通过（`build_local.bat`）
2. 运行程序，手动验证各改动效果
3. 检查无内存泄漏（detach的线程、Timer的生命周期）
4. 确保现有功能不受影响（扫描/清理/迁移/优化流程正常）
