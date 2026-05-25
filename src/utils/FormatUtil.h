#pragma once
#include <string>
#include <cstdint>

namespace IceClean::Utils {

class FormatUtil {
public:
    // 格式化文件大小(如 "1.5 GB", "256.3 MB")
    static std::wstring FormatFileSize(uint64_t bytes);

    // 格式化持续时间(如 "2分30秒")
    static std::wstring FormatDuration(double milliseconds);

    // 格式化百分比
    static std::wstring FormatPercent(uint64_t part, uint64_t total);

    // 格式化速度(如 "125.6 MB/s")
    static std::wstring FormatSpeed(uint64_t bytesPerSecond);
};

} // namespace IceClean::Utils
