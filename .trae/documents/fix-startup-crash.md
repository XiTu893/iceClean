# 修复程序启动白屏崩溃问题

## 问题现象
程序启动后白屏，然后异常退出。`assert_failures.log` 记录了 `CRT_ASSERT: "Buffer too small"` 错误。

## 根因分析

### 主要原因：resource.h 中 APP_DESCRIPTION_A 的 UTF-8 中文字符串

`resource.h` 第20行：
```c
#define APP_DESCRIPTION_A  "\xE6\x99\xBA\xE8\x83\xBDC\xE7\x9B\x98\xE6\xB8\x85\xE7\x90\x86\xE4\xB8\x8E\xE8\xBF\x81\xE7\xA7\xBB\xE5\xB7\xA5\xE5\x85\xB7"
```

这是"智能C盘清理与迁移工具"的 UTF-8 编码。Windows 资源编译器在处理 `VS_VERSION_INFO` 的 `StringFileInfo` 块（代码页 0x0409 = 英语）时，ANSI 字符串不支持 UTF-8 多字节序列。资源编译器在解析这些字节时，可能导致内部缓冲区溢出，触发 CRT 的 "Buffer too small" 断言。

### 次要风险点（不一定是当前崩溃原因，但应一并修复）

1. **StartupPanel 构造期间 COM 初始化/反初始化**：`LoadScheduledTasks()` 在构造函数中调用 `ScheduledTaskOptimizer`（构造时 CoInitializeEx MTA，析构时 CoUninitialize），可能与 wxWidgets 的 STA 初始化冲突
2. **DeepCleanPanel 注册表列表的 wxImageList**：`AssignImageList` 后在 `wxEVT_LEFT_DOWN` 中通过 `HitTest` 检测点击，但 `HitTest` 返回的行索引可能与 `m_registryChecked` 不同步
3. **detach 线程的 use-after-free 风险**：多处 `std::thread.detach()` 捕获了 `this`

## 修复方案

### 1. 修复 resource.h 中的 APP_DESCRIPTION_A（核心修复）

将 `APP_DESCRIPTION_A` 改为纯 ASCII 英文描述，中文描述仅保留在宽字符版本 `APP_DESCRIPTION` 中：

```c
#define APP_DESCRIPTION                 L"智能C盘清理与迁移工具"
#define APP_DESCRIPTION_A               "IceClean - Smart C-Drive Cleaner"
```

Windows 版本信息的 ANSI 字符串块只支持当前代码页的字符，UTF-8 多字节序列在 0x0409（英语）代码页下不合法。

### 2. StartupPanel 延迟加载计划任务

将 `LoadScheduledTasks()` 从构造函数中移除，改为在标签页首次切换到时加载（与进程管理一致），避免构造期间的 COM 操作：

- 在 `OnNotebookPageChanged` 中，当切换到计划任务标签页时才调用 `LoadScheduledTasks()`
- 添加 `m_tasksLoaded` 标志防止重复加载

### 3. DeepCleanPanel 注册表列表点击事件优化

将 `wxEVT_LEFT_DOWN` + `HitTest` 改为更可靠的方式：使用 `wxEVT_LIST_ITEM_ACTIVATED`（双击），同时在列表下方添加提示"双击行切换勾选"。或者保留 `wxEVT_LEFT_DOWN` 但增加边界检查。

## 修改文件清单

| 文件 | 修改内容 |
|------|----------|
| `src/gui/resources/resource.h` | `APP_DESCRIPTION_A` 改为英文，`APP_COMPANY_A` 保持 "XiTu" |
| `src/gui/panels/StartupPanel.cpp` | 移除构造函数中的 `LoadScheduledTasks()`，改为 `OnNotebookPageChanged` 中延迟加载 |

## 验证步骤

1. 编译 Debug 版本
2. 删除旧的 `assert_failures.log` 和 `IceClean.log`
3. 运行 `IceClean.exe`，确认窗口正常显示
4. 检查 `assert_failures.log` 是否为空
5. 逐一测试各导航页面是否正常
6. 测试注册表清理的复选框点击
7. 测试计划任务标签页的延迟加载
