# IceClean - 智能C盘清理与系统优化工具

<div align="center">

![Logo](XiTu-logo.jpg)

**一款专为 Windows 用户打造的 C 盘清理与系统优化工具，让磁盘重获新生**

[![Build](https://github.com/XiTu893/iceClean/actions/workflows/build.yml/badge.svg)](https://github.com/XiTu893/iceClean/actions/workflows/build.yml)
[![License](https://img.shields.io/github/license/XiTu893/iceClean?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010%2F11%20(64--bit)-0078D4?style=flat-square)](#)
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=cplusplus)](#)

**[安装包下载](https://github.com/XiTu893/iceClean/releases/latest)** &nbsp;|&nbsp; **[功能介绍](#功能特性)** &nbsp;|&nbsp; **[快速开始](#快速开始)** &nbsp;|&nbsp; **[编译构建](#编译构建)**

---

![Preview](app_preview.png)

</div>

---

## 产品简介

随着日常使用，Windows 系统盘往往会积累大量临时文件、缓存和冗余数据，导致磁盘空间告急、系统运行缓慢。IceClean 正是为解决这一痛点而生的本地工具——无需联网、无需安装、不收集任何用户数据，一键扫描、一键清理，让您的 C 盘重新轻盈上阵。

> **核心理念**：安全第一，智能清理，数据不丢失。

---

## 功能特性

### 🧹 深度清理 — 释放海量磁盘空间

- **12 类扫描引擎**：系统临时文件、Windows 更新缓存、浏览器缓存（Chrome / Edge / Firefox / Opera / Brave / Vivaldi / 360 / QQ）、缩略图缓存、预读取文件、系统日志、崩溃转储、驱动备份、WinSxS 组件、休眠文件
- **开发工具缓存清理**：npm / yarn / pnpm / pip / conda / Maven / Gradle / Go / NuGet / Cargo / Electron — 动辄数 GB 的缓存一键清零
- **5 类专业清理器**：文件清理、隐私清理、注册表清理、休眠文件清理、WinSxS 组件清理

### 🔄 智能迁移 — 盘间腾挪零负担

- **6 大迁移场景**：Steam 游戏库、用户文件夹（桌面/文档/下载）、微信聊天缓存、QQ 缓存、大型软件、开发工具缓存
- **Junction 链接技术**：迁移后程序无缝续用，无需重装
- **自动枚举**：智能探测所有非系统盘可用空间，迁移目标一目了然

### ⚡ 系统加速 — 开机即巅峰

- **启动项管理**：精准识别注册表与启动文件夹中的冗余项，禁用非必要开机启动
- **服务优化**：智能分析 Windows 系统服务，一键禁用拖累开机速度的无关服务
- **计划任务管理**：清理冗余的计划任务，减少后台资源占用
- **进程管理**：一键发现并终止 QQProtect 等第三方顽固进程，支持三级递进终止策略（普通 → SeDebugPrivilege 强制 → SYSTEM 身份）

### 💻 硬件监控 — 实时掌控系统状态

- **CPU / 内存 / 磁盘 / GPU / 网络** 五大指标实时监控
- **性能历史图表**：图表化呈现系统资源使用趋势
- **硬件详情面板**：CPU 规格、内存容量、磁盘型号、网卡信息一应俱全

### 🛡️ 安全保护 — 放心操作的底气

- **三级安全标识**（🟢安全 / 🟡谨慎 / 🔴危险）贯穿所有扫描与操作
- **50+ 白名单路径**自动保护，系统关键文件永不误删
- **操作前自动创建系统还原点**，随时可回滚
- **操作日志 JSON 持久化**（最多 50 条），所有危险操作全程可追溯
- **危险操作二次确认对话框**，防止手滑

### 📦 软件管理 — 应用管理更省心

- **软件卸载**：列出所有已安装程序，干净卸载不留残余
- **驱动管理**：设备驱动 + 驱动包双视图，支持驱动备份与还原
- **软件推荐**：精选优质软件推荐，界面友好、无需联网

### 🌐 网络优化 — 带宽利用更高效

- **应用流量监控**：实时追踪各进程的网卡占用，帮您揪出偷流量的后台程序
- **网络优化**：系统网络参数调优，提升带宽利用率

---

## 快速开始

1. 前往 **[Releases](https://github.com/XiTu893/iceClean/releases/latest)** 下载最新版 `IceClean_Setup_v*.exe`
2. 右键 → **以管理员身份运行**（完整功能需要管理员权限）
3. 首次使用建议点击**一键扫描**，查看 C 盘健康状况
4. 勾选需要清理的类别，点击**立即清理**
5. 如需迁移大文件，切换到**智能迁移**标签页

> **系统要求**：Windows 10 / Windows 11 (64 位)，建议 1920×1080 及以上分辨率

---

## 技术架构

| 层级 | 描述 |
|------|------|
| **核心语言** | C++20（MSVC 14.44），现代语法，性能与安全兼得 |
| **GUI 框架** | wxWidgets 3.3.1，原生 Windows 外观，轻量流畅 |
| **构建系统** | CMake 3.25+ · vcpkg，x64-windows-static 静态链接，单文件分发 |
| **第三方库** | nlohmann-json（配置）、spdlog（日志）、libwebp（图片解码）、SQLite（本地数据） |
| **CI/CD** | GitHub Actions 自动编译与 Release 发布 |

### 编译构建

```bash
# 克隆仓库
git clone git@github.com:XiTu893/iceClean.git
cd iceClean

# 初始化 vcpkg（如尚未安装）
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg && .\bootstrap-vcpkg.bat && cd ..

# 编译
build_local.bat
```

编译产物位于 `build/x64-debug/src/IceClean.exe`（Debug）或 `build/x64-release/src/IceClean.exe`（Release）。

---

## 项目结构

```
iceClean/
├── src/
│   ├── core/                  # 核心业务逻辑（纯 C++，无 GUI 依赖）
│   │   ├── scanner/            # 12 类扫描器
│   │   ├── cleaner/           # 5 类清理器
│   │   ├── migrator/          # 6 类迁移器（Junction 链接）
│   │   ├── optimizer/         # 启动优化 + 服务优化 + 计划任务
│   │   ├── analyzer/          # 进程分析器 + 硬件监控
│   │   └── safety/            # 还原点 + 白名单 + 操作日志
│   ├── gui/                   # wxWidgets 图形界面
│   │   ├── controls/          # 自绘控件（CustomTitleBar / NavSidebar 等）
│   │   ├── panels/            # 9 大功能面板
│   │   └── dialogs/           # 进度对话框
│   ├── models/                # 数据模型（纯数据类）
│   └── utils/                 # Win32 / 文件 / 注册表工具函数
├── scripts/                   # 安装脚本
└── .github/workflows/         # CI/CD 配置
```

---

## 支持的浏览器与开发工具

| 类别 | 支持项 |
|------|--------|
| **浏览器** | Chrome、Microsoft Edge、Mozilla Firefox、Opera、Brave、Vivaldi、360 安全浏览器、QQ 浏览器 |
| **开发工具缓存** | npm、yarn、pnpm、node-gyp、pip、conda、Maven、Gradle、Go Module、NuGet、Cargo、Electron |

---

## 隐私声明

IceClean 是一款**纯本地工具**，不收集、不上传任何用户数据。所有扫描和清理操作均在本地完成，日志文件仅保存在用户本机。软件推荐数据（来自 GitHub）仅在用户主动点击"刷新"时下载，完全离线使用。

---

## 许可证

- **个人用户**：免费使用
- **企业用户**：需书面授权

联系邮箱：28491599@qq.com

---

<div align="center">

**如果您觉得 IceClean 有用，欢迎 Star ⭐ 支持一下！**

![捐赠](QrReward.jpg)

</div>
