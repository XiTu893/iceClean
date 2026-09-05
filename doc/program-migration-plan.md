# 智能迁移增强方案：Program Files 应用迁移

**制定日期**：2026-09-03
**方案状态**：待确认后实施

---

## 一、需求背景

用户希望智能迁移功能能够分析 `C:\Program Files` 和 `C:\Program Files (x86)` 下的应用程序，帮助用户将不常用但体积较大的应用迁移到其他盘符，从而释放 C 盘空间。

---

## 二、业界调研结论

### 2.1 主流工具对比

| 工具 | 类型 | 是否提供应用迁移 |
|------|------|----------------|
| TreeSize | 磁盘分析 | ❌ 仅分析，不迁移 |
| CCleaner | 系统清理 | ❌ 清理注册表/临时文件 |
| WizTree | 磁盘扫描 | ❌ 快速扫描，无迁移 |
| SpaceMonger | 磁盘可视化 | ❌ 仅显示占用 |
| Steam | 游戏平台 | ✅ 内置库管理（原理：复制+重定向）|
| **FreeMove** | 应用迁移 | ✅ **唯一专注此领域的成熟开源工具（2.3k stars）** |

**结论**：主流清理工具均**不提供**通用的 Program Files 应用迁移功能。FreeMove 是业界唯一成熟方案。

### 2.2 FreeMove 核心技术原理

```
1. 将文件移动到新位置
2. 在旧位置创建一个符号链接/目录联接(symbolic link / junction point)
3. 任何访问旧位置的程序自动被重定向到新位置
```

IceClean 项目已完整实现这一机制（`JunctionPoint.cpp`）。

### 2.3 重要结论

> "可以自由移动 `C:\Program Files\HugeProgramIDontWantOnMySSD` 而不会产生任何问题"

FreeMove 明确指出：**可以自由移动 Program Files 内**的具体应用，不会破坏系统。

---

## 三、现有架构分析

### 3.1 已有的迁移相关模块

| 模块 | 职责 | 状态 |
|------|------|------|
| `JunctionPoint.cpp` | 目录联接创建 | ✅ 可直接复用 |
| `LargeFolderDetector` | 大文件夹扫描 | ✅ 可参考 |
| `MigratorBase` | 迁移流程（复制→删除→联接）| ✅ 可复用 |
| `SteamMigrator` | Steam 游戏迁移 | ✅ 可参考 |
| `UserFolderMigrator` | 用户文件夹迁移 | ✅ 可参考 |
| `MigrationPanel` | 迁移 UI 面板 | ⚠️ 需增强 |
| `MigrationItem` | 迁移项数据模型 | ⚠️ 需扩展 |

### 3.2 当前迁移面板的局限性

- 扫描逻辑基于 `LargeFolderDetector`，按**文件夹大小**排序
- 无法区分"普通文件夹"和"可执行应用"
- 无法显示应用的**元信息**（名称、发行商、安装日期、是否可安全迁移）
- 无法识别应用是否正在**运行**

---

## 四、方案设计

### 4.1 新增模块：ProgramMigrator

```
src/core/migrator/
├── ProgramMigrator.h          (新增) 程序迁移器
├── ProgramMigrator.cpp
├── ProgramInfo.h              (新增) 程序信息模型
└── ProgramInfo.cpp
```

### 4.2 ProgramInfo 数据模型

```cpp
struct ProgramInfo {
    std::wstring name;              // DisplayName
    std::wstring installLocation;   // 安装路径
    std::wstring publisher;         // 发行商
    std::wstring version;           // 版本
    std::wstring uninstallString;   // 卸载命令
    uint64_t sizeBytes = 0;         // 程序目录大小
    bool is64bit = false;           // 是否64位
    bool isRunning = false;          // 是否正在运行
    SafetyLevel safetyLevel;         // 安全级别（Safe/Caution/Danger）
    std::wstring safetyReason;       // 安全说明
};
```

### 4.3 扫描逻辑

#### 阶段一：注册表识别（快速）

扫描以下注册表位置，筛选 `InstallLocation` 包含 `C:\Program Files` 或 `C:\Program Files (x86)` 的条目：

```
HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{GUID}
HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\{GUID}
HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{GUID}
```

#### 阶段二：目录大小测量（慢速，可取消）

对阶段一找到的每个程序目录，调用已有的 `LargeFolderDetector::MeasureDirectory` 获取精确大小。

#### 阶段三：进程与安全检查

- 检测应用是否在运行（`IsProcessRunning`）
- 安全级别评估：
  - 🟢 安全：普通桌面应用，可迁移
  - 🟡 谨慎：系统组件/驱动，提示风险后可迁移
  - 🔴 危险：禁止迁移（Windows 系统目录）

### 4.4 迁移流程（复用 MigratorBase）

```
1. 预检查
   ├── 目标盘是否有足够空间
   ├── 应用是否正在运行（提示关闭）
   └── 是否创建系统还原点（强烈建议）

2. 执行迁移
   ├── FileUtil::CopyFolder(source, target)   // 复制到目标盘
   ├── FileUtil::DeleteFolder(source)           // 删除源目录
   └── JunctionPoint::Create(source, target)     // 在源路径创建联结点

3. 后处理
   ├── 验证联结点是否生效
   └── 提示用户重启应用
```

### 4.5 UI 增强（MigrationPanel）

新增**两个扫描模式**：

| 模式 | 说明 | 触发条件 |
|------|------|----------|
| 大文件夹扫描（现有）| 按文件夹大小排序，适合用户自定义大目录 | 默认显示 |
| 应用迁移扫描（新增）| 识别 Program Files 中的应用，按大小排序 | 新增切换 Tab |

**应用迁移列表列**：

| 列名 | 说明 |
|------|------|
| ☑ | 选择框 |
| 程序图标 | 提取 .exe 图标 |
| 程序名称 | DisplayName |
| 安装位置 | 路径（截断显示）|
| 大小 | 目录占用空间 |
| 发行商 | Publisher |
| 安全级别 | 🟢/🟡/🔴 图标 |
| 状态 | 空闲/运行中/已迁移 |

**安全提示对话框**：

```
⚠️ 应用迁移风险提示

以下应用正在运行，建议先关闭后再迁移：
- Adobe Photoshop
- Microsoft Visual Studio

[仍要迁移]  [取消]
```

---

## 五、安全策略

### 5.1 黑名单（禁止迁移）

```
C:\Program Files\Windows*
C:\Program Files\Microsoft*
C:\Program Files\Internet Explorer*
C:\Program Files\Common Files\Microsoft Shared*
C:\Program Files\Windows Defender*
C:\Program Files\Windows Mail*
C:\Program Files\Windows Media Player*
C:\Program Files\Windows Photo Viewer*
C:\Program Files\Windows Security*
```

### 5.2 灰名单（需二次确认）

```
C:\Program Files\Microsoft Office*
C:\Program Files\Microsoft Visual Studio*
C:\Program Files\Adobe*
C:\Program Files\Autodesk*
```

### 5.3 白名单（可直接迁移）

```
C:\Program Files\7-Zip
C:\Program Files\Bandizip
C:\Program Files\Notepad++
C:\Program Files\VSCode
C:\Program Files\JetBrains*
C:\Program Files\WinRAR
C:\Program Files\OBS*
C:\Program Files\Git*
C:\Program Files\nodejs
C:\Program Files\Python*
```

---

## 六、实施计划

### Phase 1: 核心功能（预计 2-3 天）

1. **新增 `ProgramInfo` 数据模型**
2. **新增 `ProgramMigrator` 扫描器**
   - 读取注册表识别应用
   - 测量目录大小
   - 安全级别评估
3. **扩展 `MigrationItem` 支持程序类型**
4. **增强 `MigrationPanel` UI**
   - Tab 切换（大文件夹 / 应用）
   - 新列表列（名称、发行商、安全级别）

### Phase 2: 体验优化（预计 1-2 天）

5. **进程检测**：扫描时显示正在运行的应用
6. **图标提取**：显示各应用的 .exe 图标
7. **迁移前确认对话框**：显示风险提示
8. **操作日志**：记录迁移历史

### Phase 3: 进阶功能（可选）

9. **系统还原点**：迁移前自动创建还原点（调用 `SystemRestore` API）
10. **回滚功能**：一键将应用从 Junction 状态恢复到原始位置
11. **应用推荐**：识别不常用的大应用，建议卸载或迁移

---

## 七、技术风险

| 风险 | 概率 | 影响 | 缓解措施 |
|------|------|------|----------|
| 杀毒软件拦截 Junction 创建 | 低 | 中 | 显示白名单/数字签名提示 |
| 目标盘 Junction 不生效 | 低 | 高 | 迁移后验证，失败则回滚 |
| 应用依赖项缺失 | 中 | 中 | 灰名单二次确认 |
| UWP/Store 应用迁移失败 | 高 | 中 | 自动识别并排除 |
| .NET 应用配置路径硬编码 | 中 | 中 | 黑名单兜底 |

---

## 八、参考资源

- **FreeMove**: https://github.com/imDema/FreeMove
- **Windows 符号链接文档**: https://learn.microsoft.com/en-us/windows/win32/fileio/symbolic-links
- **Reparse Points**: https://learn.microsoft.com/en-us/windows/win32/fileio/reparse-points

---

## 九、方案确认

请确认以下决策点：

1. **迁移机制**：是否继续使用 Junction 联接方案（推荐，已在项目中实现）？
2. **安全策略**：是否采用黑名单/灰名单/白名单三级安全策略？
3. **实施范围**：是否按 Phase 1 → 2 → 3 顺序实施？
4. **优先级**：应用迁移 vs 其他功能（启动优化、隐私清理等）的优先级排序？

---

*方案制定：Claude Code*
*日期：2026-09-03*
