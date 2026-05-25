#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <mutex>
#include "IScanner.h"
#include "models/ScanResult.h"
#include "gui/Events.h"
#include <wx/event.h>

namespace IceClean::Core::Scanner {

// 扫描进度回调
struct ScanProgressInfo {
    int completedScanners = 0;   // 已完成的扫描器数量
    int totalScanners = 0;       // 总扫描器数量
    std::wstring currentScanner; // 当前正在运行的扫描器名称
    bool isRunning = false;      // 是否正在扫描
};

class ScannerAggregator {
public:
    ScannerAggregator();

    // 注册所有内置扫描器
    void RegisterBuiltinScanners();

    // 执行所有扫描（并行），返回完整扫描结果
    // wxEvtHandler 用于发送 wxThreadEvent 进度通知
    Models::ScanResult ScanAll(wxEvtHandler* evtHandler = nullptr);

    // 获取所有已注册的扫描器
    const std::vector<std::unique_ptr<IScanner>>& GetScanners() const;

    // 获取可用的扫描器数量
    int GetAvailableScannerCount() const;

private:
    std::vector<std::unique_ptr<IScanner>> m_scanners;
};

} // namespace IceClean::Core::Scanner
