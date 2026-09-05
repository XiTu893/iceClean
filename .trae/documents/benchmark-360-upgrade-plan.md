# IceClean 全面对标360提升计划（更新版）

## 一、已完成任务

| 任务 | 状态 | 说明 |
|------|------|------|
| 阶段1.3 导航精简重组 | ✅ 完成 | 13项→9项，wxNotebook组合面板 |
| 阶段2.3 强制卸载/强制删除 | ✅ 完成（待编译验证） | Win32Util + SoftwareUninstaller + UninstallPanel |

## 二、待实施任务（按优先级排序）

---

### 任务1：阶段2.1 软件专清扫描器

**目标**：对标360/腾讯管家的微信/QQ专清功能，新增 `SoftwareCacheScanner`

**现状分析**：
- 已有 `DevCacheScanner`（开发工具缓存），继承 `ScannerBase`，模式成熟
- 已有 `WeChatMigrator`/`QQMigrator`，但它们是迁移器（搬移数据），不是扫描器（清理缓存）
- `ScannerAggregator::RegisterBuiltinScanners()` 中硬编码注册所有扫描器
- `DeepCleanPanel` 已有3个标签页（系统清理/注册表清理/隐私清理），需新增"软件专清"标签页

**新增文件**：
1. `src/core/scanner/SoftwareCacheScanner.h` — 软件缓存扫描器头文件
2. `src/core/scanner/SoftwareCacheScanner.cpp` — 软件缓存扫描器实现

**修改文件**：
1. `src/core/scanner/ScannerAggregator.cpp` — 注册 `SoftwareCacheScanner`
2. `src/gui/panels/DeepCleanPanel.h` — 添加"软件专清"标签页的UI结构和数据
3. `src/gui/panels/DeepCleanPanel.cpp` — 创建"软件专清"标签页，集成扫描和清理逻辑

**实现细节**：

**SoftwareCacheScanner** 继承 `ScannerBase`，扫描以下软件缓存：

| 软件 | 缓存路径 | 说明 |
|------|---------|------|
| 微信 | `%USERPROFILE%\Documents\WeChat Files\*\FileStorage\Cache\` | 图片/视频/文件缓存 |
| 微信 | `%USERPROFILE%\Documents\WeChat Files\*\FileStorage\Temp\` | 临时文件 |
| QQ | `%USERPROFILE%\Documents\Tencent Files\*\Image\` | 图片缓存 |
| QQ | `%USERPROFILE%\Documents\Tencent Files\*\Video\` | 视频缓存 |
| 迅雷 | `%USERPROFILE%\AppData\Local\Thunder Network\*\Cache\` | 下载缓存 |
| 爱奇艺 | `%USERPROFILE%\AppData\Local\Qiyi\` | 视频缓存 |
| 腾讯视频 | `%USERPROFILE%\AppData\Local\Tencent\QLive\` | 视频缓存 |
| WPS | `%USERPROFILE%\AppData\Local\Kingsoft\WPS Cloud Files\` | 云端缓存 |
| 钉钉 | `%USERPROFILE%\AppData\Local\DingTalk\` | 缓存文件 |

- `GetName()` → `L"软件缓存"`
- `GetDescription()` → `L"扫描微信、QQ、迅雷等常用软件的缓存文件"`
- `GetSafetyRating()` → `SafetyRating::Caution`
- `GetIcon()` → `L"software_cache"`
- `IsAvailable()` → 检查至少一个软件缓存目录是否存在
- `Scan()` → 逐个调用 `ScanXxxCache()` 子方法，每个子方法调用基类 `ScanDirectory()`

**DeepCleanPanel 集成**：
- 在 `m_notebook` 中添加第4个标签页"软件专清"
- 标签页内容：每个软件一个CheckBox + 大小统计 + 描述
- 点击"扫描"按钮时，调用 `SoftwareCacheScanner::Scan()`，显示扫描结果
- 点击"清理"按钮时，删除选中的缓存目录内容
- 底部"开始清理"按钮根据当前标签页切换文本

**验证步骤**：
1. 编译通过
2. 运行程序，进入"深度清理"→"软件专清"标签页
3. 点击扫描，确认能检测到已安装软件的缓存
4. 勾选并清理，确认缓存被正确删除

---

### 任务2：阶段1.1 统一视觉规范

**目标**：扩展ThemeManager，建立字体和按钮尺寸的统一规范

**现状分析**：
- `ThemeManager` 已有18种颜色定义，但缺少字体和按钮尺寸常量
- 各面板字体硬编码：`wxFont(14, ..., L"微软雅黑")`、`wxFont(10, ...)` 等，字号和字重不统一
- 按钮尺寸硬编码：`wxSize(200, 48)`、`wxSize(160, 40)`、`wxSize(120, 36)` 等
- 品牌色 `#0078D4` 在多处硬编码

**修改文件**：
1. `src/gui/controls/ThemeManager.h` — 添加字体工厂方法和按钮尺寸常量
2. `src/gui/controls/ThemeManager.cpp` — 实现字体工厂方法

**实现细节**：

ThemeManager 新增内容：

```cpp
// 字体工厂方法
static wxFont GetTitleFont();      // 14号粗体 - 页面标题
static wxFont GetSubtitleFont();   // 12号粗体 - 区域标题
static wxFont GetBodyFont();       // 10号常规 - 正文/标签
static wxFont GetSmallFont();      // 9号常规 - 描述/辅助文字
static wxFont GetButtonFont();     // 11号粗体 - 主按钮
static wxFont GetSmallButtonFont();// 10号常规 - 小按钮

// 按钮尺寸常量
struct ButtonSize {
    static constexpr int PrimaryW = 200;   // 主操作按钮宽度
    static constexpr int PrimaryH = 44;    // 主操作按钮高度
    static constexpr int ActionW = 120;    // 操作按钮宽度
    static constexpr int ActionH = 36;     // 操作按钮高度
    static constexpr int SmallW = 80;      // 小按钮宽度
    static constexpr int SmallH = 28;      // 小按钮高度
};
```

- 字体统一使用"微软雅黑"，字号规范：14/12/11/10/9
- 按钮尺寸规范：主操作200x44、操作120x36、小按钮80x28
- 品牌色统一使用 `ThemeManager::Instance().GetColors().accent` 而非硬编码 `wxColour(0x00, 0x78, 0xD4)`

**注意**：此任务只扩展ThemeManager，不修改各面板（面板修改在任务5交互优化中统一进行）

**验证步骤**：
1. 编译通过
2. ThemeManager 新增方法可被其他面板正常调用

---

### 任务3：阶段1.4 首页增强

**目标**：提升首页信息密度，对标360/腾讯管家的首页体验

**现状分析**：
- `DashboardPanel` 已有：C盘空间环形图、健康评分、4个快捷卡片、扫描结果区域
- 缺少：累计清理统计、C盘空间预警、更多快捷操作入口
- 快捷卡片只有4个（临时文件/Windows更新/浏览器缓存/休眠文件），缺少软件管理/启动优化等入口

**修改文件**：
1. `src/gui/panels/DashboardPanel.h` — 添加累计统计和空间预警相关成员
2. `src/gui/panels/DashboardPanel.cpp` — 实现累计统计展示和空间预警

**实现细节**：

1. **累计清理统计**：
   - 在环形图下方添加"累计清理 XX GB"标签
   - 数据来源：读取 `%APPDATA%\IceClean\usage_stats.json`（如不存在则显示"0 GB"）
   - 每次清理完成后更新累计值

2. **C盘空间预警**：
   - 在磁盘信息区域，当C盘剩余空间 < 10% 时，显示红色预警文字"C盘空间不足！"
   - 当C盘剩余空间 < 20% 时，显示黄色提示"C盘空间偏低"

3. **快捷操作卡片扩展**：
   - 将4个卡片扩展为6个，新增"启动加速"和"软件管理"两个快捷入口
   - 点击卡片时导航到对应面板（通过发送 `wxEVT_NAV_SELECTION_CHANGED` 事件）

**验证步骤**：
1. 编译通过
2. 首页显示累计清理统计
3. C盘空间低于阈值时显示预警
4. 新增快捷卡片可点击跳转

---

### 任务4：阶段3 交互优化

**目标**：统一按钮样式、ListCtrl样式、修复Toggle开关

**现状分析**：
- 按钮尺寸和字号在各面板中硬编码，不统一
- ListCtrl 有的用 `wxBORDER_NONE`，有的用 `wxBORDER_SIMPLE`
- StartupPanel 中 Toggle 开关有内存泄漏（`new bool` 未释放）和闪烁问题
- 清理进度对话框缺少实时空间释放计数

**修改文件**：
1. 所有面板的 `.cpp` 文件 — 替换硬编码字体为 `ThemeManager::GetXxxFont()`，替换硬编码颜色为 `ThemeManager::Instance().GetColors().xxx`
2. `src/gui/panels/StartupPanel.h/.cpp` — 修复Toggle内存泄漏，提取为独立控件
3. `src/gui/controls/ToggleSwitch.h/.cpp` — 新建独立Toggle开关控件
4. `src/gui/dialogs/CleanProgressDialog.h/.cpp` — 增强清理进度展示

**实现细节**：

1. **统一按钮样式**（3种规格）：
   - 主操作按钮：200x44, 13号粗体, 品牌色背景, 白色文字
   - 操作按钮：120x36, 10号粗体, 品牌色背景, 白色文字
   - 小按钮：80x28, 9号, 浅色边框, 品牌色文字

2. **统一ListCtrl样式**：
   - `wxBORDER_NONE` + 自绘表头
   - 行高32px，交替行颜色（白色/#F8F9FA）
   - 选中行颜色（#E8F0FE）
   - 表头9号粗体，内容9号

3. **ToggleSwitch 独立控件**：
   - 提取自 StartupPanel，修复内存泄漏
   - 使用 `wxBufferedPaintDC` 防止闪烁
   - 自定义事件 `wxEVT_TOGGLE_CHANGED`

4. **清理进度增强**：
   - 添加已释放空间实时计数
   - 添加分类清理进度条

**验证步骤**：
1. 编译通过
2. 所有面板按钮样式统一
3. Toggle开关无内存泄漏、无闪烁
4. 清理进度对话框显示实时空间释放

---

### 任务5：阶段2.2 软件升级检测

**目标**：检测已安装软件的新版本，对标360/CCleaner的软件升级功能

**现状分析**：
- `SoftwareUninstaller::GetInstalledSoftware()` 已能读取注册表获取已安装软件列表（名称+版本+发布者）
- `UninstallPanel` 已有软件列表展示
- 缺少版本比对和升级提示

**新增文件**：
1. `src/core/analyzer/SoftwareUpdateChecker.h` — 软件升级检测头文件
2. `src/core/analyzer/SoftwareUpdateChecker.cpp` — 软件升级检测实现

**修改文件**：
1. `src/gui/panels/UninstallPanel.h/.cpp` — 添加"可升级"标签页

**实现细节**：

1. **SoftwareUpdateChecker**：
   - 读取已安装软件列表（复用 `SoftwareUninstaller` 的数据）
   - 维护一个常用软件版本查询列表（本地JSON配置文件）
   - 比对本地版本与最新版本，返回可升级软件列表
   - 初期使用本地JSON配置（`resources/software_versions.json`），后续可扩展为在线查询

2. **UninstallPanel 集成**：
   - 在 `m_notebook` 中添加"可升级"标签页
   - 显示软件名称、当前版本、最新版本、升级按钮
   - 升级按钮打开软件官网下载页面

**验证步骤**：
1. 编译通过
2. 软件管理面板显示"可升级"标签页
3. 能正确检测到可升级的软件

---

### 任务6：阶段1.2 深色主题可用化

**目标**：让深色主题真正可用

**现状分析**：
- `ThemeManager` 已实现深色主题颜色方案和切换机制
- 但所有面板硬编码颜色（`*wxWHITE`、`wxColour(0x33,0x33,0x33)` 等），不响应主题变更
- `SettingsPanel` 无主题切换UI入口
- 自绘控件（NavSidebar/CircularProgress/CardPanel/SafetyBadge）不响应主题变更

**修改文件**：
1. `src/gui/panels/SettingsPanel.h/.cpp` — 添加主题切换下拉框
2. 所有面板 `.cpp` — 替换硬编码颜色为 `ThemeManager` 语义颜色
3. `src/gui/controls/NavSidebar.cpp` — 响应主题变更
4. `src/gui/controls/CircularProgress.cpp` — 响应主题变更
5. `src/gui/controls/CardPanel.cpp` — 响应主题变更
6. `src/gui/controls/SafetyBadge.cpp` — 响应主题变更

**实现细节**：

1. **SettingsPanel 主题切换**：
   - 在"常规设置"区域添加"主题"下拉框（浅色/深色/跟随系统）
   - 选择后调用 `ThemeManager::Instance().SetTheme()`

2. **面板颜色替换**：
   - `*wxWHITE` → `ThemeManager::Instance().GetColors().background`
   - `wxColour(0x33,0x33,0x33)` → `ThemeManager::Instance().GetColors().textPrimary`
   - `wxColour(0x99,0x99,0x99)` → `ThemeManager::Instance().GetColors().textSecondary`
   - `wxColour(0x00,0x78,0xD4)` → `ThemeManager::Instance().GetColors().accent`

3. **自绘控件响应**：
   - 注册 `ThemeManager::Instance().RegisterChangeCallback()`
   - 在回调中更新颜色并 `Refresh()`

**验证步骤**：
1. 编译通过
2. 设置面板可切换主题
3. 切换深色主题后，所有面板和控件正确显示深色
4. 重启后主题设置保持

---

### 任务7：阶段4 细节打磨

**目标**：累计统计持久化、C盘监控、注册表备份、操作确认增强

**修改文件**：
1. 新增 `src/core/safety/UsageStats.h/.cpp` — 累计统计持久化
2. `src/gui/panels/DashboardPanel.h/.cpp` — C盘空间监控定时器
3. `src/core/cleaner/RegistryCleaner.h/.cpp` — 清理前自动备份注册表
4. `src/gui/dialogs/ConfirmDialog.h/.cpp` — 危险操作确认增强

**实现细节**：

1. **UsageStats**：
   - 记录累计清理大小、清理次数、拦截次数
   - JSON持久化到 `%APPDATA%\IceClean\usage_stats.json`
   - 提供静态方法 `RecordClean(size)`、`RecordBlock()`、`GetStats()`

2. **C盘空间监控**：
   - DashboardPanel 添加 `wxTimer`，每60秒检查C盘空间
   - 低于10%时红色预警，低于20%时黄色提示

3. **注册表自动备份**：
   - `RegistryCleaner::Clean()` 前自动导出备份到 `%APPDATA%\IceClean\registry_backup\`
   - 使用 `RegExportKey()` 或 `reg export` 命令

4. **操作确认增强**：
   - 危险操作确认对话框添加风险说明
   - 添加3秒倒计时（倒计时结束前"确认"按钮禁用）

**验证步骤**：
1. 编译通过
2. 清理操作后累计统计正确更新
3. C盘空间不足时首页显示预警
4. 注册表清理前自动备份
5. 危险操作确认有倒计时

---

## 三、实施顺序

| 顺序 | 任务 | 优先级 | 预计改动量 |
|------|------|--------|-----------|
| 1 | 阶段2.1 软件专清扫描器 | 高 | 新增2文件+修改3文件 |
| 2 | 阶段1.1 统一视觉规范 | 高 | 修改2文件 |
| 3 | 阶段1.4 首页增强 | 高 | 修改2文件 |
| 4 | 阶段3 交互优化 | 中 | 新增1文件+修改多文件 |
| 5 | 阶段2.2 软件升级检测 | 中 | 新增2文件+修改1文件 |
| 6 | 阶段1.2 深色主题 | 中 | 修改多文件 |
| 7 | 阶段4 细节打磨 | 低 | 新增1文件+修改3文件 |

## 四、假设与决策

1. **软件专清范围**：先实现微信/QQ/迅雷/视频软件4类核心，后续迭代增加
2. **强制删除策略**：使用已有的三轮递进终止策略 + ForceDeleteFile/ForceDeleteDirectory
3. **深色主题**：先确保浅色主题统一，深色主题作为后续迭代
4. **不实现病毒防护**：按用户要求，不对标杀毒功能
5. **软件升级检测**：初期使用本地JSON配置，后续可扩展为在线查询
6. **CMakeLists.txt**：使用 `GLOB_RECURSE`，新增 `.cpp` 文件自动被包含，但需重新运行 CMake 配置
