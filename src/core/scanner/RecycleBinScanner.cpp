#include "RecycleBinScanner.h"
#include "utils/ShellUtil.h"
#include "utils/Win32Util.h"

namespace IceClean::Core::Scanner {

Models::ScanCategory RecycleBinScanner::Scan(const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    // 使用 Shell API 获取回收站大小
    uint64_t recycleBinSize = Utils::ShellUtil::GetRecycleBinSize();

    if (stopFlag && stopFlag->load()) return category;

    if (recycleBinSize > 0) {
        // 创建一个虚拟文件项来表示回收站内容
        Models::ScanFileItem item;
        item.path = L"$RECYCLE.BIN";
        item.size = recycleBinSize;
        item.selected = true;

        SYSTEMTIME st;
        GetSystemTime(&st);
        SystemTimeToFileTime(&st, &item.lastWriteTime);

        category.items.push_back(item);
        category.totalSize = recycleBinSize;
    }

    return category;
}

bool RecycleBinScanner::IsAvailable() const {
    // 回收站在所有Windows系统上都存在
    return true;
}

} // namespace IceClean::Core::Scanner
