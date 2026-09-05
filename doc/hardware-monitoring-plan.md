# IceClean 硬件检测 & 性能监控 功能提升方案

## 1. 当前功能情况

### 现有功能
- **HardwareInfoPanel**：位于 "关于 → 硬件信息" 标签页
- **HardwareDetector**：C++ 实现，获取静态硬件信息
  - CPU 信息
  - GPU 信息
  - 内存信息
  - 磁盘信息
  - 主板信息
  - 操作系统信息
  - 系统运行时间

### 功能缺口
- ❌ 左侧导航栏无独立入口
- ❌ 无实时性能监控（CPU/内存/磁盘/网络）
- ❌ 无历史趋势数据
- ❌ 无异常告警
- ❌ 无资源占用可视化

---

## 2. 业界系统设计参考

### 主流工具功能划分

| 工具 | 硬件监控方式 | 性能指标 | 特色功能 |
|------|-------------|---------|---------|
| **CCleaner** | About → PC Analysis | CPU/内存/磁盘 | 硬件健康报告、升级提醒 |
| **HWInfo** | 独立仪表盘 | 全面实时监控 | 详细硬件参数、SMART 监控 |
| **MSI Afterburner** | 桌面小组件 | GPU 实时监控 | 叠加式监控、录屏 |
| **Windows 任务管理器** | 实时监控 | CPU/内存/磁盘/网络 | 进程级监控、资源调度 |
| **WinUtil** | PowerShell 脚本 | 硬件健康 | 升级检测、硬件报告 |
| **O&O ShutUp10++** | 静态检测 | 隐私设置 | 僭存运行时检测 |
| **Autoruns** | 启动项分析 | 启动耗时 | 启动优化建议 |

### 行业共识
1. **仪表盘式设计**：CPU 核心指标 + 硬件摘要
2. **实时可视化**：折线图/柱形图显示资源占用
3. **健康评分**：磁盘健康/SMART 状态/内存完整性
4. **历史趋势**：24h/7d 占用峰值跟踪
5. **异常告警**：CPU 超频/温度过高/磁盘健康预警

---

## 3. 功能规格建议

### 3.1 核心监控指标

| 指标 | 数据来源 | 更新频率 | 设计 |
|------|---------|---------|------|
| CPU 使用率 | GetSystemTimes() | 1s | 环形进度条 |
| CPU 核心数 | WMI | 启动时读取 | 静态显示 |
| CPU 频率 | WMI | 5s | 静态显示 |
| 内存使用 | GlobalMemoryStatusEx() | 1s | 条形图 |
| 内存占用 | - | 1s | 数值 + 百分比 |
| 磁盘使用 | GetDiskFreeSpaceEx() | 1s | 圆形仪表盘 |
| 磁盘健康 | SMART 估算 | 启动时读取 | 健康指示灯 |
| 网络下载 | 网卡统计 | 1s | 实时计数 |
| 网络上传 | 网卡统计 | 1s | 实时计数 |

### 3.2 硬件信息

| 项目 | 内容 | 设计 |
|------|------|------|
| CPU 型号 | WMI WIN32_Processor | 名称 + 核心数 + 频率 |
| GPU 型号 | WMI Win32_VideoController | 名称 + 显存 |
| 内存容量 | GlobalMemoryStatusEx | GB 显示 |
| 磁盘列表 | GetLogicalDriveStrings | 多个盘符支持 |
| 主板型号 | WMI WIN32_BaseBoard | 品牌 + 型号 |
| 操作系统 | GetVersionEx | 版本 + 构建号 |
| 运行时间 | GetSystemInfo | 时:分:秒格式 |

### 3.3 可选增强功能

| 功能 | 说明 |
|------|------|
| 硬件健康报告 | 磁盘 SMART + 温度监控 |
| 温度监控 | WMI CIM_Temperature |
| GPU 温度 | NVIDIA/AMD 驱动接口 |
| 风扇转速 | WMI CIM_Fan |
| 电源状态 | 电池/BIOS 电源管理 |
| 电脑外接 | USB 设备列表 |

---

## 4. 实现方案

### 方案架构

```
src/
├── core/
│   └── analyzer/
│       ├── HardwareDetector.h         (现有)
│       ├── PerformanceMonitor.h       (新增)
│       └── HardwareHealthCheck.h      (新增)
├── gui/
│   └── panels/
│       ├── HardwareMonitorPanel.h     (新增或扩展)
│       └── HardwareInfoPanel.cpp      (现有，作为静态信息展示)
└── gui/controls/
    └── PerformanceChart.h             (新增 - 折线图控件)
```

### 4.1 PerformanceMonitor 核心类

```cpp
// src/core/analyzer/PerformanceMonitor.h
class PerformanceMonitor {
public:
    // CPU
    double GetCpuUsage();
    double GetCpuFrequencyMHz();
    int GetCpuCoreCount();

    // Memory
    uint64_t GetTotalPhysicalMemory();
    uint64_t GetAvailablePhysicalMemory();
    double GetMemoryUsagePercent();

    // Disk
    std::vector<DiskUsage> GetDiskUsage();
    DiskHealth GetDiskHealth(const wchar_t* driveLetter);

    // Network
    NetworkUsage GetNetworkUsage();

    // 启动计时器
    void StartMonitoring(int intervalMs = 1000);
    void StopMonitoring();

private:
    std::atomic<bool> m_running{false};
    std::jthread m_workerThread;
};
```

### 4.2 HardwareMonitorPanel 设计

| 区域 | 控件 | 说明 |
|------|------|------|
| 顶部 | 硬件摘要卡片 | CPU+GPU+内存+磁盘概览 |
| 中部 | 性能图表区 | 4 张标签页：CPU/内存/磁盘/网络 |
| 底部 | 健康报告 | 磁盘 SMART + 温度 |

### 4.3 UI 布局建议

```
┌─────────────────────────────────────┐
│ 硬件概览                             │
│ ┌──────┬──────┬──────┬──────┐      │
│ │CPU 45%│GPU 30%│内存 4GB│D 56% │      │
│ └──────┴──────┴──────┴──────┘      │
├─────────────────────────────────────┤
│ 性能监控 (标签页切换)               │
│ ┌─CPU─┬─内存─┬─磁盘─┬─网络─┐      │
│ │实时折线图                          │
│ │CPU 使用率: 45%                   │
│ │核心: 4 | 频率: 2800 MHz          │
│ └───────────────────────────────┘      │
├─────────────────────────────────────┤
│ 健康报告                             │
│ 磁盘 C: 健康 | 温度: 45°C           │
└─────────────────────────────────────┘
```

---

## 5. 实施步骤

### 阶段一：后端实现 (2-3 天)

| 步骤 | 任务 | 文件 |
|------|------|------|
| 1 | 编写 PerformanceMonitor 类 | core/analyzer/PerformanceMonitor.{h/cpp} |
| 2 | 实现 CPU 监控 | PerformanceMonitor.cpp |
| 3 | 实现内存监控 | PerformanceMonitor.cpp |
| 4 | 实现磁盘/网络监控 | PerformanceMonitor.cpp |
| 5 | 添加硬件健康检查 | core/analyzer/HardwareHealthCheck.h |

### 阶段二：前端实现 (2 天)

| 步骤 | 任务 | 文件 |
|------|------|------|
| 1 | 创建 PerformanceChart 控件 | gui/controls/PerformanceChart.h/cpp |
| 2 | 设计 HardwareMonitorPanel | gui/panels/HardwareMonitorPanel.{h/cpp} |
| 3 | 设计性能图表布局 | HardwareMonitorPanel.cpp |
| 4 | 集成到主界面 | MainWindow.cpp |

### 阶段三：导航集成 (1 天)

| 步骤 | 任务 | 产出 |
|------|------|------|
| 1 | 添加导航项 | NavSidebar.cpp |
| 2 | 更新 NavPage 枚举 | MainWindow.h |
| 3 | 更新页面索引映射 | MainWindow.cpp |

### 阶段四：优化完善 (1 天)

| 步骤 | 任务 | 产出 |
|------|------|------|
| 1 | 添加主题支持 | ThemeManager.cpp |
| 2 | 优化资源占用 | 100ms 批量更新 |
| 3 | 添加动画效果 | 平滑过渡 |
| 4 | 本地化文本 | locale/zh_CN |

---

## 6. 实现要点

### 6.1 性能优化

```cpp
// 关键：避免频繁的系统调用
class PerformanceMonitor {
    struct LastMeasurement {
        FILETIME lastIdleTime;
        FILETIME lastKernelTime;
        FILETIME lastUserTime;
        MEMORYSTATUSEX lastMemStatus;
    };
    
    LastMeasurement m_lastCpu;
    LastMeasurement m_lastMem;
    std::chrono::steady_clock::time_point m_lastUpdate;
    
public:
    // 节流：1s 内只更新 1 次
    double GetCpuUsage() {
        auto now = std::chrono::steady_clock::now();
        if (now - m_lastUpdate < 100ms) {
            return m_lastCpuUsage;  // 返回缓存值
        }
        // ...实际计算...
    }
};
```

### 6.2 线程安全

- 使用 `std::atomic` 持续读取的指标
- 使用 `jthread` 自动联合析构
- 使用 `CallAfter` 跨线程更新 UI
- 定时器驱动 UI 刷新（而非扫描器）

### 6.3 内存管理

- 历史数据使用环形缓冲区
- 最大保留 600 个数据点（10 分钟）
- 使用 `shared_ptr<Payload>` 传递数据

---

## 7. 资源占用目标

| 项目 | 目标值 | 说明 |
|------|--------|------|
| 背景内存 | < 5 MB | 基础功能 |
| 监控线程 | < 1% CPU | 1 秒检查一次 |
| 图表刷新 | 500ms |  smoothness |
| 启动时间 | < 500ms | 硬件检测延迟加载 |

---

## 8. 风险评估

| 风险 | 影响 | 缓解措施 |
|------|------|---------|
| 频繁系统调用 | CPU 占用 | 1 秒节流 + 批量读取 |
| WMI 调用慢 | 启动延迟 | 异步加载 + 缓存 |
| 跨线程更新 | UI 卡顿 | 只更新 UI，数据在线程中 |
| SMART 访问失败 | 功能缺失 | 优雅降级为估算 |

---

## 9. 成功标准

- ✅ CPU 使用率显示准确（误差 < 5%）
- ✅ 内存占用显示准确（误差 < 100MB）
- ✅ 磁盘空间计算准确（误差 < 1%）
- ✅ UI 线程不阻塞（每帧 < 16ms）
- ✅ 左侧导航栏新增硬件监控项
- ✅ 实时折线图平滑显示
- ✅ 主题切换时图表颜色自动更新

---

## 10. 参考文献

1. **MS Docs** - Performance Information：GetSystemTimes, GlobalMemoryStatusEx
2. **WMI 参考** - Win32_Processor, Win32_PhysicalMemory, Win32_LogicalDisk
3. **SMART 标准** - Windows.Storage.Smart
4. **业界参考** - HWiNFO64 API, MSI Afterburner SDK

---

*制定日期：2026-09-02*
*预计实施周期：5-7 天*