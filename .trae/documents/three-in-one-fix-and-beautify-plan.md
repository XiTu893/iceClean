# 三合一修复与美化计划（升级版）

## 问题概述

1. **弹窗拦截崩溃**：点击拦截后程序自动退出
2. **注册表清理无效**：清理不掉任何项
3. **UI美化**：需要渐变背景、商业级视觉品质

---

## 一、弹窗拦截崩溃修复

### 根因

`PopupBlocker::BlockPopup` 对 360/腾讯管家等安全软件进程调用 `TerminateProcess`，触发安全软件自我保护反杀 IceClean。次要原因：`std::thread::detach()` 捕获 `this` 指针，窗口关闭后回调访问已销毁对象。

### 修复内容

#### 1.1 PopupBlocker.cpp — 禁止终止安全软件进程

- **移除 `TerminateProcess` 逻辑**（第313-333行）：对 AdwarePopup 类型不再终止进程，仅通过注册表/计划任务方式拦截
- **扩展 `IsSystemPopup` 保护列表**：加入 360、腾讯管家、金山、火绒等安全软件进程名
- **`CreateProcessW` 缓冲区修复**（第266-281行、第367-381行）：将 `const_cast<LPWSTR>(cmd.c_str())` 改为 `std::vector<wchar_t>` 可变缓冲区

#### 1.2 SecurityPanel.cpp — 线程安全

- 将 `std::thread::detach()` 替换为保存 `std::thread` 成员变量
- 析构时等待线程结束（`join()`）
- `CallAfter` 回调中增加 `IsBeingDeleted()` 检查

#### 1.3 PopupBlocker.cpp — 配置文件并发保护

- `IncrementBlockCount` 添加 `std::mutex` 保护

#### 1.4 FileWatcher.cpp — 线程安全修复

- `AddChangeRecord` 添加 `std::mutex` 保护
- `WatchThread` 修复 stopEvent 索引传递

#### 1.5 SecurityPanel.cpp — 修复空循环体

- `OnBlockAll` 的 CallAfter 回调中正确更新 `m_popupItems` 的 `isBlocked` 状态

---

## 二、注册表清理无效修复

### 根因

`RegDeleteKeyW` 无法删除含有子键的注册表键；64位系统上未指定 `KEY_WOW64_64KEY` 标志。

### 修复内容

#### 2.1 RegistryCleaner.cpp — 使用 RegDeleteTreeW

- 第78行和第792行：`RegDeleteKeyW` → `RegDeleteTreeW`

#### 2.2 RegistryUtil.cpp — 64位注册表访问

- 所有 `RegOpenKeyExW` 调用添加 `KEY_WOW64_64KEY` 标志

#### 2.3 DeepCleanPanel.cpp — 结果反馈

- 清理完成提示显示成功数和失败数

---

## 三、UI 整体美化（对标腾讯电脑管家/360安全卫士）

### 设计参考

对标产品：**腾讯电脑管家18版**（深色侧边栏+浅色内容区+品牌蓝渐变）、**360安全卫士极速版**（科技蓝主题+卡片式布局+渐变按钮）、**Ashampoo WinOptimizer**（深色主题+渐变进度条+圆角卡片）

核心设计语言：
- **深蓝渐变侧边栏** — 从深蓝 `#0D1B3E` 到深蓝灰 `#162447`，营造科技感和专业感
- **品牌蓝渐变按钮** — 从 `#165DFF` 到 `#3576FF`，悬停时更亮
- **卡片微渐变** — 从纯白到极浅灰，增加层次感
- **渐变进度弧** — 品牌蓝到亮蓝渐变，末端发光效果
- **图标适配** — 浅色主题用深色图标，深色主题用亮色图标

### 修改内容

#### 3.1 ThemeManager.h — 新增渐变色字段

```cpp
// 新增字段
wxColour sidebarGradientEnd;     // 侧边栏渐变终止色
wxColour sidebarSelectedText;    // 选中项文字色
wxColour accentGradientEnd;      // 品牌色渐变终止色
wxColour cardGradientStart;      // 卡片渐变起始色
wxColour cardGradientEnd;        // 卡片渐变终止色
```

#### 3.2 ThemeManager.cpp — 重定义颜色方案

**浅色主题**（对标腾讯电脑管家18浅色版 + 360科技蓝）：

| 字段 | 新值 | 说明 |
|------|------|------|
| background | `#F2F4F8` | 内容区浅灰蓝背景 |
| surface | `#FFFFFF` | 纯白卡片 |
| surfaceHover | `#F0F2F6` | 悬停微灰 |
| textPrimary | `#1D2129` | 深色文字 |
| textSecondary | `#4E5969` | 中灰辅助文字 |
| textDisabled | `#C9CDD4` | 禁用文字 |
| accent | `#165DFF` | 品牌蓝 |
| accentHover | `#3576FF` | 悬停蓝 |
| accentGradientEnd | `#4080FF` | 品牌蓝渐变终止 |
| danger | `#F53F3F` | 明亮红 |
| warning | `#FF9C00` | 橙色警告 |
| success | `#00B42A` | 鲜明绿 |
| **sidebar** | **#0D1B3E** | **深蓝侧边栏（科技感）** |
| **sidebarGradientEnd** | **#162447** | **侧边栏渐变终止** |
| sidebarText | `#C9D1D9` | 亮灰文字 |
| sidebarSelected | `#165DFF` | 品牌蓝选中 |
| sidebarSelectedText | `#FFFFFF` | 白色选中文字 |
| sidebarHover | `#1A2E5A` | 悬停深蓝 |
| border | `#E5E6EB` | 柔和边框 |
| divider | `#E5E6EB` | 分隔线 |
| cardShadow | `rgba(0,0,0,0.08)` | 卡片阴影 |
| cardGradientStart | `#FFFFFF` | 卡片渐变起始 |
| cardGradientEnd | `#FAFBFD` | 卡片渐变终止 |
| progressBar | `#165DFF` | 进度条蓝 |
| progressBarBg | `#E5E6EB` | 进度条背景 |

**深色主题**（对标腾讯电脑管家18深色版）：

| 字段 | 新值 | 说明 |
|------|------|------|
| background | `#17171A` | 深黑背景 |
| surface | `#232328` | 深灰卡片 |
| surfaceHover | `#2E2E35` | 悬停深灰 |
| textPrimary | `#F2F3F5` | 亮白文字 |
| textSecondary | `#A9AAB8` | 灰色辅助 |
| textDisabled | `#5C5D6E` | 禁用文字 |
| accent | `#3D7FFF` | 深色模式蓝 |
| accentHover | `#5B94FF` | 悬停蓝 |
| accentGradientEnd | `#6BA4FF` | 渐变终止 |
| danger | `#F76965` | 明亮红 |
| warning | `#FFB732` | 橙色警告 |
| success | `#34D190` | 鲜明绿 |
| **sidebar** | **#0A0F1E** | **极深蓝侧边栏** |
| **sidebarGradientEnd** | **#111830** | **渐变终止** |
| sidebarText | `#B0B8C8` | 亮灰文字 |
| sidebarSelected | `#3D7FFF` | 品牌蓝选中 |
| sidebarSelectedText | `#FFFFFF` | 白色 |
| sidebarHover | `#151D38` | 悬停深蓝 |
| border | `#3B3B44` | 深色边框 |
| divider | `#3B3B44` | 分隔线 |
| cardShadow | `rgba(0,0,0,0.25)` | 深阴影 |
| cardGradientStart | `#232328` | 卡片渐变起始 |
| cardGradientEnd | `#28282E` | 卡片渐变终止 |
| progressBar | `#3D7FFF` | 进度条蓝 |
| progressBarBg | `#3B3B44` | 进度条背景 |

#### 3.3 NavSidebar.cpp — 深蓝渐变侧边栏

**背景渐变**：使用 `gc->CreateLinearGradientBrush` 从 sidebar → sidebarGradientEnd 垂直渐变

**标题 "IceClean"**：
- 浅色主题：白色文字 + 品牌蓝副标题
- 深色主题：白色文字
- 字号增大到 18 号粗体

**图标颜色**：根据主题选择适配色组

浅色主题图标色（亮色，与深蓝背景协调）：
```
首页:     #5B9BF5 (亮蓝)
深度清理: #4DD8A4 (亮绿)
智能迁移: #FF9248 (亮橙)
加速优化: #FFD04A (亮黄)
软件管理: #FF7070 (亮红)
软件推荐: #A78BFA (亮紫)
安全防护: #FF7A9A (亮玫红)
网络优化: #5CE0A0 (亮翠绿)
设置:     #9CA3AF (亮灰)
关于:     #6BA3F7 (亮浅蓝)
```

深色主题图标色（与浅色主题相同，因为侧边栏也是深色）：
```
同上
```

选中项图标色：白色 `#FFFFFF`

**选中项背景渐变**：accent → accentGradientEnd 水平渐变，圆角8px

**悬停背景**：sidebarHover 半透明圆角矩形

**选中项文字**：sidebarSelectedText（白色）

**主题变更回调**：更新所有颜色字段 + Refresh

#### 3.4 CardPanel.cpp — 渐变卡片 + 双层阴影

- 卡片背景使用 `cardGradientStart → cardGradientEnd` 微渐变
- 悬停时渐变更明显（surface → surfaceHover）
- 阴影使用双层：
  - 外层：偏移(2,4)，alpha 12，模拟环境光
  - 内层：偏移(1,2)，alpha 8，模拟接触阴影
- 悬停时阴影加深 + 边框变为 accent 色

#### 3.5 CircularProgress.cpp — 渐变进度弧 + 发光效果

- 进度弧使用 `accent → accentGradientEnd` 渐变
- 轨道使用 progressBarBg 渐变
- 末端添加发光效果：小圆点 + 半透明光晕（alpha 60 的 accent 色圆）

#### 3.6 其他面板硬编码颜色修复

| 文件 | 修改 |
|------|------|
| SecurityPanel.cpp | 删除6个静态颜色常量，全部改用 ThemeManager |
| MigrationPanel.cpp | 替换硬编码颜色 |
| DiskAnalyzerPanel.cpp | 替换硬编码颜色 |
| NetworkPanel.cpp | 替换硬编码颜色 |
| DuplicateFilePanel.cpp | 替换硬编码颜色 |
| DriverPanel.cpp | 替换硬编码颜色 |
| ScanResultPanel.cpp | `*wxWHITE` → ThemeManager background |
| StartupPanel.cpp | `*wxWHITE_BRUSH` → ThemeManager surface |
| MainWindow.cpp | 分隔线硬编码 → ThemeManager divider |
| AboutPanel.cpp | 替换硬编码颜色 |
| MigrationProgressDlg.cpp | 替换硬编码颜色 |
| CleanProgressDialog.cpp | 替换硬编码颜色 |

---

## 修改文件完整清单

| # | 文件 | 修改类型 | 优先级 |
|---|------|---------|--------|
| 1 | `src/core/safety/PopupBlocker.cpp` | Bug修复 | P0 |
| 2 | `src/gui/panels/SecurityPanel.cpp` | Bug修复+美化 | P0 |
| 3 | `src/core/safety/FileWatcher.cpp` | Bug修复 | P0 |
| 4 | `src/core/cleaner/RegistryCleaner.cpp` | Bug修复 | P0 |
| 5 | `src/utils/RegistryUtil.cpp` | Bug修复 | P0 |
| 6 | `src/gui/panels/DeepCleanPanel.cpp` | Bug修复 | P1 |
| 7 | `src/gui/controls/ThemeManager.h` | 美化：新增渐变色字段 | P1 |
| 8 | `src/gui/controls/ThemeManager.cpp` | 美化：重定义颜色方案 | P1 |
| 9 | `src/gui/controls/NavSidebar.cpp` | 美化：渐变侧边栏 | P1 |
| 10 | `src/gui/controls/CardPanel.cpp` | 美化：渐变卡片 | P1 |
| 11 | `src/gui/controls/CircularProgress.cpp` | 美化：渐变进度弧 | P1 |
| 12 | `src/gui/panels/ScanResultPanel.cpp` | 美化 | P2 |
| 13 | `src/gui/panels/MigrationPanel.cpp` | 美化 | P2 |
| 14 | `src/gui/panels/DiskAnalyzerPanel.cpp` | 美化 | P2 |
| 15 | `src/gui/panels/NetworkPanel.cpp` | 美化 | P2 |
| 16 | `src/gui/panels/DuplicateFilePanel.cpp` | 美化 | P2 |
| 17 | `src/gui/panels/DriverPanel.cpp` | 美化 | P2 |
| 18 | `src/gui/panels/AboutPanel.cpp` | 美化 | P2 |
| 19 | `src/gui/panels/StartupPanel.cpp` | 美化 | P2 |
| 20 | `src/gui/MainWindow.cpp` | 美化 | P2 |
| 21 | `src/gui/dialogs/MigrationProgressDlg.cpp` | 美化 | P2 |
| 22 | `src/gui/dialogs/CleanProgressDialog.cpp` | 美化 | P2 |

## 验证步骤

1. 编译通过
2. 弹窗拦截：点击拦截后程序不再自动退出
3. 注册表清理：能成功删除含子键的注册表项
4. 浅色主题：深蓝渐变侧边栏，科技感强，文字清晰
5. 深色主题：极深蓝侧边栏，对比度好
6. 侧边栏有渐变效果，选中项品牌蓝渐变背景
7. 卡片有微渐变和双层阴影
8. 进度条有渐变弧线+发光效果
9. 所有面板深色主题正确响应
10. 无硬编码颜色残留
