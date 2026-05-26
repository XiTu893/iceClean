#include "ScannerAggregator.h"
#include "gui/Events.h"
#include "SystemTempScanner.h"
#include "WindowsUpdateScanner.h"
#include "BrowserCacheScanner.h"
#include "RecycleBinScanner.h"
#include "ThumbnailScanner.h"
#include "PrefetchScanner.h"
#include "LogScanner.h"
#include "CrashDumpScanner.h"
#include "DriverBackupScanner.h"
#include "HibernationScanner.h"
#include "WinSxSScanner.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>

namespace IceClean::Core::Scanner {

ScannerAggregator::ScannerAggregator() {
    RegisterBuiltinScanners();
}

void ScannerAggregator::RegisterBuiltinScanners() {
    m_scanners.clear();

    m_scanners.push_back(std::make_unique<SystemTempScanner>());
    m_scanners.push_back(std::make_unique<WindowsUpdateScanner>());
    m_scanners.push_back(std::make_unique<BrowserCacheScanner>());
    m_scanners.push_back(std::make_unique<RecycleBinScanner>());
    m_scanners.push_back(std::make_unique<ThumbnailScanner>());
    m_scanners.push_back(std::make_unique<PrefetchScanner>());
    m_scanners.push_back(std::make_unique<LogScanner>());
    m_scanners.push_back(std::make_unique<CrashDumpScanner>());
    m_scanners.push_back(std::make_unique<DriverBackupScanner>());
    m_scanners.push_back(std::make_unique<HibernationScanner>());
    m_scanners.push_back(std::make_unique<WinSxSScanner>());
}

Models::ScanResult ScannerAggregator::ScanAll(wxEvtHandler* evtHandler) {
    Models::ScanResult result;
    m_stopRequested = false;

    auto startTime = std::chrono::high_resolution_clock::now();

    // 筛选可用的扫描器
    std::vector<IScanner*> availableScanners;
    for (const auto& scanner : m_scanners) {
        if (scanner->IsAvailable()) {
            availableScanners.push_back(scanner.get());
        }
    }

    int totalScanners = static_cast<int>(availableScanners.size());
    if (totalScanners == 0) {
        result.scanDurationMs = 0;
        return result;
    }

    // 并行扫描
    std::mutex resultMutex;
    std::atomic<int> completedCount{0};

    // 预分配结果空间
    std::vector<Models::ScanCategory> categories(totalScanners);

    std::vector<std::thread> threads;
    threads.reserve(totalScanners);

    for (int i = 0; i < totalScanners; ++i) {
        threads.emplace_back([&, i]() {
            // 检查是否已请求停止
            if (m_stopRequested.load()) return;

            IScanner* scanner = availableScanners[i];

            // 发送进度事件 - 开始扫描
            if (evtHandler) {
                ScanProgressInfo progress;
                progress.completedScanners = completedCount.load();
                progress.totalScanners = totalScanners;
                progress.currentScanner = scanner->GetName();
                progress.isRunning = true;

                wxThreadEvent* event = new wxThreadEvent(IceClean::Gui::wxEVT_SCAN_PROGRESS_UPDATE);
                event->SetPayload(progress);
                wxQueueEvent(evtHandler, event);
            }

            // 执行扫描
            Models::ScanCategory category = scanner->Scan();

            // 保存结果
            {
                std::lock_guard<std::mutex> lock(resultMutex);
                categories[i] = std::move(category);
            }

            int completed = completedCount.fetch_add(1) + 1;

            // 发送进度事件 - 完成扫描
            if (evtHandler && !m_stopRequested.load()) {
                ScanProgressInfo progress;
                progress.completedScanners = completed;
                progress.totalScanners = totalScanners;
                progress.currentScanner = scanner->GetName();
                progress.isRunning = (completed < totalScanners);

                wxThreadEvent* event = new wxThreadEvent(IceClean::Gui::wxEVT_SCAN_PROGRESS_UPDATE);
                event->SetPayload(progress);
                wxQueueEvent(evtHandler, event);
            }
        });
    }

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    // 汇总已完成的结果
    for (auto& category : categories) {
        result.totalSize += category.totalSize;
        result.totalFileCount += static_cast<int>(category.items.size());

        for (const auto& item : category.items) {
            if (item.selected) {
                result.selectedSize += item.size;
                result.selectedFileCount++;
            }
        }

        result.categories.push_back(std::move(category));
    }

    result.wasStopped = m_stopRequested.load();

    auto endTime = std::chrono::high_resolution_clock::now();
    result.scanDurationMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    return result;
}

void ScannerAggregator::RequestStop() {
    m_stopRequested = true;
}

bool ScannerAggregator::IsStopRequested() const {
    return m_stopRequested.load();
}

const std::vector<std::unique_ptr<IScanner>>& ScannerAggregator::GetScanners() const {
    return m_scanners;
}

int ScannerAggregator::GetAvailableScannerCount() const {
    int count = 0;
    for (const auto& scanner : m_scanners) {
        if (scanner->IsAvailable()) {
            ++count;
        }
    }
    return count;
}

} // namespace IceClean::Core::Scanner
