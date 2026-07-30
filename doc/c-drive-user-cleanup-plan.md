# IceClean C 盘用户目录清理与搬迁提升方案

> 制定日期：2026-07-30
> 聚焦：「用户目录(AppData)深度清理」+「C 盘用户文件夹搬迁」两大方向

---

## 一、问题分析

C 盘用户目录 (`C:\Users\<用户名>`) 随时间推移会不可控地增长，主要来源：

| 来源 | 典型路径 | 典型大小 | 增长原因 |
|------|---------|---------|---------|
| AppData\Local\Temp | `%LOCALAPPDATA%\Temp` | 1-10 GB | 软件临时文件未清理 |
| 浏览器缓存 | `%LOCALAPPDATA%\Google\Chrome\User Data\Default\Cache` | 2-20 GB | 浏览器缓存无上限增长 |
| Electron 应用缓存 | `%LOCALAPPDATA%\Discord`, `%LOCALAPPDATA%\Slack`, `%APPDATA%\Code` | 2-15 GB | Electron 应用各自缓存数据 |
| UWP 包数据 | `%LOCALAPPDATA%\Packages` | 5-30 GB | 商店应用缓存和数据 |
| 微信/QQ 缓存 | `%USERPROFILE%\Documents\WeChat Files`, `\Tencent Files` | 5-50 GB | 聊天记录、图片、视频缓存 |
| 下载文件夹 | `%USERPROFILE%\Downloads` | 5-50 GB | 用户堆积下载文件 |
| 回收站 | `C:\$Recycle.Bin` | 1-20 GB | 已删除文件未清空 |
| Windows 旧版本 | `C:\Windows.old` | 10-30 GB | 系统更新残留 |
| 休眠文件 | `C:\hiberfil.sys` | 4-20 GB | 休眠功能占用 |
| 虚拟内存 | `C:\pagefile.sys` | 4-16 GB | 虚拟内存文件 |
| NuGet/npm 缓存 | `%LOCALAPPDATA%\NuGet`, `%APPDATA%\npm` | 1-5 GB | 开发工具包缓存 |
| Docker/WSL 数据 | `%LOCALAPPDATA%\Docker`, `%LOCALAPPDATA%\Packages\*\LocalState` | 10-50 GB | 容器/子系统镜像和数据 |

---

## 二、业界工具对标分析

### 2.1 磁盘空间分析工具

| 工具 | 类型 | 核心能力 | IceClean 差距 |
|------|------|---------|-------------|
| **WizTree** | 免费/闭源 | 直接读取 NTFS MFT，扫描速度极快(秒级)，可视化矩形树图 | ❌ 已有 DiskSpaceAnalyzer 矩形树图但无 MFT 直读，扫描大目录慢 |
| **TreeSize** | 免费/商业 | 文件资源管理器风格界面，按大小排序，支持文件类型统计、PDF 报告 | ❌ 无文件类型统计、无导出报告功能 |
| **WinDirStat** | 开源免费 | 经典矩形树图，支持文件扩展名统计图 | ✅ 已有类似矩形树图，缺扩展名统计 |
| **SpaceSniffer** | 免费 | 动态 treemap 可视化，可钻取 | ❌ 无钻取交互能力 |
| **Dism++** | 开源免费 | 空间回收(系统更新清理/临时文件/AppX 清理)，系统优化 | ❌ 无 DISM 集成空间回收 |

### 2.2 用户目录清理工具

| 工具 | 类型 | 核心能力 | IceClean 差距 |
|------|------|---------|-------------|
| **CCleaner** | 商业 | 浏览器缓存清理、AppData 临时文件、系统临时文件 | ✅ 有类似能力，但浏览器覆盖和清理深度不足 |
| **BleachBit** | 开源 | 跨平台清理，支持 Win+Linux，CLI 自动化 | ❌ 无 CLI 模式 |
| **PrivaZer** | 免费/商业 | 深度隐私清理，扫描用户活动痕迹(最近文档、运行历史、剪贴板等) | ❌ 隐私清理深度不足 |
| **System Ninja** | 免费 | 垃圾文件扫描、重复文件、大文件管理 | ❌ 部分覆盖 |
| **Storage Sense** | Windows 内置 | 自动清理临时文件，可定时运行 | ❌ 无定时自动清理 |

### 2.3 C 盘搬迁/迁移工具

| 工具 | 类型 | 核心能力 | IceClean 差距 |
|------|------|---------|-------------|
| **Steam Mover** | 免费 | 通过 Junction 链接将游戏目录从 C 盘迁移到其他盘 | ✅ 已有 FolderMigrator/Junction 能力 |
| **FolderMove** | 免费 | 图形化界面将任意文件夹迁移到其他盘并创建符号链接 | ✅ 已有类似能力但缺 GUI 引导 |
| **Link Shell Extension** | 免费 | 右键扩展创建符号链接/硬链接/Junction | ❌ 无右键集成 |
| **Windows 设置** | 内置 | 设置 → 存储 → 更改新内容的保存位置 | ❌ 无类似引导配置 |
| **符号链接工具 (mklink)** | 内置 CLI | `mklink /J` 命令创建 Junction | ✅ 已通过 JunctionPoint 工具类封装 |

### 2.4 综合评估：IceClean 现状

| 领域 | IceClean 现有能力 | 完整度 | 核心不足 |
|------|-----------------|--------|---------|
| **磁盘空间分析** | DiskSpaceAnalyzer 矩形树图 | ⚠️ 50% | 无 MFT 直读速度慢、无文件类型统计、无导出报告 |
| **C盘垃圾扫描** | 12 类扫描器 | ✅ 80% | AppData 深度不够、缺 Electron/浏览器缓存专项扫描 |
| **用户文件夹迁移** | UserFolderMigrator + FolderMigrator + Junction | ✅ 85% | 缺 UI 引导流程、缺迁移前评估报告 |
| **大文件夹检测** | LargeFolderDetector | ⚠️ 60% | 只扫一级二级目录，深度不够 |
| **软件缓存清理** | SoftwareCacheScanner | ✅ 75% | 覆盖软件不够全、缺 Electron 应用清理 |
| **系统临时清理** | SystemTempScanner | ✅ 90% | 功能基本完整 |
| **迁移回滚** | RollbackManager | ✅ 80% | 迁移回滚依赖备份 |

---

## 三、提升方案

### 3.1 文件类型统计与导出（对标 TreeSize/WizTree）

```cpp
// 新增: src/core/analyzer/FileTypeAnalyzer.h
class FileTypeAnalyzer {
    // 按文件扩展名统计分析
    struct FileTypeStat {
        std::wstring extension;         // .pdf, .log, .tmp...
        std::wstring category;          // 文档/图片/视频/缓存/日志...
        uint64_t totalSize = 0;
        int fileCount = 0;
        std::wstring typicalPath;       // 典型案例路径
        bool canClean = false;          // 是否可安全清理
    };

    std::vector<FileTypeStat> Analyze(const std::wstring& path);
    bool ExportReport(const std::vector<FileTypeStat>& stats, const std::wstring& outputPath);
};
```

| 具体任务 | 价值 | 工作量 |
|---------|------|-------|
| 文件类型分类统计 | 用户直观了解哪些类型文件占空间 | 2天 |
| HTML/PDF/TXT 报告导出 | 保存分析结果 | 1天 |
| 图表增强(饼图/柱状图) | 可视化占比 | 2天 |

### 3.2 AppData 深度扫描器（核心新增）

```
%LOCALAPPDATA% 专项扫描目录清单:
├── Temp/                           ← 临时文件（安全）
├── Google/Chrome/User Data/Default/Cache/    ← 浏览器缓存
├── Google/Chrome/User Data/Default/Code Cache/
├── Microsoft/Edge/User Data/Default/Cache/
├── Microsoft/Windows/INetCache/   ← IE/Edge 缓存
├── Discord/Cache/
├── Discord/Code Cache/
├── Slack/Cache/
├── Slack/Service Worker/CacheStorage/
├── Microsoft/VS/    ← Visual Studio 缓存
├── Microsoft/vcpkg/ ← vcpkg 下载缓存
├── npm-cache/       ← npm 包缓存
├── NuGet/Cache/     ← NuGet 包缓存
├── PipCache/        ← Python pip 缓存
├── caching/XXX/     ← 其他软件缓存
├── Packages/XXX/LocalState/  ← UWP 应用缓存
│   ├── Microsoft.Widgets*
│   ├── Microsoft.Windows.Photos*
│   └── (其他 UWP 应用)
```

```cpp
// 新增: src/core/scanner/AppDataScanner.h
class AppDataScanner : public ScannerBase {
    // 扫描 %LOCALAPPDATA% 下所有可清理缓存目录
    // 扫描 %APPDATA% 下可清理缓存
    // 专项: 浏览器缓存、Electron 缓存、IDE 缓存、包管理器缓存
    // 安全等级区分: 临时文件 Safe / 缓存 Caution / 应用数据 Dangerous
};
```

| 具体任务 | 工作量 |
|---------|-------|
| 定义 AppData 可清理路径清单 | 1天 |
| 实现扫描逻辑 | 2天 |
| 安全等级分类(哪些可删、哪些需保留) | 1天 |
| Electron 应用缓存专项 (Discord/Slack/VS Code/Teams) | 1天 |
| 包管理器缓存 (npm/NuGet/pip/vcpkg/Cargo) | 1天 |
| UWP Package 缓存清理 | 1天 |

### 3.3 系统还原点与休眠文件管理（增强）

```cpp
// 新增: src/core/scanner/SystemFileScanner.h
class SystemFileScanner : public ScannerBase {
    // 扫描 Windows.old（系统更新残留）
    // 扫描 hiberfil.sys (休眠文件)
    // 扫描 pagefile.sys (虚拟内存)
    // 扫描 $Recycle.Bin (回收站)
    // 扫描 Upgrade 目录 (Windows 升级日志)
};
```

| 具体任务 | 工作量 |
|--------|-------|
| 扫描 Windows.old 目录大小并提示清理 | 0.5天 |
| 休眠文件管理(查询/禁用/缩小比例) | 1天 |
| 虚拟内存配置引导 | 0.5天 |
| 系统升级日志清理 | 0.5天 |

### 3.4 用户文件夹搬迁 UI 引导（核心增强）

现有 `UserFolderMigrator` + `FolderMigrator` 功能完整但缺 UI 引导流程。

**搬迁引导流程设计：**

```
Step 1: 扫描评估
  ├── 分析 C:\Users\ 下各文件夹大小
  ├── 检查目标驱动器(如 D:\) 可用空间
  ├── 生成搬迁建议报告（推荐迁移哪些文件夹）
  └── 预估搬迁后可释放 C 盘空间

Step 2: 搬迁执行
  ├── 用户勾选待迁移文件夹
  ├── 选择目标驱动器
  ├── 执行: 复制 → 删除原目录 → 创建 Junction
  ├── 更新注册表 (User Shell Folders)
  └── (可选) 搬迁前自动创建还原点

Step 3: 完成确认
  ├── 验证 Junction 是否创建成功
  ├── 验证新路径文件完整性
  ├── 如失败自动回滚
  └── 显示最终释放空间
```

| 具体任务 | 工作量 |
|---------|-------|
| 多选 UI 面板（源文件夹列表+勾选） | 1天 |
| 目标驱动器选择和空间检查 | 0.5天 |
| 搬迁进度条和状态显示 | 0.5天 |
| 搬迁前评估报告生成 | 1天 |
| 搬迁后验证和回滚提示 | 1天 |

### 3.5 下载文件夹管理（新增）

下载文件夹是所有用户共同痛点。

```cpp
// 新增: src/core/analyzer/DownloadManager.h
class DownloadManager {
    // 扫描下载文件夹文件分类
    struct DownloadItem {
        std::wstring path;
        std::wstring name;
        uint64_t size;
        FILETIME lastAccessTime;
        std::wstring category;   // 安装包/文档/图片/视频/其他
        bool isOrphaned = false; // 是否已被安装（安装包判断）
    };

    // 获取安装包类文件（exe/msi/zip 且未使用）
    std::vector<DownloadItem> GetInstallers();
    // 获取长时间未访问文件（>90天）
    std::vector<DownloadItem> GetInactiveFiles(int days);
    // 按类型分组清理建议
    std::vector<DownloadItem> GetCleanupSuggestions();
    // 一键清理过期安装包
    int CleanInstallers();
    // 移动久未访问文件到目标盘
    int MoveInactiveFiles(const std::wstring& targetPath, int days);
};
```

| 具体任务 | 工作量 |
|---------|-------|
| 下载文件夹文件分类扫描 | 1天 |
| 安装包识别（exe/msi/iso 且未使用 >30天） | 1天 |
| 长时间未访问文件迁移 | 1天 |
| 一键清理/迁移 UI 面板 | 1天 |

### 3.6 缓存分类深度清理（增强现有 SoftwareCacheScanner）

新增清理类别：

| 新增类别 | 路径 | 安全等级 | 说明 |
|---------|------|---------|------|
| Electron 缓存 | `%LOCALAPPDATA%\Discord`, `\Slack`, `\Postman`, `\Figma`, `\Notion`, `\Telegram` | 🟡 谨慎 | 可清理 Cache 但保留配置 |
| 包管理器缓存 | `%LOCALAPPDATA%\npm-cache`, `\NuGet\Cache`, `\pip\Cache`, `\Cargo\Registry`, `\vcpkg\downloads` | 🟢 安全 | 全部可清 |
| IDE 缓存 | `%APPDATA%\Code\Cache`, `\Code\CachedData`, `%LOCALAPPDATA%\JetBrains*\cache` | 🟡 谨慎 | 清缓存不影响项目 |
| 浏览器扩展数据 | `%LOCALAPPDATA%\Google\Chrome\User Data\Default\Extensions` | 🔴 危险 | 仅清理未使用的扩展 |
| 日志文件 | `%LOCALAPPDATA%\Microsoft\Windows\*.log`, `\Temp\*.etl` | 🟢 安全 | 可安全清理 |
| 缩略图缓存 | `%LOCALAPPDATA%\Microsoft\Windows\Explorer\thumbcache_*.db` | 🟢 安全 | 可清理 |
| 字体缓存 | `%LOCALAPPDATA%\Microsoft\Windows\*\*.fnt` | 🟡 谨慎 | 清理后重建 |
| 交付优化缓存 | `C:\ProgramData\Microsoft\DeliveryOptimization\Cache` | 🟢 安全 | Windows 更新 P2P 缓存 |

| 具体任务 | 工作量 |
|---------|-------|
| Electron 应用缓存路径收集和实现 | 1天 |
| 包管理器缓存清理 | 1天 |
| IDE/开发工具缓存 | 1天 |
| 系统日志和缩略图缓存 | 0.5天 |

### 3.7 AppData 迁移（高级功能）

完整迁移 `%LOCALAPPDATA%` 到其他盘（通过 Junction 方式）:

```cpp
// 扩展: src/core/migrator/AppDataMigrator.h
class AppDataMigrator : public MigratorBase {
    // 1. 评估 AppData 大小和组成
    // 2. 选择可安全迁移的子目录
    // 3. 迁移: 复制 → 删除原目录 → Junction
    // 4. 更新环境变量(可选)
};
```

### 3.8 快捷操作增强

| 功能 | 描述 | 工作量 |
|------|------|-------|
| 一键打开 `%TEMP%` | 按钮快速跳转到临时文件夹 | 0.5天 |
| 一键打开 `%LOCALAPPDATA%` | 跳转到本地 AppData | 0.5天 |
| 显示 AppData 大文件夹 Top 10 | 列出 AppData 中最大的 10 个目录 | 0.5天 |
| 右键集成 | 文件资源管理器右键 → "用 IceClean 分析此文件夹" | 1天 |

---

## 四、UI 导航整合

保持原有简洁风格，新增面板：

```
仪表盘         → DashboardPanel
深度清理       → DeepCleanPanel
  ├── 系统临时文件  (已有)
  ├── 软件缓存扫描  (已有 → 增强 Electron/包管理器)
  ├── AppData 深度  (新增)
  └── 系统文件管理  (新增: Windows.old/休眠/虚拟内存)
智能迁移       → MigrationPanel (已有)
  ├── 用户文件夹搬迁 (已有 → 增强 UI 引导)
  ├── 应用迁移(微信/QQ/Steam) (已有)
  ├── 下载文件夹管理 (新增)
  └── AppData 迁移    (新增)
磁盘分析       → DiskAnalyzerPanel (已有 → 增强文件类型统计)
  ├── 矩形树图    (已有)
  ├── 文件类型统计 (新增)
  └── 报告导出    (新增)
```

---

## 五、优先级评估

| 功能 | 用户价值 | 开发成本 | 实施先后 |
|------|---------|---------|---------|
| AppData 深度扫描器 | ⭐⭐⭐⭐⭐ | 中(7天) | **P0** |
| 系统文件管理(休眠/虚拟内存/Windows.old) | ⭐⭐⭐⭐⭐ | 低(2天) | **P0** |
| 文件类型统计分析 + 导出 | ⭐⭐⭐⭐ | 中(5天) | **P1** |
| 下载文件夹管理 | ⭐⭐⭐⭐ | 中(4天) | **P1** |
| 搬迁 UI 引导增强 | ⭐⭐⭐⭐ | 中(4天) | **P1** |
| 缓存分类深度清理(Electron/包管理器) | ⭐⭐⭐⭐ | 中(5天) | **P1** |
| AppData 迁移 | ⭐⭐⭐ | 高(5天) | **P2** |
| 右键集成 | ⭐⭐⭐ | 低(2天) | **P2** |

---

## 六、预计效果

| 方案 | 预计可释放 C 盘空间 |
|------|-------------------|
| AppData 临时文件清理 | 2-10 GB |
| 浏览器缓存清理 | 2-20 GB |
| Electron 应用缓存 | 2-10 GB |
| 包管理器缓存 | 1-5 GB |
| 系统文件管理(休眠/Windows.old) | 10-40 GB |
| 下载文件夹清理 | 5-50 GB |
| 用户文件夹搬迁(D: 盘) | 20-200 GB |
| **合计** | **42-335 GB** |

---

## 七、实施建议

1. **先做 P0**：AppData 深度扫描 + 系统文件管理，这两个投入小见效快
2. **安全优先**：清理 AppData 时安全等级标注必须清晰，🚫 禁止默认勾选 Caution/Dangerous 等级
3. **搬迁前自动创建还原点**：所有迁移操作执行前强制创建系统还原点
4. **渐进式实施**：按 P0 → P1 → P2 顺序推进，每完成一个功能即验证
5. **规则外部化**：AppData 可清理路径清单放入 JSON 配置文件（参考 `softdetail.json` 模式），便于社区贡献新增条目

---

> **下一步**：确认本方案中的优先级和实施范围后，按 P0/P1/P2 分阶段开发。
