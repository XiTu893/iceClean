# IceClean - 智能C盘清理与迁移工具 实施计划

## 一、项目概述

打造一款**最傻瓜、最智能**的C盘清理和迁移工具，综合CCleaner、Dism++、Microsoft PC Manager、360安全卫士、腾讯电脑管家、火绒等业界优秀产品的设计精华，实现一键清理彻底、智能迁移大文件、启动加速，让C盘空间充足、系统运行流畅。

**技术栈**: C++20 + wxWidgets 3.3 + CMake + vcpkg

---

## 二、当前状态分析

- 项目目录 `f:\project\iceClean` 为空，需从零搭建
- 目标平台: Windows 10 21H2+ / Windows 11
- C++优势: 原生性能、极低内存占用、直接调用Win32 API、单文件发布无运行时依赖

---

## 三、核心功能模块（借鉴业界最佳实践）

### 模块1: 一键扫描与清理（核心卖点）

**借鉴**: Microsoft PC Manager的一键式 + CCleaner的分类预览 + Dism++的深度清理

| 功能 | 清理目标 | 预估可释放空间 | 安全等级 |
|------|----------|---------------|---------|
| 系统临时文件 | `%TEMP%`、`C:\Windows\Temp` | 1-5GB | 🟢 安全 |
| Windows更新缓存 | `SoftwareDistribution\Download` | 3-10GB | 🟢 安全 |
| 旧Windows安装 | `Windows.old`、`$Windows.~BT`、`$Windows.~WS` | 10-50GB | 🟡 谨慎 |
| WinSxS被取代组件 | DISM API清理 | 2-10GB | 🟡 谨慎 |
| 缩略图缓存 | `thumbcache_*` | 0.1-2GB | 🟢 安全 |
| Prefetch预取 | `C:\Windows\Prefetch\*.pf` | 0.05-0.5GB | 🟢 安全 |
| 错误报告/内存转储 | `Minidump`、`MEMORY.DMP` | 0.1-5GB | 🟢 安全 |
| 回收站 | 已删除未清空文件 | 不定 | 🟢 安全 |
| 传递优化文件 | `DeliveryOptimization` | 0.5-5GB | 🟢 安全 |
| 浏览器缓存 | Chrome/Edge/Firefox Cache | 1-10GB | 🟢 安全 |
| 系统日志 | `C:\Windows\Logs`、`.evtx` | 0.5-2GB | 🟢 安全 |
| 休眠文件 | `hiberfil.sys` | 4-32GB | 🟡 关闭休眠功能 |
| CompactOS压缩 | 系统文件压缩(非删除) | 2-5GB | 🟢 安全 |
| 字体缓存 | `FontCache` | 0.01-0.1GB | 🟢 安全 |
| 旧驱动备份 | 过期驱动副本 | 0.5-2GB | 🟢 安全 |

**交互设计**:
- 首页大按钮「一键扫描」→ 扫描动画(实时显示已发现垃圾大小) → 分类结果页(每类显示大小+安全等级) → 一键清理/逐项选择
- 借鉴Microsoft PC Manager: 默认只勾选🟢安全项，🟡谨慎项需手动展开确认
- 借鉴CCleaner: 清理前展示文件列表预览，可取消勾选
- 借鉴Dism++: 深度清理项用橙色醒目标注风险

### 模块2: 智能迁移（差异化功能）

**借鉴**: 腾讯电脑管家C盘守护者 + Steam官方迁移 + 360 C盘搬家

| 迁移场景 | 实现方式 | 说明 |
|----------|---------|------|
| Steam游戏迁移 | 检测Steam库→自动发现大游戏→robocopy+mklink /J | 借鉴Steam官方移动+符号链接 |
| Epic/Xbox游戏 | 同上，检测对应平台库路径 | 自动发现安装路径 |
| 用户文件夹迁移 | 桌面/文档/下载/图片/视频→移动+修改注册表位置 | Windows原生支持 |
| 微信/QQ缓存迁移 | 检测缓存路径→移动→修改软件配置 | 释放10-50GB |
| 大型软件迁移 | VSCode插件/Anaconda/Adobe→robocopy+mklink /J | Junction链接程序无感知 |
| 自定义文件夹迁移 | 用户选择任意文件夹→移动+创建Junction | 最灵活 |

**交互设计**:
- 「C盘瘦身」页面 → 自动扫描C盘大文件/大文件夹(>500MB) → 按大小排序展示
- 每项显示: 文件夹路径、大小、迁移建议(可迁移/不建议迁移)、目标盘选择
- 迁移过程: 进度条+速度+剩余时间，支持取消
- 迁移完成: 验证链接有效性，显示释放的空间

### 模块3: 启动加速

**借鉴**: Glary Utilities延迟加载 + CCleaner启动项管理 + Autoruns

| 优化项 | 实现方式 | 效果 |
|--------|---------|------|
| 启动项管理 | 读取注册表`Run`/`RunOnce`+任务管理器API | 开机提速12-25秒 |
| 服务优化 | `EnumServicesStatusEx`枚举服务→建议禁用项 | 减少后台资源占用 |
| 计划任务管理 | `ITaskService` COM接口 | 减少登录延迟 |
| 延迟启动 | 将非紧急项设为延迟30s/60s启动 | 分散IO压力 |

**安全保护**: 内置白名单，关键系统服务不可禁用(Windows Update/Security/Firewall/DHCP/DNS)

### 模块4: 磁盘空间分析

**借鉴**: WizTree/SpaceSniffer的矩形树图 + CCleaner磁盘分析器

- 扫描C盘所有文件/文件夹，计算大小
- 矩形树图(Treemap)可视化展示空间占用
- 按文件类型/文件夹深度筛选
- 点击矩形→定位到具体文件/文件夹
- 支持直接删除或迁移操作

### 模块5: 安全保护机制

**借鉴**: CCleaner注册表备份 + Dism++安全评级 + Windows系统还原

| 保护机制 | 实现 |
|----------|------|
| 清理前创建系统还原点 | `SRSetRestorePoint` Win32 API |
| 注册表备份 | 清理注册表前导出备份文件(`RegSaveKey`) |
| 安全等级标注 | 🟢安全/🟡谨慎/🔴危险 三级标注 |
| 预览模式 | 清理前展示将删除的文件完整列表 |
| 白名单机制 | 内置不可清理路径白名单 |
| 操作日志 | 记录所有清理/迁移操作，支持回溯 |
| 二次确认 | 危险操作(删Windows.old/关休眠)弹窗二次确认 |

---

## 四、项目架构

```
IceClean/
├── CMakeLists.txt                        # CMake根配置
├── CMakePresets.json                     # CMake预设(开发/发布)
├── vcpkg.json                            # vcpkg依赖清单
├── vcpkg-configuration.json              # vcpkg配置
│
├── src/
│   ├── main.cpp                          # 程序入口
│   ├── App.h / App.cpp                   # wxApp派生应用类
│   │
│   ├── gui/                              # GUI层(wxWidgets)
│   │   ├── MainWindow.h/.cpp            # 主窗口(侧边导航+内容区)
│   │   ├── panels/                       # 各功能面板
│   │   │   ├── DashboardPanel.h/.cpp     # 首页仪表盘
│   │   │   ├── ScanResultPanel.h/.cpp    # 扫描结果页
│   │   │   ├── DeepCleanPanel.h/.cpp     # 深度清理页
│   │   │   ├── MigrationPanel.h/.cpp     # 智能迁移页
│   │   │   ├── StartupPanel.h/.cpp       # 启动加速页
│   │   │   ├── DiskAnalyzerPanel.h/.cpp  # 磁盘分析页
│   │   │   └── SettingsPanel.h/.cpp      # 设置页
│   │   ├── controls/                     # 自定义控件
│   │   │   ├── TreemapCtrl.h/.cpp        # 矩形树图控件
│   │   │   ├── CircularProgress.h/.cpp   # 圆形进度条
│   │   │   ├── SafetyBadge.h/.cpp        # 安全等级标识
│   │   │   ├── NavSidebar.h/.cpp         # 侧边导航栏
│   │   │   └── CardPanel.h/.cpp          # 卡片容器面板
│   │   ├── dialogs/                      # 对话框
│   │   │   ├── ConfirmDialog.h/.cpp      # 确认对话框
│   │   │   ├── MigrationProgressDlg.h/.cpp # 迁移进度对话框
│   │   │   └── FilePreviewDlg.h/.cpp     # 文件预览对话框
│   │   └── resources/                    # 资源文件
│   │       ├── resource.h                # 资源ID定义
│   │       ├── app.rc                    # Windows资源文件(图标/清单)
│   │       ├── app.manifest              # UAC管理员权限清单
│   │       └── icons/                    # 图标资源
│   │
│   ├── core/                             # 核心业务逻辑
│   │   ├── scanner/                      # 扫描器
│   │   │   ├── IScanner.h                # 扫描器接口
│   │   │   ├── ScannerBase.h/.cpp        # 扫描器基类
│   │   │   ├── SystemTempScanner.h/.cpp  # 系统临时文件扫描
│   │   │   ├── WindowsUpdateScanner.h/.cpp # 更新缓存扫描
│   │   │   ├── BrowserCacheScanner.h/.cpp # 浏览器缓存扫描
│   │   │   ├── WinSxSScanner.h/.cpp      # WinSxS组件扫描
│   │   │   ├── HibernationScanner.h/.cpp # 休眠文件扫描
│   │   │   ├── RecycleBinScanner.h/.cpp  # 回收站扫描
│   │   │   ├── LogScanner.h/.cpp         # 日志文件扫描
│   │   │   ├── ThumbnailScanner.h/.cpp   # 缩略图缓存扫描
│   │   │   ├── PrefetchScanner.h/.cpp    # 预取文件扫描
│   │   │   ├── CrashDumpScanner.h/.cpp   # 崩溃转储扫描
│   │   │   ├── DriverBackupScanner.h/.cpp # 旧驱动备份扫描
│   │   │   ├── DeliveryOptScanner.h/.cpp # 传递优化扫描
│   │   │   └── ScannerAggregator.h/.cpp  # 扫描器聚合(一键扫描)
│   │   ├── cleaner/                      # 清理器
│   │   │   ├── ICleaner.h                # 清理器接口
│   │   │   ├── CleanerBase.h/.cpp        # 清理器基类
│   │   │   ├── FileCleaner.h/.cpp        # 文件删除清理
│   │   │   ├── DismCleaner.h/.cpp        # DISM API深度清理
│   │   │   ├── CompactOSCleaner.h/.cpp   # CompactOS压缩
│   │   │   ├── HibernationCleaner.h/.cpp # 休眠文件处理
│   │   │   ├── RecycleBinCleaner.h/.cpp  # 回收站清空
│   │   │   └── BrowserCleaner.h/.cpp     # 浏览器缓存清理
│   │   ├── migrator/                     # 迁移器
│   │   │   ├── IMigrator.h               # 迁移器接口
│   │   │   ├── MigratorBase.h/.cpp       # 迁移器基类
│   │   │   ├── FolderMigrator.h/.cpp     # 通用文件夹迁移
│   │   │   ├── SteamMigrator.h/.cpp      # Steam游戏迁移
│   │   │   ├── UserFolderMigrator.h/.cpp # 用户文件夹迁移
│   │   │   ├── WeChatMigrator.h/.cpp     # 微信缓存迁移
│   │   │   ├── QQMigrator.h/.cpp         # QQ缓存迁移
│   │   │   └── LargeFolderDetector.h/.cpp # 大文件夹检测
│   │   ├── optimizer/                    # 优化器
│   │   │   ├── StartupOptimizer.h/.cpp   # 启动项优化
│   │   │   ├── ServiceOptimizer.h/.cpp   # 服务优化
│   │   │   └── ScheduledTaskOptimizer.h/.cpp # 计划任务优化
│   │   ├── analyzer/                     # 分析器
│   │   │   ├── DiskSpaceAnalyzer.h/.cpp  # 磁盘空间分析
│   │   │   └── TreemapBuilder.h/.cpp     # 矩形树图数据构建
│   │   └── safety/                       # 安全保护
│   │       ├── SafetyRating.h            # 安全等级枚举
│   │       ├── WhitelistProvider.h/.cpp  # 白名单提供者
│   │       ├── RestorePointManager.h/.cpp # 系统还原点管理
│   │       └── OperationLogger.h/.cpp    # 操作日志记录
│   │
│   ├── models/                           # 数据模型
│   │   ├── ScanResult.h/.cpp            # 扫描结果
│   │   ├── CleanItem.h/.cpp             # 清理项
│   │   ├── MigrationItem.h/.cpp         # 迁移项
│   │   ├── StartupItem.h/.cpp           # 启动项
│   │   ├── DiskNode.h/.cpp              # 磁盘节点(树图)
│   │   └── OperationRecord.h/.cpp       # 操作记录
│   │
│   └── utils/                            # 工具类
│       ├── Win32Util.h/.cpp             # Win32 API封装
│       ├── JunctionPoint.h/.cpp         # 符号链接/Junction操作
│       ├── DismUtil.h/.cpp              # DISM命令行封装
│       ├── RegistryUtil.h/.cpp          # 注册表操作封装
│       ├── ShellUtil.h/.cpp             # Shell API(回收站等)
│       ├── TaskSchedulerUtil.h/.cpp     # 任务计划COM接口封装
│       ├── FileUtil.h/.cpp              # 文件操作工具
│       ├── FormatUtil.h/.cpp            # 格式化工具(大小/时间)
│       ├── JsonUtil.h/.cpp              # JSON配置读写
│       └── ThreadUtil.h/.cpp            # 线程/并发工具
│
├── tests/                                # 测试
│   ├── CMakeLists.txt
│   ├── test_scanner.cpp
│   ├── test_cleaner.cpp
│   └── test_migrator.cpp
│
└── installer/                            # 安装程序
    └── IceClean.nsi                      # NSIS安装脚本
```

---

## 五、UI设计方案

### 整体布局（借鉴Microsoft PC Manager + 火绒）

```
┌─────────────────────────────────────────────────┐
│  IceClean                              _ □ ✕    │
├──────────┬──────────────────────────────────────┤
│          │                                      │
│  首页     │   ┌──────────────────────────────┐   │
│          │   │    C盘健康度: 72%             │   │
│  清理     │   │    ████████░░░░  78.5GB/120GB│   │
│          │   │                              │   │
│  迁移     │   │  [  一键扫描  ]  大按钮      │   │
│          │   │                              │   │
│  加速     │   │  快捷入口:                   │   │
│          │   │  临时文件   更新缓存           │   │
│  分析     │   │  浏览器     休眠文件         │   │
│          │   │                              │   │
│  设置     │   │  最近操作:                   │   │
│          │   │  * 清理临时文件 释放2.3GB     │   │
│          │   │  * 迁移Steam游戏 释放45GB     │   │
│          │   └──────────────────────────────┘   │
│          │                                      │
└──────────┴──────────────────────────────────────┘
```

### 设计风格
- **现代扁平化**: 圆角面板、微妙渐变、卡片式布局
- **配色**: 深蓝主色(#0078D4) + 白色背景 + 安全等级色(绿/橙/红)
- **字体**: 微软雅黑/Segoe UI (wxFont设置)
- **图标**: 自绘SVG图标或使用wxArtProvider + 内嵌PNG
- **动画**: wxTimer驱动的扫描动画、进度条动画

### wxWidgets关键控件映射

| 功能需求 | wxWidgets控件 | 说明 |
|----------|--------------|------|
| 侧边导航 | wxListBox / 自绘wxPanel | 自绘导航按钮+图标+选中高亮 |
| 内容区切换 | wxSimplebook | 隐藏/显示不同面板 |
| 圆形进度条 | 自绘wxPanel | wxBufferedPaintDC + GDI+绘制 |
| 卡片布局 | wxStaticBoxSizer | 带标题的分组框 |
| 分类列表 | wxTreeListCtrl | 可折叠树形列表 |
| 安全等级标识 | 自绘wxPanel | 彩色圆点+文字 |
| 矩形树图 | 自绘wxPanel | Squarified算法+wxBufferedPaintDC |
| 进度条 | wxGauge / wxProgressDialog | 迁移进度 |
| 开关按钮 | 自绘wxPanel | Toggle开关样式 |
| 系统托盘 | wxTaskBarIcon | 最小化到托盘 |

### 关键页面

**1. 首页仪表盘 (DashboardPanel)**
- C盘空间使用率环形图(自绘wxPanel，wxGraphicsContext绘制弧形)
- 一键扫描大按钮(借鉴Microsoft PC Manager，大号圆角按钮)
- 快捷入口卡片(4个常用清理项)
- 最近操作记录(wxListCtrl)

**2. 扫描结果页 (ScanResultPanel)**
- 分类列表(wxTreeListCtrl): 每类显示图标+名称+大小+安全等级标识
- 底部汇总: 总可释放空间 + 「一键清理」按钮
- 点击分类展开文件详情列表
- 🟡谨慎项默认折叠，需点击展开确认

**3. 深度清理页 (DeepCleanPanel)**
- 分标签页(wxNotebook): 系统清理 / 注册表清理 / 隐私清理
- 每项带安全等级标识和说明文字
- WinSxS清理、CompactOS压缩等高级功能
- 操作前强制创建还原点

**4. 智能迁移页 (MigrationPanel)**
- 自动扫描大文件/大文件夹列表(wxListCtrl，按大小降序)
- 每项显示: 图标+名称+大小+路径+迁移建议
- 目标盘选择下拉框(wxChoice)
- 迁移进度弹窗(wxProgressDialog)
- 迁移历史记录

**5. 启动加速页 (StartupPanel)**
- 启动项列表(自定义wxPanel，含Toggle开关)
- 服务列表(建议禁用项标注)
- 延迟启动设置
- 优化效果预估

**6. 磁盘分析页 (DiskAnalyzerPanel)**
- 矩形树图可视化(自绘TreemapCtrl)
- 文件类型筛选(wxCheckListBox)
- 右键菜单(wxMenu): 删除/迁移/属性

---

## 六、实施步骤

### 第1步: 项目初始化与基础架构
1. 创建CMake项目结构 + vcpkg配置
2. 配置wxWidgets 3.3依赖
3. 创建app.manifest(管理员权限UAC)
4. 实现App类(wxApp派生) + MainWindow(主窗口框架)
5. 实现侧边导航(NavSidebar) + 内容区切换(wxSimplebook)
6. 实现基础工具类: Win32Util, FileUtil, FormatUtil, RegistryUtil
7. 实现数据模型: ScanResult, CleanItem, MigrationItem等
8. 编译验证，确保窗口正常显示

### 第2步: 核心扫描与清理功能
1. 实现IScanner/ICleaner接口和ScannerBase/CleanerBase基类
2. 实现各扫描器(按优先级):
   - SystemTempScanner (系统临时文件)
   - WindowsUpdateScanner (更新缓存)
   - RecycleBinScanner (回收站)
   - BrowserCacheScanner (浏览器缓存)
   - ThumbnailScanner (缩略图缓存)
   - PrefetchScanner (预取文件)
   - LogScanner (系统日志)
   - CrashDumpScanner (崩溃转储)
   - DriverBackupScanner (旧驱动备份)
   - DeliveryOptScanner (传递优化)
   - HibernationScanner (休眠文件)
   - WinSxSScanner (WinSxS组件)
3. 实现ScannerAggregator (聚合一键扫描，std::thread并行)
4. 实现各清理器:
   - FileCleaner (通用文件删除)
   - RecycleBinCleaner (回收站清空，SHEmptyRecycleBin)
   - BrowserCleaner (浏览器缓存清理)
   - DismCleaner (DISM命令行封装)
   - HibernationCleaner (powercfg /hibernate off)
   - CompactOSCleaner (Compact命令)
5. 实现安全保护: WhitelistProvider, SafetyRating, RestorePointManager
6. 实现DashboardPanel + ScanResultPanel UI
7. 实现一键扫描→结果展示→一键清理完整流程

### 第3步: 智能迁移功能
1. 实现JunctionPoint (CreateSymbolicLink/DeviceIoControl FSCTL_SET_REPARSE_POINT)
2. 实现LargeFolderDetector (大文件夹检测)
3. 实现FolderMigrator (通用迁移: SHFileOperation复制+创建Junction)
4. 实现SteamMigrator (解析libraryfolders.vdf+appmanifest_*.acf)
5. 实现UserFolderMigrator (SHGetFolderPath+注册表修改)
6. 实现WeChatMigrator/QQMigrator (聊天缓存迁移)
7. 实现MigrationPanel UI + MigrationProgressDlg

### 第4步: 启动加速功能
1. 实现StartupOptimizer (RegEnumValue读取Run/RunOnce)
2. 实现ServiceOptimizer (EnumServicesStatusEx+服务建议白名单)
3. 实现ScheduledTaskOptimizer (ITaskService COM接口)
4. 实现StartupPanel UI

### 第5步: 磁盘空间分析
1. 实现DiskSpaceAnalyzer (FindFirstFile/FindNextFile递归扫描)
2. 实现TreemapBuilder (Squarified算法构建矩形数据)
3. 实现TreemapCtrl (wxPanel自绘，wxBufferedPaintDC)
4. 实现DiskAnalyzerPanel UI

### 第6步: 深度清理与设置
1. 实现DeepCleanPanel (wxNotebook标签页)
2. 实现SettingsPanel (常规设置/清理规则/白名单/操作日志)
3. 实现OperationLogger (JSON格式日志读写)
4. 实现wxTaskBarIcon系统托盘

### 第7步: 打磨与优化
1. UI动画效果(wxTimer驱动扫描动画、进度动画)
2. 多线程优化(std::thread + std::mutex并行扫描)
3. 异常处理与容错(__try/__except + 返回值检查)
4. NSIS安装程序打包
5. Release编译优化(/O2 /GL /LTCG)

---

## 七、关键技术实现要点

### 7.1 管理员权限
- app.manifest中设置 `requireAdministrator` level
- CMake中嵌入manifest: `rc文件引用app.manifest`
- 需要管理员权限的操作: DISM清理、服务管理、符号链接创建、系统还原点

### 7.2 DISM命令行封装
```cpp
// CreateProcess调用DISM命令
// Dism.exe /Online /Cleanup-Image /StartComponentCleanup /ResetBase
// 通过管道读取输出，解析进度
class DismUtil {
    static bool StartComponentCleanup(bool resetBase);
    static bool CompactOS();
};
```

### 7.3 Junction链接创建
```cpp
// 方法1: DeviceIoControl + FSCTL_SET_REPARSE_POINT (最可靠)
// 方法2: CreateSymbolicLink with SYMBOLIC_LINK_FLAG_DIRECTORY
// 方法3: 调用mklink /J (最简单)
class JunctionPoint {
    static bool Create(const std::wstring& source, const std::wstring& target);
    static bool Remove(const std::wstring& path);
    static bool IsJunction(const std::wstring& path);
};
```

### 7.4 浏览器缓存检测
- Chrome: `%LocalAppData%\Google\Chrome\User Data\Default\Cache`
- Edge: `%LocalAppData%\Microsoft\Edge\User Data\Default\Cache`
- Firefox: `%LocalAppData%\Mozilla\Firefox\Profiles\*.default-release\cache2`
- 需检测浏览器是否正在运行(CreateToolhelp32Snapshot枚举进程)

### 7.5 Steam游戏检测
- 读取 `libraryfolders.vdf` 文件获取Steam库路径
- 解析 `appmanifest_*.acf` 文件获取游戏信息(名称、大小、安装路径)
- 使用简单的键值对解析器(非完整VDF解析)

### 7.6 微信/QQ缓存检测
- 微信: 读取注册表或默认路径 `Documents\WeChat Files\wxid_*`
- QQ: 读取配置文件或默认路径 `Documents\Tencent Files\QQ号`

### 7.7 矩形树图算法
- Squarified Treemap算法
- 递归布局: 按面积降序排列，选择较短短边方向填充
- wxBufferedPaintDC + wxGraphicsContext绘制圆角矩形

### 7.8 多线程扫描
```cpp
// 使用std::thread + std::mutex + wxThreadEvent
// 扫描在后台线程执行，通过wxThreadEvent更新UI
// ScannerAggregator中每个Scanner独立线程
class ScannerAggregator {
    void ScanAll(wxEvtHandler* handler);  // 发送wxEVT_SCAN_PROGRESS/COMPLETE事件
};
```

### 7.9 系统还原点
```cpp
// 使用SRSetRestorePoint API
// 需要链接SrClient.lib
class RestorePointManager {
    static bool CreateRestorePoint(const std::wstring& description);
    static bool RestoreToPoint(int sequenceNumber);
};
```

---

## 八、依赖管理

### vcpkg依赖

| 包名 | 用途 |
|------|------|
| `wxwidgets` | GUI框架 |
| `nlohmann-json` | JSON配置读写 |
| `spdlog` | 日志记录 |
| `gtest` | 单元测试 |

### 系统库(直接链接)

| 库 | 用途 |
|------|------|
| `SrClient.lib` | 系统还原点API |
| `Psapi.lib` | 进程信息 |
| `Shlwapi.lib` | Shell工具函数 |
| `Ole32.lib` / `OleAut32.lib` | COM接口(任务计划) |
| `Taskschd.lib` | 任务计划COM |
| `Winhttp.lib` | HTTP请求(可选更新检查) |

---

## 九、假设与决策

| 决策项 | 选择 | 理由 |
|--------|------|------|
| C++标准 | C++20 | 协程、ranges、format等现代特性 |
| GUI框架 | wxWidgets 3.3 | 原生Windows控件、轻量、无额外运行时 |
| 构建系统 | CMake + vcpkg | 跨平台构建、依赖管理自动化 |
| 编译器 | MSVC 2022 (v143) | Windows最佳C++编译器，优化出色 |
| 编码 | UTF-8 (/utf-8) | 统一编码，避免中文乱码 |
| UI风格 | 现代扁平化 | wxWidgets原生渲染，圆角+卡片 |
| 迁移方式 | Junction链接 | 程序无感知，兼容性最好 |
| 深度清理 | DISM命令行 | 比API更稳定，兼容性好 |
| 发布方式 | 静态链接单文件 | 无运行时依赖，绿色免安装 |
| 是否驻留后台 | 可选(默认不驻留) | 借鉴火绒/Dism++，不打扰用户 |
| 是否有广告 | 无 | 借鉴火绒，纯净无广告 |

---

## 十、验证步骤

1. **功能验证**: 逐模块测试扫描→清理/迁移完整流程
2. **安全验证**: 确认白名单机制有效，危险操作有二次确认
3. **兼容性验证**: 在Windows 10 21H2和Windows 11上测试
4. **性能验证**: 扫描100GB+垃圾文件时内存占用<50MB，扫描时间<30秒
5. **迁移验证**: 迁移后程序能正常运行(验证Junction链接有效性)
6. **还原验证**: 清理后通过还原点能回滚
7. **体积验证**: 静态链接发布单文件<15MB
