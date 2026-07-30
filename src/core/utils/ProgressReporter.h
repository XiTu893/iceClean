#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <cstdint>

namespace IceClean::Core::Utils {

struct ProgressSnapshot {
    int percent = 0;
    std::wstring stage;
    std::wstring detail;
    int subPercent = 0;
    std::wstring subDetail;
    uint64_t processedBytes = 0;
    uint64_t totalBytes = 0;
    int processedItems = 0;
    int totalItems = 0;
    double speedBytesPerSec = 0.0;
};

class ProgressReporter {
public:
    using ProgressCallback = std::function<void(const ProgressSnapshot&)>;

    explicit ProgressReporter(ProgressCallback callback = nullptr)
        : m_callback(std::move(callback))
        , m_startTime(std::chrono::steady_clock::now()) {}

    void SetCallback(ProgressCallback cb) { m_callback = std::move(cb); }

    void ReportProgress(int percent, const std::wstring& stage, const std::wstring& detail = {});
    void ReportSubProgress(int subPercent, const std::wstring& subDetail = {});
    void ReportBytes(uint64_t processedBytes, uint64_t totalBytes);
    void ReportItems(int processedItems, int totalItems);

    void Advance(int step = 1);
    void SetTotalSteps(int total) { m_totalSteps = total; }
    void SetStage(const std::wstring& stage) { m_stage = stage; }

    bool IsCancelled() const { return m_cancelled.load(); }
    void Cancel() { m_cancelled.store(true); }
    void Reset();

    const ProgressSnapshot& GetSnapshot() const { return m_snapshot; }

private:
    void Emit();

    ProgressCallback m_callback;
    ProgressSnapshot m_snapshot;

    std::atomic<bool> m_cancelled{false};
    std::wstring m_stage;
    int m_currentStep = 0;
    int m_totalSteps = 100;

    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastReportTime;

    uint64_t m_lastReportedBytes = 0;
};

} // namespace IceClean::Core::Utils
