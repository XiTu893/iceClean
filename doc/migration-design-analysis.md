# 智能迁移方案设计分析

> 分析日期：2026-08-29
> 更新：2026-08-29（补充阈值调整 + 跳过列表完善 + CLI 测试验证 + 扫描下钻修复）

---

## 一、修复记录

### 1.0 扫描下钻修复（2026-08-29 新增）

**问题**：
- 阈值 200MB 选择后，仍出现大量 < 200MB 的小文件夹（UserFolderMigrator 无阈值过滤）
- `C:\Users` 被加入跳过列表，导致扫描器无法进入用户目录
- `AppData\Local\vcpkg` (659MB) 永远找不到——因为 `AppData` 整体（27GB）先入列，阻止下钻

**修复**：
1. `UserFolderMigrator::Detect()` 加 100MB 阈值过滤（`constexpr kMinSizeBytes = 100MB`）
2. 从跳过列表移除 `users`, `public`, `default`, `google`, `microsoft`, `mozilla`, `.config`, `.cache`, `.local`
3. 新增容器下钻逻辑：
   - `C:\Users` → `ScanUsersContainer`：枚举用户目录，Public/Default 特殊处理，用户配置目录（zeus-zzp）永远不入列但始终下钻
   - `AppData` 跳过自身但调用 `ScanContainerChildren` 深入子目录
   - 大于 5GB 的目录（`kContainerThreshold`）强制下钻而非入列，防止容器目录屏蔽内部可迁移项
   - 新增 `IsUserProfileDir()` 检测用户配置目录（含 AppData/Desktop/Documents/Downloads 等特征）

**验证结果**（100MB 阈值）：
- `C:\vcpkg` (227 MB) ✓
- `AppData\Local\vcpkg` (931 MB) ✓
- `AppData\Local\Google` (1565 MB) ✓
- `AppData\Roaming\Code` (4383 MB) ✓ — VSCode
- `AppData\Roaming\Tencent` (1327 MB) ✓ — QQ
- `.lmstudio` (2115 MB) ✓
- `Public\Documents\Unity Projects` (532 MB) ✓

### 1.1 阈值调整
- **原阈值**：500MB → **新阈值**：100MB
- **原因**：500MB 太高，大多数用户级大目录（Unity 项目 2-10GB、Android SDK 5-15GB、Adobe 素材库 1-5GB）可能被误判为"太大不值得"而被忽视
- **效果**：`C:\vcpkg` (227MB) 等中等大小目录也能被列出

### 1.2 跳过列表完善
新增以下类别（由专用迁移器处理，或不可迁移）：

| 类别 | 具体条目 | 处理方式 |
|------|---------|---------|
| 用户目录容器 | `users`, `public`, `default` | 跳过（由 UserFolderMigrator 处理） |
| 开发缓存 | `.gradle`, `.m2`, `.npm`, `.yarn`, `.pnpm-store`, `.config`, `.cache`, `.local` | 跳过（由 DevCacheMigrator 处理） |
| 前端缓存 | `node_modules`, `bower_components`, `__pycache__`, `.pytest_cache`, `.tox`, `.venv` | 跳过（DevCacheMigrator） |
| IDE 缓存 | `.idea`, `.vscode` | 跳过（DevCacheMigrator） |
| 云同步 | `onedrive`, `dropbox` 及所有前缀变体 | 跳过（junction 迁移会破坏同步） |

### 1.3 FILE_ATTRIBUTE_SYSTEM 检查移除
- **原因**：Windows 的 SYSTEM 属性不仅限于系统文件夹，部分 AppData 目录也有此属性，导致误杀
- **修复**：移除 `FILE_ATTRIBUTE_SYSTEM` 检查，仅保留：
  - `FILE_ATTRIBUTE_REPARSE_POINT`（跳过 junction 链接）
  - 精确跳过列表（名称匹配）

### 1.4 CLI 测试验证结果

```
LargeFolderDetector: C:\vcpkg (227 MB) ✅
微信: 未安装 ✅
QQ: 未安装 ✅
Steam: 未安装 ✅
用户文件夹: Desktop/Documents/Pictures/Videos/Music/Downloads ✅
```

---

## 二、当前迁移层级设计

```
IceClean 扫描结果（阈值 100MB）
├── C:\vcpkg (227 MB)                    ← LargeFolderDetector ✅
├── C:\Program Files\...                  ← 跳过（SYSTEM 属性）
├── C:\Users\zeus-zzp\Desktop            ← UserFolderMigrator ✅
├── C:\Users\zeus-zzp\.gradle           ← 跳过（DevCacheMigrator）
├── C:\Program Files (x86)\SteamLibrary  ← SteamMigrator ✅
└── C:\Users\zeus-zzp\WeChat Files     ← WeChatMigrator ✅
```

---

## 三、选择方式

| 产品 | 选择方式 | IceClean |
|------|---------|---------|
| TreeSize | ✅ 勾选框 | ✅ 已有 |
| WinDirStat | ✅ 勾选框 | ✅ 已有 |
| 360C搬坝 | ✅ 勾选框 | ✅ 已有 |

**结论**：✅ 保持 checkbox 勾选（已实现）。

---

## 四、待确认问题

1. **是否需要展开/折叠功能？**
2. **是否需要显示"已迁移"状态？**
3. **扫描阈值可调节吗？**（当前固定 100MB）

---

## 五、安全标识建议

| 迁移类型 | 标识 | 说明 |
|---------|------|------|
| Steam 游戏 | 🟢安全 | Junction 链接成熟稳定 |
| 微信/QQ | 🟡谨慎 | 可能被进程占用 |
| 开发工具缓存 | 🟢安全 | Junction 链接 + DevCacheMigrator 专门处理 |
| 用户文件夹 | 🟡谨慎 | 影响系统快捷方式 |
| 大型软件目录 | 🟡谨慎 | 可能有 DLL 依赖 |

