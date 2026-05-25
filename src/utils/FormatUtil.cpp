#include "FormatUtil.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace IceClean::Utils {

std::wstring FormatUtil::FormatFileSize(uint64_t bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 4) {
        size /= 1024.0;
        ++unitIndex;
    }

    std::wostringstream oss;
    if (unitIndex == 0) {
        oss << bytes << L" " << units[0];
    } else {
        oss << std::fixed << std::setprecision(1) << size << L" " << units[unitIndex];
    }
    return oss.str();
}

std::wstring FormatUtil::FormatDuration(double milliseconds) {
    int totalSeconds = static_cast<int>(milliseconds / 1000.0);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    std::wostringstream oss;

    if (hours > 0) {
        oss << hours << L"小时";
    }
    if (minutes > 0) {
        oss << minutes << L"分";
    }
    if (seconds > 0 || (hours == 0 && minutes == 0)) {
        oss << seconds << L"秒";
    }

    return oss.str();
}

std::wstring FormatUtil::FormatPercent(uint64_t part, uint64_t total) {
    if (total == 0) return L"0%";

    double percent = (static_cast<double>(part) / static_cast<double>(total)) * 100.0;
    if (percent > 100.0) percent = 100.0;

    std::wostringstream oss;
    oss << std::fixed << std::setprecision(1) << percent << L"%";
    return oss.str();
}

std::wstring FormatUtil::FormatSpeed(uint64_t bytesPerSecond) {
    return FormatFileSize(bytesPerSecond) + L"/s";
}

} // namespace IceClean::Utils
