# UI 美工优化计划 — 侧边栏可读性与配色升级

## 问题诊断

用户反馈"左边黑乎乎看不清楚"，核心原因：

1. **浅色主题侧边栏背景极深**：`#161C28`（近黑色），与内容区 `#F5F7FA` 反差过大，侧边栏像"黑洞"
2. **悬停状态几乎不可见**：`sidebarHover #232A3A` 与背景 `#161C28` 差异仅 13-14 RGB 值
3. **文字朦胧**：`sidebarText #C2C7D1` 灰蓝色在深色背景上缺乏锐利感
4. **图标颜色不适配深色背景**：10 个硬编码图标颜色为浅色背景设计，在深色背景上刺眼且不协调
5. **标题 "IceClean" 硬编码蓝色**：不跟随主题切换
6. **选中项文字纯白**：`m_selectedTextColor = *wxWHITE` 硬编码
7. **大量硬编码颜色**：NavSidebar 中图标色、选中色全部硬编码，不响应主题变更

## 商业软件配色参考

| 软件 | 浅色主题侧边栏 | 设计理念 |
|------|---------------|---------|
| VS Code Light Modern | `#F5F5F5` 浅灰 | 侧边栏与内容区同色系，仅略深 |
| Notion | `#FFFFFF` 纯白 | 无传统侧边栏，用缩进区分层级 |
| Tokyo Night Light | `#D6D8DF` 浅灰蓝 | 侧边栏比内容区略暗 |
| 360安全卫士 | 白色/浅灰 + 品牌色图标 | 浅色侧边栏 + 彩色图标 |
| 火绒安全 | 浅灰白 + 橙色品牌色 | 整体浅色调 |

**核心结论**：现代商业软件浅色主题几乎不使用深色侧边栏。采用"浅色侧边栏 + 品牌色点缀"的设计更符合用户预期。

## 修改方案

### 方案：浅色侧边栏 + 品牌色点缀（推荐）

浅色主题下侧边栏改为浅色系，深色主题下保持深色但提升对比度。图标使用品牌色，选中项用品牌蓝背景+白字。

---

### 1. ThemeManager.cpp — 重定义侧边栏颜色

**浅色主题**：
| 字段 | 当前值 | 新值 | 说明 |
|------|--------|------|------|
| sidebar | `#161C28` 近黑 | `#F0F1F5` 浅灰蓝 | 与内容区同色系，略深 |
| sidebarText | `#C2C7D1` 灰蓝 | `#3D4551` 深灰 | 深色文字确保可读性 |
| sidebarSelected | `#165DFF` 品牌蓝 | `#165DFF` 品牌蓝 | 保持不变 |
| sidebarHover | `#232A3A` 深灰 | `#E8E9EE` 浅灰蓝 | 明显的悬停反馈 |

**深色主题**：
| 字段 | 当前值 | 新值 | 说明 |
|------|--------|------|------|
| sidebar | `#101014` 极深 | `#1E1E24` 深灰 | 稍亮，与内容区有区分 |
| sidebarText | `#AFAFB9` 灰 | `#D1D2D6` 亮灰 | 提升对比度 |
| sidebarSelected | `#508AFF` 蓝 | `#508AFF` 蓝 | 保持不变 |
| sidebarHover | `#1E1E24` 深灰 | `#2A2A32` 中灰 | 更明显的悬停反馈 |

### 2. NavSidebar.cpp — 消除硬编码颜色

**2a. 标题颜色**：硬编码 `wxColour(22, 93, 255)` → 使用 `ThemeManager::Instance().GetColors().accent`

**2b. 图标颜色**：10 个硬编码颜色 → 使用 ThemeManager 颜色系统
- 浅色主题：使用较深的图标色（与深色文字协调）
- 深色主题：使用较亮的图标色（与亮色文字协调）
- 选中项图标色：使用白色（浅色主题选中蓝底白字）或 accent 色

**2c. 选中项文字色**：`*wxWHITE` → 使用 ThemeManager 新增的 `sidebarSelectedText` 颜色
- 浅色主题：白色（蓝底白字）
- 深色主题：白色（蓝底白字）

**2d. 主题变更回调**：已有 RegisterChangeCallback，需确保所有颜色字段都更新

### 3. ThemeManager.h — 新增 sidebarSelectedText 颜色字段

在 ThemeColors 中新增 `wxColour sidebarSelectedText` 字段：
- 浅色主题：`#FFFFFF` 白色
- 深色主题：`#FFFFFF` 白色

### 4. NavSidebar.cpp — 图标颜色适配方案

新增 ThemeColors 字段或使用现有字段组合：

**浅色主题图标色**（深色，与浅色背景协调）：
```
首页:     #2B5FCC (深蓝)
深度清理: #0D8A5C (深绿)
智能迁移: #C45A14 (深橙)
加速优化: #B58A08 (深黄)
软件管理: #C43838 (深红)
软件推荐: #6D3ABF (深紫)
安全防护: #C43858 (深玫红)
网络优化: #1D8A4E (深翠绿)
设置:     #5A6068 (深灰)
关于:     #2D6BC8 (深浅蓝)
```

**深色主题图标色**（亮色，与深色背景协调）：
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

### 5. 其他面板硬编码颜色修复

| 文件 | 问题 | 修复 |
|------|------|------|
| ScanResultPanel.cpp | `SetBackgroundColour(*wxWHITE)` | → `ThemeManager::Instance().GetColors().background` |
| MigrationPanel.cpp | `SetBackgroundColour(*wxWHITE)` | → 同上 |
| DiskAnalyzerPanel.cpp | `SetBackgroundColour(*wxWHITE)` | → 同上 |
| StartupPanel.cpp | `dc.SetBrush(*wxWHITE_BRUSH)` | → 使用 ThemeManager surface 色 |
| SecurityPanel.cpp | 多处 `wxColour(245, 245, 247)` | → `ThemeManager::Instance().GetColors().surface` |
| MainWindow.cpp | 分隔线 `wxColour(230, 230, 230)` | → `ThemeManager::Instance().GetColors().divider` |

## 修改文件清单

| 文件 | 修改内容 |
|------|---------|
| `src/gui/controls/ThemeManager.h` | ThemeColors 新增 sidebarSelectedText 字段 |
| `src/gui/controls/ThemeManager.cpp` | 重定义浅色/深色主题的 sidebar 4+1 个颜色值 |
| `src/gui/controls/NavSidebar.cpp` | 消除所有硬编码颜色，图标色跟随主题，标题色跟随主题 |
| `src/gui/panels/ScanResultPanel.cpp` | 替换 *wxWHITE |
| `src/gui/panels/MigrationPanel.cpp` | 替换 *wxWHITE |
| `src/gui/panels/DiskAnalyzerPanel.cpp` | 替换 *wxWHITE |
| `src/gui/panels/StartupPanel.cpp` | 替换 *wxWHITE_BRUSH |
| `src/gui/panels/SecurityPanel.cpp` | 替换硬编码 wxColour(245,245,247) 等 |
| `src/gui/MainWindow.cpp` | 替换分隔线硬编码颜色 |

## 验证步骤

1. 编译通过
2. 浅色主题下侧边栏为浅灰蓝色，文字深色清晰可读
3. 深色主题下侧边栏对比度提升，文字明亮
4. 悬停状态明显可辨
5. 选中项品牌蓝背景 + 白字
6. 图标颜色在浅色/深色主题下都协调
7. 主题切换时侧边栏正确响应
