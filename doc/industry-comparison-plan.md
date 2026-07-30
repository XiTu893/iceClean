# IceClean 业界工具对标分析与提升方案

> 制定日期：2026-07-30
> 说明：本方案分析业界主流 Windows 优化/清理工具的功能特点，提出 IceClean 的提升方向。
> **UI 原则：保持原始简洁风格，不追求花哨动效，以清晰高效为目标。**

---

## 一、对标工具总览

| 工具 | 定位 | 技术栈 | 开源 | 活跃度 | 值得借鉴的核心能力 |
|------|------|--------|------|--------|-------------------|
| **Chris Titus WinUtil** | 一站式系统精简/软件安装 | PowerShell+WPF | ✅ MIT | ⭐59k | 预设配置系统、模块化架构、Winget集成 |
| **Win11Debloat** | 轻量级精简/隐私脚本 | PowerShell | ✅ MIT | ⭐54k | 隐私控制全面、AI功能(Copilot/Recall)禁用、变更可还原 |
| **RogueCleaner** | 流氓软件查杀 | Win32 | ❌ | 中国区 | 中国特供流氓软件规则库、浏览器劫持修复、顽固文件删除 |
| **Optimizer** | 隐私+安全+系统优化 | C#/.NET WinForms | ✅ GPLv3 | ⭐18k | 服务管理、DNS切换、文件锁识别、硬件检测 |
| **CCleaner** | 磁盘/注册表清理 | C++ | ❌ 商业 | 业界标杆 | 深度注册表清理、浏览器全覆盖、驱动清理 |
| **BCUninstaller** | 批量软件卸载 | C#/.NET 8 | ✅ Apache2 | ⭐20k | 10+检测来源、智能静默、残留清理、应用逆向识别 |
| **O&O ShutUp10++** | 隐私/反间谍 | Win32 Native | ❌ 免费软件 | 商业 | 色彩编码安全建议、持久化防重置 |
| **WPD** | 隐私仪表盘 | Win32 API | ❌ | 已停更 | 双层防御(设置+网络层)、极轻量 |
| **BleachBit** | 开源 CCleaner 替代 | Python | ✅ GPL | 跨平台 | CLI模式、自动化友好、无遥测 |
| **Autoruns** | 启动项深度管理 | C++ Native | ❌ Sysinternals | 微软出品 | 最全启动项检测点(注册表/服务/任务/驱动等) |

---

## 二、差异化定位

### IceClean 的独特优势

1. **C盘专项清理**：已有 12 类扫描器 + 7 类清理器，覆盖系统垃圾核心场景
2. **应用迁移**：微信/QQ/Steam/用户文件夹迁移，业界独家差异化能力
3. **安全等级标注**：三级安全标识（Safe/Caution/Dangerous），用户可控
4. **C++ Native 性能**：相比 PowerShell/.NET 工具，启动和执行效率更高
5. **中文生态适配**：针对国内用户环境优化

### 需要补齐的短板

1. **Windows 精简/Debloat**：无预装应用移除、遥测禁用功能
2. **软件卸载管理**：BCUninstaller 级别的多源检测和残留清理
3. **隐私深度控制**：O&O ShutUp10++ 级别的隐私开关面板
4. **注册表清理深度**：CCleaner 级别的扫描项丰富度
5. **浏览器清理覆盖**：缺失国内浏览器（360/QQ/搜狗等）清理支持
6. **流氓软件识别**：RogueCleaner 级别的中国区流氓软件规则
7. **自动化/静默模式**：定时任务 + CLI 静默运行

---

## 三、提升方案（分阶段）

### 第一阶段：补齐基础能力（高优先级）

#### 1.1 Windows 精简模块（借鉴 WinUtil + Win11Debloat）

```cpp
// 新增: src/core/optimizer/WindowsDebloater.h
class WindowsDebloater : public OptimizerBase {
    // 预装应用移除: Get-AppxPackage 封装
    // 遥测禁用: 注册表 + 服务综合控制
    // Copilot/Recall/OneDrive 移除
    // 右键菜单恢复经典样式
    // 任务栏/资源管理器个性化
};
```

| 具体任务 | 借鉴来源 | 工作量 |
|---------|---------|-------|
| UWP 应用列表查询与批量移除 | WinUtil/Win11Debloat | 3天 |
| 遥测禁用（注册表+服务+计划任务） | WinUtil/O&O | 2天 |
| AI 功能禁用（Copilot/Recall） | Win11Debloat | 1天 |
| 系统个性化（右键菜单/任务栏等） | Win11Debloat | 2天 |
| 预设配置系统（Standard/Minimal/Custom） | WinUtil | 2天 |

#### 1.2 软件卸载增强（借鉴 BCUninstaller）

```cpp
// 扩展: src/core/cleaner/SoftwareUninstaller.h
// 新增检测源
enum class InstallSource {
    Registry,        // 注册表 HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall
    Hidden,          // 隐藏/受保护应用
    Store,           // Windows Store (UWP)
    Steam,           // Steam 游戏
    Portable,        // 便携软件检测
    Chocolatey,      // 包管理器
};
```

| 具体任务 | 借鉴来源 | 工作量 |
|---------|---------|-------|
| 多源应用检测（Steam/UWP/Chocolatey） | BCUninstaller | 3天 |
| 智能静默卸载（自动处理弹窗） | BCUninstaller | 3天 |
| 卸载后残留扫描清理 | BCUninstaller | 2天 |
| 强制卸载（损坏安装程序处理） | BCUninstaller | 2天 |
| 卸载前创建还原点 | 已有能力集成 | 0.5天 |

#### 1.3 隐私控制面板（借鉴 O&O ShutUp10++）

```cpp
// 新增: src/core/optimizer/PrivacyOptimizer.h
class PrivacyOptimizer : public OptimizerBase {
    // 集中管理所有 Windows 隐私设置
    // 色彩编码安全建议 (Green/Yellow/Red)
    // 一键应用预设配置
    // 变更备份与逐个还原
};
```

| 具体任务 | 借鉴来源 | 工作量 |
|---------|---------|-------|
| 隐私设置项清单（50+项） | O&O ShutUp10++ | 3天 |
| 三级安全色彩编码 | O&O ShutUp10++ | 1天 |
| 一键应用/还原 | O&O ShutUp10++ | 2天 |
| 网络层防御（防火墙/Hosts） | WPD | 2天 |

---

### 第二阶段：深度增强（中优先级）

#### 2.1 流氓软件识别（借鉴 RogueCleaner）

```cpp
// 扩展: src/core/safety/MalwareDetector.h
class MalwareDetector {
    // 已知流氓软件特征库（2345/PPS/Flash特供版等）
    // 浏览器劫持检测与修复
    // 顽固文件/进程强制终止
    // 社区规则可更新
};
```

| 具体任务 | 借鉴来源 | 工作量 |
|---------|---------|-------|
| 中国区流氓软件特征库 | RogueCleaner | 3天 |
| 浏览器劫持检测与修复 | RogueCleaner | 2天 |
| 顽固文件强制删除引擎 | RogueCleaner | 2天 |
| 规则更新机制 | - | 2天 |

#### 2.2 注册表清理深度扩展（借鉴 CCleaner）

```cpp
// 扩展: src/core/cleaner/RegistryCleaner.h
// 新增扫描类型
enum class RegistryCleanType {
    // 已有
    InvalidPath,      // 无效路径引用
    EmptyKey,         // 空键
    OrphanedOCX,      // 孤立 OCX/COM
    // 新增
    SharedDLL,        // 共享 DLL 无效引用
    FontRef,          // 字体引用缺失
    HelpFile,         // 帮助文件引用缺失
    MRUHistory,       // MRU 历史残留
    ActiveX,          // ActiveX/TypeLib 无效
    InstallerStale,   // 安装程序残留
    MUICache,         // MUI 缓存
    EnvPath,          // 环境变量无效路径
    FileExt,          // 文件扩展名残留
    AppPath,          // App Paths 残留
    UninstallStale,   // 卸载信息残留
};
```

| 具体任务 | 借鉴来源 | 工作量 |
|---------|---------|-------|
| 扩展扫描项至 15+ 类型 | CCleaner | 4天 |
| 注册表备份与恢复增强 | 现有改进 | 1天 |
| 智能安全过滤（白名单） | 已有能力 | 0.5天 |

#### 2.3 浏览器清理全覆盖

| 具体任务 | 借鉴来源 | 工作量 |
|---------|---------|-------|
| 新增国内浏览器支持（360/QQ/搜狗/猎豹） | CCleaner | 2天 |
| 新增 Brave/Vivaldi/Opera 支持 | CCleaner | 1天 |
| 浏览器历史/Cookie/缓存/密码 完整清理 | CCleaner | 2天 |
| 浏览器扩展/插件检测 | CCleaner | 1天 |

#### 2.4 网络优化（借鉴 Optimizer）

```cpp
// 扩展: src/core/optimizer/NetworkOptimizer.h
class NetworkOptimizer : public OptimizerBase {
    // DNS 快速切换（公共 DNS 预设）
    // HOSTS 文件编辑与管理
    // TCP/IP 参数优化
    // 网络连接 Ping/延迟检测
    // SHODAN.io IP 查询（可选）
};
```

| 具体任务 | 借鉴来源 | 工作量 |
|---------|---------|-------|
| DNS 切换（阿里/114/Google/Cloudflare） | Optimizer | 1天 |
| HOSTS 编辑 | Optimizer | 1天 |
| TCP/IP 参数优化 | Optimizer | 1天 |
| 网络延迟检测 | Optimizer | 1天 |

---

### 第三阶段：自动化与生态（低优先级）

#### 3.1 自动化与静默模式

| 具体任务 | 借鉴来源 | 工作量 |
|---------|---------|-------|
| CLI 命令行参数（静默扫描/清理） | BleachBit/WinUtil | 2天 |
| 预设配置导出/导入 | WinUtil | 1天 |
| 定时任务增强（新增场景） | 现有改进 | 2天 |
| 清理完成后自动操作（关机/休眠） | CCleaner | 1天 |

#### 3.2 硬件检测（借鉴 Optimizer）

```cpp
// 新增: src/core/analyzer/HardwareDetector.h
// CPU/GPU/内存/磁盘信息收集
// 硬件温度检测（可选）
// 系统信息报告
```

| 具体任务 | 借鉴来源 | 工作量 |
|---------|---------|-------|
| 硬件信息收集 | Optimizer | 2天 |
| 系统信息报告生成 | Optimizer | 2天 |
| 诊断报告导出 | 已有 DiagnosticReporter | 1天 |

#### 3.3 启动项管理增强（借鉴 Autoruns）

| 具体任务 | 借鉴来源 | 工作量 |
|---------|---------|-------|
| 扩展启动项检测点（服务/驱动/计划任务/代码注入等） | Autoruns | 3天 |
| 启动项安全分析/社区评级 | Autoruns | 2天 |
| 延迟启动配置 | CCleaner | 1天 |

---

## 四、UI 设计原则

**保持原始简洁风格，不做过度设计。**

### 核心原则
1. **功能优先**：UI 以清晰展示信息为首要目标，不添加无意义的装饰元素
2. **扁平化**：无渐变、阴影、复杂动效，使用纯色块和简洁线条
3. **高信息密度**：减少空白浪费，一屏展示更多有效信息
4. **色彩克制**：以系统色为主，安全等级用红/黄/绿三点标识
5. **响应式**：控制窗口缩放行为合理，不多余

### 新增功能的 UI 建议

| 新增模块 | UI 方案 | 集成位置 |
|---------|--------|---------|
| Windows 精简 | 左侧导航新增"系统精简"面板，清单列表+一键操作按钮 | 导航栏新增项 |
| 软件卸载 | 扩展已有 UninstallPanel，增加来源筛选/批量操作 | 现有面板升级 |
| 隐私控制 | 左侧导航新增"隐私设置"面板，分类开关列表 | 导航栏新增项 |
| 流氓软件识别 | 集成到已有 SecurityPanel | 现有面板升级 |
| 网络优化 | 左侧导航新增"网络优化"面板 | 导航栏新增项 |
| 硬件检测 | 集成到 AboutPanel 或新增简洁面板 | 现有面板升级 |

### 导航结构（保持简洁）

```
仪表盘        → DashboardPanel
深度清理      → DeepCleanPanel
智能迁移      → MigrationPanel
系统精简      → DebloatPanel        ← 新增
软件卸载      → UninstallPanel
隐私设置      → PrivacyPanel        ← 新增
网络优化      → NetworkPanel        ← 新增
磁盘分析      → DiskAnalyzerPanel
启动优化      → StartupPanel
安全保护      → SecurityPanel
设置          → SettingsPanel
关于          → AboutPanel
```

---

## 五、优先级评估矩阵

| 功能 | 用户价值 | 开发成本 | 差异化 | 推荐优先级 |
|------|---------|---------|-------|-----------|
| Windows 精简(Debloat) | ⭐⭐⭐⭐⭐ | 低(10天) | 中 | **P0** |
| 软件卸载(BCUninstaller级) | ⭐⭐⭐⭐⭐ | 中(15天) | 中 | **P0** |
| 隐私控制面板 | ⭐⭐⭐⭐ | 中(12天) | 高 | **P1** |
| 浏览器清理全覆盖 | ⭐⭐⭐⭐ | 低(6天) | 低 | **P1** |
| 注册表清理深度扩展 | ⭐⭐⭐⭐ | 中(8天) | 低 | **P1** |
| 流氓软件识别 | ⭐⭐⭐⭐ | 中(10天) | **极高** | **P1** |
| 网络优化 | ⭐⭐⭐ | 低(5天) | 中 | **P2** |
| 自动/静默模式 | ⭐⭐⭐ | 中(8天) | 中 | **P2** |
| 硬件检测 | ⭐⭐ | 低(5天) | 低 | **P3** |
| 启动项管理增强 | ⭐⭐⭐ | 中(8天) | 中 | **P2** |

---

## 六、实施建议

### 排期建议

| 阶段 | 功能 | 预估工时 | 建议开始时间 |
|------|------|---------|------------|
| **P0-1** | Windows 精简 (Debloat) | 10天 | 第1周 |
| **P0-2** | 软件卸载增强 | 15天 | 第3周 |
| **P1-1** | 隐私控制面板 | 12天 | 第6周 |
| **P1-2** | 浏览器全覆盖 + 注册表深度 | 14天 | 第8周 |
| **P1-3** | 流氓软件识别 | 10天 | 第10周 |
| **P2** | 网络优化 + 自动模式 + 启动增强 | 18天 | 第12周 |
| **P3** | 硬件检测 | 5天 | 第15周 |

### 技术风险

1. **Windows 精简兼容性**：不同 Windows 版本（22H2/23H2/24H2）设置路径可能不同
2. **UAC 权限**：多数操作需要管理员权限，需确保提权流程顺畅
3. **BCUninstaller 级卸载**：处理损坏/挂起的卸载程序有较高技术难度

### 流氓软件特征库维护方案

约定从 GitHub 仓库远程拉取规则文件，复刻已有成熟模式：

| 项目 | 详情 |
|------|------|
| **规则文件 URL** | `https://raw.githubusercontent.com/XiTu893/iceClean/main/docs/softdetail.json` |
| **技术方案** | 复用 `SoftwareRecommendFetcher` 的 WinHTTP + nlohmann/json 异步获取模式 |
| **缓存策略** | 启动时后台尝试获取新版本。远程可访问 → 更新本地缓存；远程不可访问 → 继续使用本地缓存，不设过期强制 |
| **规则版本** | JSON 顶层 `version` 字段控制，本地缓存版本低于远程版本时更新 |
| **更新机制** | 应用启动时后台静默检查，不阻塞 UI，用户也可手动触发更新 |
| **降级策略** | 网络不可用时使用本地缓存的规则，无缓存时仅清理已知安全项 |

**复用现有代码结构**：`MalwareDetector` 的规则获取逻辑可直接参考 `SoftwareRecommendFetcher`（详见 `src/core/safety/SoftwareRecommendFetcher.cpp`），共用 `utils/Win32Util.h` 中的 WinHTTP 工具函数（建议将重复的 WinHTTP 代码抽取为公共工具方法）。

**softdetail.json 结构设计草案**（见本文档附录 A）。

> 注：`docs/` 目录已存在于仓库根目录，`recommended_software.json` 已在其中，可直接在同目录添加 `softdetail.json`。

---

## 附录 A：softdetail.json 结构草案

```json
{
  "version": 1,
  "updated_at": "2026-07-30",
  "rules": [
    {
      "id": "rogue-001",
      "name": "2345 安全卫士",
      "aliases": ["2345安全卫士", "2345卫士"],
      "category": "adware",
      "safety_level": "dangerous",
      "indicators": {
        "processes": ["2345Safe.exe", "2345Guard.exe", "2345Svc.exe"],
        "registry": [
          "HKLM\\SOFTWARE\\2345\\SafeGuard",
          "HKCU\\Software\\2345\\SafeGuard"
        ],
        "files": [
          "%ProgramFiles%\\2345\\SafeGuard",
          "%AppData%\\2345\\SafeGuard"
        ],
        "services": ["2345SafeSvc", "2345GuardSvc"],
        "scheduled_tasks": ["2345SafeGuardUpdate"],
        "browser_hijack": {
          "homepage": ["2345.com", "hao.2345.com"],
          "search_engine": ["2345.com", "so.2345.com"]
        }
      },
      "cleanup_actions": {
        "kill_process": true,
        "stop_service": true,
        "delete_registry": true,
        "delete_files": true,
        "restore_browser": true
      },
      "description": "2345 旗下安全软件，常通过捆绑安装，难以彻底卸载"
    },
    {
      "id": "rogue-002",
      "name": "PPS 影音",
      "aliases": ["PPS", "PPS网络电视", "PPStream"],
      "category": "adware",
      "safety_level": "dangerous",
      "indicators": {
        "processes": ["pps.exe", "ppsap.exe", "PPStream.exe"],
        "registry": [
          "HKLM\\SOFTWARE\\PPS",
          "HKCU\\Software\\PPStream"
        ],
        "files": [
          "%ProgramFiles%\\PPS",
          "%AppData%\\PPS",
          "%AppData%\\PPStream"
        ],
        "services": ["PPSService", "PPSStreaming"],
        "browser_hijack": {
          "homepage": ["www.pps.tv", "v.pps.tv"]
        }
      },
      "cleanup_actions": {
        "kill_process": true,
        "stop_service": true,
        "delete_registry": true,
        "delete_files": true,
        "restore_browser": false
      },
      "description": "PPS 影音播放器，包含大量弹窗广告和后台进程"
    },
    {
      "id": "rogue-003",
      "name": "Flash 中国特供版",
      "aliases": ["Flash中国版", "Flash Helper", "Flash中心"],
      "category": "adware",
      "safety_level": "dangerous",
      "indicators": {
        "processes": ["FlashHelperService.exe", "FlashUtil.exe", "FlashCenter.exe"],
        "registry": [
          "HKLM\\SOFTWARE\\Macromedia\\FlashPlayer",
          "HKCU\\Software\\FlashHelper"
        ],
        "files": [
          "%ProgramFiles%\\FlashHelper",
          "%AppData%\\Macromedia\\FlashPlayer"
        ],
        "services": ["FlashHelperService"],
        "scheduled_tasks": ["FlashHelperUpdate"]
      },
      "cleanup_actions": {
        "kill_process": true,
        "stop_service": true,
        "delete_registry": true,
        "delete_files": true,
        "restore_browser": false
      },
      "description": "Adobe Flash 中国特供版，包含广告弹窗和后台服务"
    },
    {
      "id": "rogue-004",
      "name": "360 安全卫士极速版弹窗",
      "aliases": ["360弹窗", "360 Safe Popup"],
      "category": "popup",
      "safety_level": "caution",
      "indicators": {
        "processes": ["360pop.exe", "360Notify.exe", "360SoftMgr.exe"],
        "registry": []
      },
      "cleanup_actions": {
        "kill_process": true,
        "delete_registry": false,
        "delete_files": false,
        "restore_browser": false
      },
      "description": "360 安全卫士的弹窗广告进程，不影响主程序功能"
    },
    {
      "id": "rogue-005",
      "name": "驱动精灵/驱动人生捆绑",
      "aliases": ["驱动精灵", "驱动人生", "DriverGenius", "DriverLife"],
      "category": "bundled_software",
      "safety_level": "caution",
      "indicators": {
        "processes": ["DriverGenius.exe", "DriverLife.exe", "DrvSetup.exe"],
        "registry": [
          "HKLM\\SOFTWARE\\DriverGenius",
          "HKLM\\SOFTWARE\\DriverLife"
        ],
        "files": [
          "%ProgramFiles%\\DriverGenius",
          "%ProgramFiles%\\DriverLife"
        ],
        "scheduled_tasks": ["DriverGeniusUpdate", "DriverLifeUpdate"]
      },
      "cleanup_actions": {
        "kill_process": true,
        "stop_service": false,
        "delete_registry": true,
        "delete_files": true,
        "restore_browser": false
      },
      "description": "驱动管理工具，常通过其他软件捆绑安装，含弹窗推广"
    }
  ]
}
```

**字段说明：**

| 字段 | 说明 |
|------|------|
| `id` | 规则唯一标识，用于本地缓存匹配 |
| `name` | 规则名称 |
| `aliases` | 别名，用于多名称匹配 |
| `category` | 分类：adware(广告软件)/popup(弹窗)/bundled_software(捆绑)/browser_hijack(浏览器劫持)/toolbar(工具栏) |
| `safety_level` | 安全等级：safe/caution/dangerous |
| `indicators` | 检测指标：进程/注册表/文件/服务/计划任务/浏览器劫持 |
| `cleanup_actions` | 清理策略：各维度是否执行清理 |
| `description` | 描述说明 |

### 不可实施的建议（明确排除）

| 建议 | 排除原因 |
|------|---------|
| 实时病毒监控 | 超出 C 盘清理工具定位，技术成本极高 |
| 云端存储/同步 | 需要服务器维护，与离线工具定位冲突 |
| 复杂 3D 动效 UI | 违反简洁 UI 原则 |
| 跨平台支持 | Windows 专有工具，无跨平台需求 |

---

## 七、能力对比预测（实施后）

```
能力维度              IceClean当前    P0实施后       P1实施后       P2/P3实施后
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
C盘垃圾清理           ████████░░ 80%  ████████░░ 80%  █████████░ 90%  █████████░ 90%
Windows 精简          ░░░░░░░░░░  0%  ████████░░ 80%  ████████░░ 80%  ████████░░ 80%
软件卸载              █████░░░░░ 50%  █████████░ 90%  █████████░ 90%  ██████████ 100%
注册表清理            ████░░░░░░ 40%  ████░░░░░░ 40%  ██████████ 100%  ██████████ 100%
隐私保护              ████░░░░░░ 40%  ██████░░░░ 60%  ██████████ 100%  ██████████ 100%
应用迁移              █████████░ 90%  █████████░ 90%  █████████░ 90%  █████████░ 90%
流氓软件识别          ██░░░░░░░░ 20%  ██░░░░░░░░ 20%  ██████████ 100%  ██████████ 100%
网络优化              ░░░░░░░░░░  0%  ░░░░░░░░░░  0%  ░░░░░░░░░░  0%  ████████░░ 80%
启动管理              ██████░░░░ 60%  ██████░░░░ 60%  ██████░░░░ 60%  █████████░ 90%
磁盘分析              ████████░░ 80%  ████████░░ 80%  ████████░░ 80%  ████████░░ 80%
综合评分              ████░░░░░░ 42%  ██████░░░░ 58%  ████████░░ 75%  █████████░ 88%
```

---

## 八、关键行动项

1. **确认 UI 风格**：保持现有简约设计，新增面板风格统一，不做大改
2. **优先实施 P0**：Windows 精简 + 软件卸载增强，这是用户感知最强、投入产出比最高的模块
3. **差异化竞争**：流氓软件识别是 IceClean 区别于国际工具的独特卖点，建议 P1 优先
4. **外部规则文件**：流氓软件特征、Windows 精简清单、隐私设置项等使用 JSON 外部配置，便于社区贡献
5. **持续监控行业动态**：Windows 版本更新可能影响精简/隐私功能，需保持跟踪

---

> **下一步**：确认本方案后，针对 P0 模块出具详细的技术设计文档和代码实现。
