# IceClean 系统托盘增强方案

**制定日期**：2026-09-04
**方案状态**：待确认后实施

---

## 一、现状分析

### 1.1 已有功能

| 功能 | 当前实现 | 文件位置 |
|------|----------|----------|
| 托盘图标创建 | `wxTaskBarIcon` + `wxArtProvider::GetIcon(wxART_INFORMATION)` | `MainWindow.cpp:376` |
| 左键单击显示窗口 | `wxEVT_TASKBAR_LEFT_DOWN` → `Show + Iconize(false) + Raise` | `MainWindow.cpp:381` |
| 关闭时最小化到托盘 | `OnClose` 检查 `IsMinimizeToTrayEnabled()` + `event.Veto()` | `MainWindow.cpp:440` |
| 设置项 | "关闭时最小化到系统托盘" 复选框（默认关闭）| `SettingsPanel.cpp:75` |
| 托盘图标文字 | "IceClean - 智能C盘清理工具" | `MainWindow.cpp:379` |

### 1.2 缺失功能（与业界对比）

| 功能 | 状态 | 说明 |
|------|------|------|
| 右键菜单 | ❌ 完全缺失 | 无托盘右键菜单 |
| 退出菜单项 | ❌ 缺失 | 无法通过托盘退出程序 |
| 显示主窗口菜单项 | ❌ 缺失 | 无显式"打开主窗口"选项 |
| 自定义托盘图标 | ❌ 缺失 | 使用系统默认 `wxART_INFORMATION` 图标 |
| 一键清理菜单项 | ❌ 缺失 | 无法通过托盘直接触发清理 |
| 气球通知 | ❌ 缺失 | 扫描/清理完成无通知 |
| 扫描/清理中图标动画 | ❌ 缺失 | 后台运行时图标无状态反馈 |
| 待处理数字徽章 | ❌ 缺失 | 无垃圾文件数量提示 |
| 开机自启动设置 | ❌ 缺失 | 无相关设置项 |
| 后台监控设置 | ❌ 缺失 | 无后台监控开关 |
| 静默清理菜单项 | ❌ 缺失 | 无后台静默清理选项 |

---

## 二、业界最佳实践

基于对 CCleaner / IObit Advanced SystemCare / Wise Care 365 / 腾讯电脑管家 / 火绒安全的调研：

### 2.1 关闭主窗口行为

- **主流默认**：**最小化到托盘**（国内 100%、国际 80% 的工具采用）
- **设置选项**：提供"最小化到托盘"和"关闭时退出"两个选项（radio button）
- **IceClean 当前**：已有复选框，默认**关闭**（与主流相反）

**建议**：改为默认**开启**，并改为 Radio Button 形式，让用户明确二选一。

### 2.2 右键菜单结构

```
IceClean
├── 打开主窗口              ← 必备
├── ───────────────         ← 分隔线
├── 一键清理                ← 常用操作
├── 静默清理                ← 后台执行，无弹窗
├── ───────────────         ← 分隔线
├── 设置                    ← 子菜单入口
│   ├── 关闭时行为
│   ├── 开机自启动
│   └── 后台监控
├── 关于 IceClean
├── ───────────────         ← 分隔线
└── 退出程序                ← 必备
```

### 2.3 左键单击行为

- **主流模式**：单击在"显示主窗口"和"隐藏主窗口"之间**切换**（推荐）
- **IceClean 当前**：仅显示窗口（不切换）

**建议**：改为切换模式：窗口可见时单击隐藏，窗口隐藏时单击显示。

### 2.4 气球通知

| 场景 | 内容 | 持续 |
|------|------|------|
| 扫描完成 | "发现 {n} 项可清理，约 {size}" | 5s |
| 清理完成 | "已清理 {size}，释放 {n} 个项目" | 5s |
| 定时任务 | "每日扫描完成，发现 {n} 项" | 5s |
| 错误 | "发生错误：{msg}" | 10s |

### 2.5 图标状态

| 状态 | 视觉反馈 | 实现方式 |
|------|----------|----------|
| 普通 | 冰蓝色图标 | 自定义 `.ico` 资源 |
| 扫描/清理中 | 旋转箭头叠加 | `wxTaskBarIcon::SetIcon` + 动画帧 |
| 待处理项 | 数字徽章 | 多尺寸图标切换 |
| 暂停监控 | 灰色图标 | `wxIcon` 灰度处理 |

---

## 三、详细实施计划

### Phase 1：基础托盘增强（P0）

#### 3.1.1 自定义托盘图标

**文件**：`src/gui/resources/`（需新增）

创建 3 个 `.ico` 文件：
- `tray_icon.ico`（16x16, 32x32）：主托盘图标，冰蓝色主题
- `tray_icon_scan.ico`：扫描中状态（带旋转箭头）
- `tray_icon_error.ico`：错误/警告状态

**实现**：
```cpp
// MainWindow.h
enum class TrayIconState { Normal, Scanning, Warning };
void SetTrayIcon(TrayIconState state);

// MainWindow.cpp
void MainWindow::SetTrayIcon(TrayIconState state) {
    // 根据状态加载不同图标
    wxIcon icon = LoadTrayIcon(state);
    if (m_taskBarIcon) {
        m_taskBarIcon->SetIcon(icon, GetTrayTooltip());
    }
}
```

#### 3.1.2 右键菜单

**文件**：`src/gui/MainWindow.cpp`

```cpp
void MainWindow::CreateTrayMenu() {
    auto* menu = new wxMenu();
    menu->Append(ID_SHOW_WINDOW, L"打开主窗口");
    menu->AppendSeparator();
    menu->Append(ID_QUICK_CLEAN, L"一键清理");
    menu->Append(ID_SILENT_CLEAN, L"静默清理");
    menu->AppendSeparator();
    // 设置子菜单
    auto* settingsMenu = new wxMenu();
    settingsMenu->AppendRadioItem(ID_CLOSE_ACTION_TRAY, L"最小化到托盘");
    settingsMenu->AppendRadioItem(ID_CLOSE_ACTION_EXIT, L"退出程序");
    settingsMenu->AppendSeparator();
    settingsMenu->AppendCheckItem(ID_STARTUP_ENABLED, L"开机自启动");
    settingsMenu->AppendCheckItem(ID_BACKGROUND_MONITOR, L"后台监控");
    menu->AppendSubMenu(settingsMenu, L"设置");
    menu->Append(ID_ABOUT, L"关于 IceClean");
    menu->AppendSeparator();
    menu->Append(ID_EXIT, L"退出程序");
    return menu;
}
```

**事件绑定**：
```cpp
m_taskBarIcon->Bind(wxEVT_TASKBAR_LEFT_DOWN, &MainWindow::OnTrayLeftClick, this);
m_taskBarIcon->Bind(wxEVT_MENU, &MainWindow::OnTrayMenuCommand, this);
```

#### 3.1.3 左键单击切换行为

```cpp
void MainWindow::OnTrayLeftClick(wxEvent&) {
    if (IsShown()) {
        Hide();
    } else {
        Show(true);
        Iconize(false);
        Raise();
    }
}
```

#### 3.1.4 托盘菜单事件处理

| 菜单ID | 处理函数 | 逻辑 |
|--------|----------|------|
| `ID_SHOW_WINDOW` | `OnTrayShowWindow` | 显示并激活窗口 |
| `ID_QUICK_CLEAN` | `OnTrayQuickClean` | 切换到扫描结果页 + 触发扫描 |
| `ID_SILENT_CLEAN` | `OnTraySilentClean` | 后台扫描+清理，无 UI 弹窗，完成发通知 |
| `ID_EXIT` | `OnTrayExit` | 保存设置 + 退出程序 |
| `ID_ABOUT` | `OnTrayAbout` | 弹出 About 对话框 |

#### 3.1.5 退出菜单项

当前 `OnClose` 没有显式退出菜单项，需新增：
```cpp
void MainWindow::OnTrayExit(wxCommandEvent&) {
    // 强制关闭（绕过最小化到托盘）
    m_forceExit = true;
    Close(true);
}
```

---

### Phase 2：设置项增强（P1）

#### 3.2.1 设置项分组调整

**文件**：`src/gui/panels/SettingsPanel.cpp`

**当前**：只有一个"关闭时最小化到系统托盘"复选框

**改进为 Radio Button 组 + 新增选项**：

```
┌─────────────────────────────────────────────┐
│ 托盘与后台                                    │
├─────────────────────────────────────────────┤
│ 关闭主窗口时：                                │
│ ○ 最小化到系统托盘（推荐）                    │
│ ○ 退出程序                                   │
│                                             │
│ □ 开机时自动启动                              │
│ □ 开机时最小化到托盘                          │
│ □ 后台监控磁盘变化                            │
│                                             │
│ 通知设置：                                    │
│ □ 扫描完成后显示通知                          │
│ □ 清理完成后显示通知                          │
│ □ 磁盘空间不足时显示警告                      │
└─────────────────────────────────────────────┘
```

#### 3.2.2 开机自启动注册

```cpp
// Utils/Win32Util.h
namespace IceClean::Utils {
    bool SetStartupEnabled(bool enabled);
    bool IsStartupEnabled();
}
```

注册表路径：
- 有管理员权限：`HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run`
- 普通用户：`HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Run`

#### 3.2.3 设置持久化

```cpp
// AppSettings.h - 新增结构
struct TraySettings {
    bool minimizeToTray = true;       // 关闭时最小化到托盘
    bool closeToExit = false;         // 关闭时退出
    bool startWithWindows = false;     // 开机自启
    bool startMinimized = false;      // 开机最小化
    bool backgroundMonitor = false;    // 后台监控
    bool notifyOnScanComplete = true;  // 扫描完成通知
    bool notifyOnCleanComplete = true; // 清理完成通知
    bool notifyLowDiskSpace = true;   // 磁盘空间不足警告
};
```

---

### Phase 3：通知增强（P1）

#### 3.3.1 扫描/清理完成通知

```cpp
// MainWindow.cpp
void MainWindow::ShowTrayNotification(const wxString& title,
                                      const wxString& message,
                                      wxBalloonIconFlags iconType = wxICON_INFORMATION) {
    if (m_taskBarIcon) {
        m_taskBarIcon->ShowBalloon(title, message, 5000, iconType);
    }
}
```

**调用时机**：
- `OnScanComplete` 中调用（若后台扫描完成）
- `OnCleanComplete` 中调用
- `ScheduledCleanManager` 定时任务完成后调用

#### 3.3.2 扫描中图标状态切换

```cpp
// MainWindow.cpp
void MainWindow::OnScanRequest(wxThreadEvent& event) {
    SetTrayIcon(TrayIconState::Scanning);  // 开始扫描，图标变化
    // ... 执行扫描
}

void MainWindow::OnScanComplete(wxThreadEvent& event) {
    SetTrayIcon(TrayIconState::Normal);
    if (m_settings.notifyOnScanComplete) {
        auto result = event.GetPayload<ScanResult>();
        ShowTrayNotification(L"扫描完成",
            wxString::Format(L"发现 %d 项可清理，约 %s",
                result.totalItems,
                FormatFileSize(result.totalSize)));
    }
}
```

---

### Phase 4：后台监控（P2）

#### 3.4.1 后台磁盘监控线程

```cpp
// Core/Monitor/DiskMonitor.h
class DiskMonitor : public std::enable_shared_from_this<DiskMonitor> {
public:
    void Start();
    void Stop();
    void SetCallback(std::function<void(const std::vector<ScanFileItem>&)> callback);

private:
    void MonitorThread();
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::function<void(const std::vector<ScanFileItem>&)> m_callback;
};
```

#### 3.4.2 定时清理任务

```cpp
// Core/Optimizer/ScheduledCleanManager.h
void ExecuteScheduledClean() {
    auto result = RunSilentScan();
    if (result.totalSize > kMinCleanSize) {
        ShowTrayNotification(L"定时清理完成",
            wxString::Format(L"清理了 %s", FormatFileSize(result.totalSize)));
    }
}
```

---

## 四、实施优先级与工作量

| 优先级 | 功能 | 工作量 | 风险 |
|--------|------|--------|------|
| **P0** | 右键菜单（显示/退出/关于） | 小 | 低 |
| **P0** | 左键单击切换窗口 | 小 | 低 |
| **P0** | 自定义托盘图标 | 中 | 低 |
| **P0** | 设置项改为 Radio Button | 小 | 低 |
| **P1** | 一键清理菜单项 | 中 | 中 |
| **P1** | 扫描/清理完成气球通知 | 中 | 中 |
| **P1** | 开机自启动设置 | 小 | 中 |
| **P1** | 静默清理菜单项 | 中 | 中 |
| **P1** | 扫描中图标动画 | 中 | 中 |
| **P2** | 后台监控设置 | 大 | 中 |
| **P2** | 定时清理任务 | 大 | 中 |
| **P3** | 待处理数字徽章 | 中 | 高 |

---

## 五、关键文件变更清单

### 新增文件
| 文件 | 说明 |
|------|------|
| `src/gui/resources/tray_icon.ico` | 托盘主图标 |
| `src/gui/resources/tray_icon_scan.ico` | 扫描中图标 |
| `src/gui/resources/tray_icon_error.ico` | 错误状态图标 |
| `src/core/monitor/DiskMonitor.h` | 后台磁盘监控器 |
| `src/core/monitor/DiskMonitor.cpp` | 后台磁盘监控器实现 |
| `src/models/AppSettings.h` | 应用程序设置模型 |

### 修改文件
| 文件 | 变更内容 |
|------|----------|
| `src/gui/MainWindow.h` | 新增托盘相关成员和方法 |
| `src/gui/MainWindow.cpp` | 右键菜单、事件处理、图标状态 |
| `src/gui/panels/SettingsPanel.cpp` | 托盘设置 UI 分组 |
| `src/utils/Win32Util.h/cpp` | 开机自启动注册 |
| `src/core/optimizer/ScheduledCleanManager.cpp` | 完成通知 |

---

## 六、确认事项

请确认以下决策点：

1. **关闭行为**：默认"最小化到托盘"还是"退出程序"？
2. **开机自启动**：默认开启还是关闭？（建议默认关闭，避免被视为流氓软件）
3. **后台监控**：是否纳入第一期实施范围？（涉及多线程，需更多测试）
4. **静默清理**：是否需要实现"无弹窗后台清理"功能？
5. **托盘图标**：是否需要自定义冰蓝色图标？（可使用简单的蓝色圆形图标）

---

*方案制定：Claude Code*
*日期：2026-09-04*
