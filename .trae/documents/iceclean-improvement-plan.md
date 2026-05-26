# IceClean 提升计划

## 一、当前问题修复（紧急）

### 问题1：停止扫描无效，一直显示"正在停止"

**根因**：`ScannerBase::ScanDirectory()` 是纯同步阻塞的递归遍历，`m_stopRequested` 只在线程启动前检查一次，`scanner->Scan()` 执行期间完全无法响应停止请求。`thread.join()` 会一直等待所有扫描器线程自然结束。

**修复方案**：在 `ScannerBase::ScanDirectory` 的循环中添加停止检查点

- **文件**: `src/core/scanner/ScannerBase.h` / `ScannerBase.cpp`
  - `ScanDirectory` 增加 `const std::atomic<bool>& stopFlag` 参数
  - 在 `do-while` 循环每次迭代开头检查 `stopFlag`，若为 true 则 break
  - 在递归调用前检查 `stopFlag`，若为 true 则 return
  - 每处理 50 个文件后检查一次（避免频繁 atomic 读取影响性能）

- **文件**: `src/core/scanner/IScanner.h`
  - `Scan()` 方法增加 `const std::atomic<bool>* stopFlag = nullptr` 参数

- **文件**: 所有具体扫描器（11个）
  - `Scan()` 实现中传递 `stopFlag` 给 `ScanDirectory`
  - 不使用 `ScanDirectory` 的扫描器（如 `WinSxSScanner`、`HibernationScanner`）在关键点检查 `stopFlag`

- **文件**: `src/core/scanner/ScannerAggregator.cpp`
  - `ScanAll` 中将 `m_stopRequested` 传递给各扫描器的 `Scan()` 调用

### 问题2：扫描进度不够细粒度，看起来像僵死

**根因**：进度事件只在每个扫描器开始/完成时发送（11个扫描器最多22个事件），某个扫描器耗时较长时无任何中间更新。

**修复方案**：在 `ScanDirectory` 中添加文件计数进度回调

- **文件**: `src/core/scanner/ScannerBase.h` / `ScannerBase.cpp`
  - `ScanDirectory` 增加 `std::function<void(int fileCount)>` 进度回调参数
  - 每扫描 100 个文件后调用一次回调

- **文件**: `src/core/scanner/ScannerAggregator.cpp`
  - 各扫描器线程中，通过进度回调发送 `wxEVT_SCAN_PROGRESS_UPDATE` 事件
  - 进度事件中增加 `filesScanned` 字段（可选，或直接在 `currentScanner` 中附带文件数）

- **文件**: `src/gui/panels/DashboardPanel.cpp`
  - `UpdateScanProgress` 中显示已扫描文件数，如 "正在扫描: 浏览器缓存 (1,234 个文件)"

---

## 二、功能差距补齐（切实有效，不贪多）

### 优先级 P0：补齐半成品功能

#### 2.1 隐私清理后端实现

**现状**：DeepCleanPanel 隐私清理标签页有 UI（Cookies/历史记录/表单数据勾选框），但 `StartDeepClean` 中未处理这些 ID，后端完全缺失。

**实现方案**：

- **新增文件**: `src/core/cleaner/PrivacyCleaner.h` / `PrivacyCleaner.cpp`
  - 清理浏览器 Cookies：删除 Chrome/Edge/Firefox 的 Cookies 数据库文件
  - 清理浏览器历史记录：删除 History 数据库文件
  - 清理表单自动填充：删除 Web Data / formhistory.sqlite
  - 清理前检测浏览器是否运行，运行中则跳过（避免数据损坏）

- **修改文件**: `src/gui/MainWindow.cpp` → `StartDeepClean`
  - 处理 `cookies`/`history`/`formData` 等 ID，调用 `PrivacyCleaner`

#### 2.2 注册表清理实现

**现状**：DeepCleanPanel 注册表清理标签页完全是占位文本。

**实现方案**：

- **新增文件**: `src/core/cleaner/RegistryCleaner.h` / `RegistryCleaner.cpp`
  - 扫描无效的卸载信息（HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall 中指向不存在路径的项）
  - 扫描无效的启动项（指向不存在路径的 Run 键值）
  - 扫描无效的文件关联（指向不存在程序的扩展名关联）
  - 扫描残留的 COM 组件注册信息
  - 清理前自动备份注册表（导出 .reg 文件到操作日志目录）
  - 安全等级：Caution（需用户确认）

- **修改文件**: `src/gui/panels/DeepCleanPanel.cpp`
  - 注册表清理标签页从占位改为实际扫描结果列表

### 优先级 P1：高价值新功能

#### 2.3 更多浏览器支持

**现状**：仅 Chrome/Edge/Firefox。

**实现方案**：

- **修改文件**: `src/core/scanner/BrowserCacheScanner.cpp`
  - 添加 Opera（`%APPDATA%\Opera Software\Opera Stable`）
  - 添加 Brave（`%LOCALAPPDATA%\BraveSoftware\Brave-Browser\User Data`）
  - 添加 Vivaldi（`%LOCALAPPDATA%\Vivaldi\User Data`）
  - 添加 360安全浏览器（`%APPDATA%\360se6`）
  - 添加 QQ浏览器（`%APPDATA%\Tencent\QQBrowser`）
  - 这些浏览器大多基于 Chromium，缓存路径结构与 Chrome 相同

#### 2.4 清理进度实时反馈

**现状**：`StartClean` 的进度回调为空实现，用户点击清理后无任何反馈。

**实现方案**：

- **修改文件**: `src/core/cleaner/FileCleaner.h` / `FileCleaner.cpp`
  - `Clean` 方法中每删除一个文件后调用进度回调
  - 回调中包含：当前文件路径、已完成数/总数、已释放大小

- **修改文件**: `src/gui/MainWindow.cpp`
  - `StartClean` 中接收进度回调，发送 `wxEVT_CLEAN_PROGRESS` 事件
  - 在 DashboardPanel 或 ScanResultPanel 中显示清理进度

- **新增**: 清理进度对话框（类似 `MigrationProgressDlg`）
  - 显示当前正在清理的文件、进度条、已释放大小、取消按钮

---

## 三、实施步骤

### 第一批：修复停止扫描 + 扫描进度（最紧急）

1. `IScanner.h` - Scan() 增加 stopFlag 参数
2. `ScannerBase.h/cpp` - ScanDirectory 增加 stopFlag + 进度回调
3. 所有扫描器 - 传递 stopFlag
4. `ScannerAggregator.cpp` - 传递 m_stopRequested，增加细粒度进度
5. `DashboardPanel.cpp` - 进度显示增加文件数

### 第二批：隐私清理 + 注册表清理

6. 新增 `PrivacyCleaner` - 浏览器隐私数据清理
7. `MainWindow.cpp` - StartDeepClean 处理隐私清理 ID
8. 新增 `RegistryCleaner` - 注册表无效项扫描和清理
9. `DeepCleanPanel.cpp` - 注册表标签页接入实际功能

### 第三批：更多浏览器 + 清理进度

10. `BrowserCacheScanner.cpp` - 添加 5 种浏览器支持
11. `FileCleaner.cpp` - 添加清理进度回调
12. 新增清理进度对话框
13. `MainWindow.cpp` - 清理进度事件处理

---

## 四、验证步骤

1. 点击"一键扫描"→ 点击"停止扫描"→ 应在 1-2 秒内停止，不再卡在"正在停止"
2. 扫描过程中 → CircularProgress 百分比持续变化，磁盘信息区域显示"正在扫描: XXX (N 个文件)"
3. 深度清理 → 隐私清理标签页勾选 Cookies/历史记录 → 点击清理 → 实际清理浏览器数据
4. 深度清理 → 注册表清理标签页 → 显示扫描到的无效注册表项 → 勾选清理
5. 扫描浏览器缓存 → Opera/Brave/360浏览器等缓存被扫描到
6. 点击清理 → 弹出清理进度对话框，显示实时进度，可取消
