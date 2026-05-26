# IceClean 提升计划 - 开发工具缓存清理 + 扫描进度优化

## 一、扫描进度优化（紧急）

### 问题1：扫描进度不够细，用户看不到在扫描什么

**现状**：进度回调每100个文件才触发一次，且只传递文件数量，不传递当前文件路径。DashboardPanel 只显示"正在扫描: 浏览器缓存 (234 个文件)"，看不到具体在扫描哪个文件。

**修复方案**：

1. **修改 `ScanProgressCallback` 类型**（`IScanner.h`）
   - 从 `std::function<void(int filesScanned)>` 改为 `std::function<void(int filesScanned, const std::wstring& currentFile)>`
   - 每次回调都传递当前正在扫描的文件路径

2. **修改 `ScannerBase::ScanDirectory`**（`ScannerBase.cpp`）
   - 将进度回调频率从每100个文件改为每10个文件（更频繁）
   - 每次回调传递当前文件路径
   - 在 `ScannerAggregator` 的 lambda 中加时间节流（100ms），避免事件队列积压

3. **修改 `ScanProgressInfo` 结构**（`ScannerAggregator.h`）
   - 添加 `std::wstring currentFile` 字段

4. **修改 `DashboardPanel::UpdateScanProgress`**（`DashboardPanel.cpp`）
   - 显示当前正在扫描的文件名（截取最后部分），如"正在扫描: 浏览器缓存 → Cache\abc.dat"

### 问题2：停止扫描仍然不能停

**现状**：`ShouldStop` 使用 `memory_order_relaxed`，且检查间隔为50次迭代。对于文件少的目录可能无法及时响应。

**修复方案**：

1. **修改 `ShouldStop`**（`ScannerBase.cpp`）
   - 改为 `memory_order_acquire` 确保停止标志立即可见

2. **修改 `ScanDirectory`**（`ScannerBase.cpp`）
   - 将停止检查从每50次迭代改为**每次迭代都检查**
   - `atomic load` 开销极小（~1ns），不影响性能

3. **修改 `ScannerAggregator::ScanAll`**（`ScannerAggregator.cpp`）
   - 进度回调 lambda 中加时间节流，记录上次发送事件时间，间隔<100ms则跳过
   - 这样即使每10个文件触发回调，也不会产生过多 wxQueueEvent

---

## 二、开发工具缓存扫描器

### 新增文件

1. **`src/core/scanner/DevCacheScanner.h`**
2. **`src/core/scanner/DevCacheScanner.cpp`**

### DevCacheScanner 设计

- 名称：L"开发工具缓存"
- 描述：L"扫描npm、pip、conda、Maven等开发工具的缓存和包文件"
- 安全等级：Caution（删除后需重新下载，但不影响系统）
- 图标：L"dev_cache"

### 扫描的缓存路径

| 类型 | 路径 | 安全等级 |
|------|------|----------|
| npm cache | `%LOCALAPPDATA%\npm-cache` | Safe |
| .npm | `%USERPROFILE%\.npm` | Safe |
| .node-gyp | `%USERPROFILE%\.node-gyp` | Safe |
| yarn cache | `%LOCALAPPDATA%\Yarn\Cache` | Safe |
| pnpm store | `%LOCALAPPDATA%\pnpm-store` | Safe |
| pip cache | `%LOCALAPPDATA%\pip\Cache` | Safe |
| conda pkgs | `%USERPROFILE%\miniconda3\pkgs` 或 `%USERPROFILE%\anaconda3\pkgs` | Caution |
| Maven .m2 | `%USERPROFILE%\.m2\repository` | Caution |
| Gradle caches | `%USERPROFILE%\.gradle\caches` | Safe |
| Gradle wrapper | `%USERPROFILE%\.gradle\wrapper\dists` | Safe |
| Go mod cache | `%USERPROFILE%\go\pkg\mod` | Safe |
| NuGet packages | `%USERPROFILE%\.nuget\packages` | Caution |
| Cargo registry | `%USERPROFILE%\.cargo\registry` | Safe |
| electron cache | `%LOCALAPPDATA%\electron\Cache` | Safe |

### 实现方式

- 每种缓存类型一个私有方法（`ScanNpmCache`, `ScanPipCache` 等）
- 使用 `ScanDirectory` 递归扫描，传递 `stopFlag` 和 `progressCb`
- `IsAvailable()` 检查至少一种缓存路径存在
- Docker 缓存需检测进程是否运行

### 修改文件

1. **`src/core/scanner/ScannerAggregator.cpp`** — `RegisterBuiltinScanners()` 中添加 `std::make_unique<DevCacheScanner>()`

---

## 三、开发工具缓存迁移器

### 新增文件

1. **`src/core/migrator/DevCacheMigrator.h`**
2. **`src/core/migrator/DevCacheMigrator.cpp`**

### DevCacheMigrator 设计

参照 `WeChatMigrator` 模式：
- `Detect()` 检测上述缓存目录，生成 MigrationItem 列表
- `Migrate()` 使用 `MoveAndCreateJunction()` 将缓存目录迁移到目标盘
- 迁移后创建 NTFS 联接链接，工具仍能正常访问缓存
- Docker 迁移前检测进程是否运行

### 修改文件

1. **`src/models/MigrationItem.h`** — `MigrationType` 枚举添加 `DevCache`
2. **`src/gui/MainWindow.cpp`** — `StartMigrate` 中处理 `DevCache` 类型

---

## 四、实施步骤

### 步骤1：修复扫描进度显示
1. `IScanner.h` - 修改 ScanProgressCallback 签名，增加 currentFile 参数
2. `ScannerBase.cpp` - 每次迭代检查 stopFlag，每10个文件回调带文件路径
3. `ScannerAggregator.h` - ScanProgressInfo 增加 currentFile 字段
4. `ScannerAggregator.cpp` - 进度回调加100ms时间节流
5. `DashboardPanel.cpp` - 显示当前扫描文件名

### 步骤2：修复停止扫描
6. `ScannerBase.cpp` - ShouldStop 改用 memory_order_acquire
7. `ScannerBase.cpp` - 每次迭代都检查 stopFlag

### 步骤3：新增 DevCacheScanner
8. 创建 `DevCacheScanner.h` / `DevCacheScanner.cpp`
9. `ScannerAggregator.cpp` - 注册新扫描器

### 步骤4：新增 DevCacheMigrator
10. `MigrationItem.h` - 添加 DevCache 枚举
11. 创建 `DevCacheMigrator.h` / `DevCacheMigrator.cpp`
12. `MainWindow.cpp` - 处理 DevCache 迁移类型

### 步骤5：编译验证并推送
13. 编译验证
14. 推送到 GitHub

---

## 五、验证步骤

1. 点击"一键扫描" → DashboardPanel 显示"正在扫描: XXX → 具体文件名"
2. 扫描过程中 → 进度百分比平滑变化，文件名实时更新
3. 点击"停止扫描" → 1秒内停止，不再卡在"正在停止..."
4. 扫描结果中 → 出现"开发工具缓存"分类，显示 npm/pip/Maven 等缓存大小
5. 勾选开发工具缓存 → 点击清理 → 成功删除缓存文件
6. 迁移面板 → 检测到 npm cache/pip cache 等可迁移项 → 迁移到D盘 → Junction链接正常
