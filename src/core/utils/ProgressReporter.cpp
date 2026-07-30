#include "ProgressReporter.h"
#include <algorithm>
#include <cmath>

namespace IceClean::Core::Utils {

void ProgressReporter::ReportProgress(int percent, const std::wstring& stage, const std::wstring& detail) {
    m_snapshot.percent = std::clamp(percent, 0, 100);
    if (!stage.empty()) m_snapshot.stage = stage;
    if (!detail.empty()) m_snapshot.detail = detail;
    m_stage = stage;
    Emit();
}

void ProgressReporter::ReportSubProgress(int subPercent, const std::wstring& subDetail) {
    m_snapshot.subPercent = std::clamp(subPercent, 0, 100);
    if (!subDetail.empty()) m_snapshot.subDetail = subDetail;
    Emit();
}

void ProgressReporter::ReportBytes(uint64_t processedBytes, uint64_t totalBytes) {
    m_snapshot.processedBytes = processedBytes;
    m_snapshot.totalBytes = totalBytes;

    if (totalBytes > 0) {
        m_snapshot.percent = std::clamp(
            static_cast<int>(processedBytes * 100 / totalBytes), 0, 100);
    }

    Emit();
}

void ProgressReporter::ReportItems(int processedItems, int totalItems) {
    m_snapshot.processedItems = processedItems;
    m_snapshot.totalItems = totalItems;

    if (totalItems > 0) {
        m_snapshot.percent = std::clamp(
            static_cast<int>(processedItems * 100 / totalItems), 0, 100);
    }

    Emit();
}

void ProgressReporter::Advance(int step) {
    m_currentStep += step;
    if (m_totalSteps > 0) {
        m_snapshot.percent = std::clamp(
            m_currentStep * 100 / m_totalSteps, 0, 100);
    }
    Emit();
}

void ProgressReporter::Reset() {
    m_snapshot = ProgressSnapshot{};
    m_cancelled.store(false);
    m_currentStep = 0;
    m_lastReportedBytes = 0;
    m_startTime = std::chrono::steady_clock::now();
    m_lastReportTime = {};
}

void ProgressReporter::Emit() {
    if (!m_callback) return;

    auto now = std::chrono::steady_clock::now();

    // 计算速度
    auto elapsed = std::chrono::duration<double>(now - m_startTime).count();
    if (elapsed > 0.0 && m_snapshot.processedBytes > 0) {
        m_snapshot.speedBytesPerSec = m_snapshot.processedBytes / elapsed;
    }

    // 节流：距上次发送不足 100ms 且进度未跳变 >5% 则跳过
    if (m_lastReportTime.time_since_epoch().count() > 0) {
        auto sinceLast = std::chrono::duration<double>(now - m_lastReportTime).count();
        if (sinceLast < 0.1 && std::abs(m_snapshot.percent - GetSnapshot().percent) < 5) {
            return;
        }
    }

    m_lastReportTime = now;
    m_callback(m_snapshot);
}

} // namespace IceClean::Core::Utils
