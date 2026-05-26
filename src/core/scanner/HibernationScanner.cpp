#include "HibernationScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Scanner {

Models::ScanCategory HibernationScanner::Scan(const std::atomic<bool>* stopFlag, ScanProgressCallback progressCb) {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    // 谨慎项默认不选中
    category.selected = false;

    // 检查 C:\hiberfil.sys 是否存在
    std::wstring hiberfilPath = L"C:\\hiberfil.sys";

    if (stopFlag && stopFlag->load()) return category;

    // hiberfil.sys 是系统隐藏文件，需要特殊方式获取大小
    // 使用 GetCompressedFileSizeW 可以获取压缩文件大小
    DWORD highSize = 0;
    DWORD lowSize = GetCompressedFileSizeW(hiberfilPath.c_str(), &highSize);

    if (lowSize != INVALID_FILE_SIZE || GetLastError() == NO_ERROR) {
        ULARGE_INTEGER fileSize;
        fileSize.LowPart = lowSize;
        fileSize.HighPart = highSize;

        if (fileSize.QuadPart > 0) {
            Models::ScanFileItem item;
            item.path = hiberfilPath;
            item.size = fileSize.QuadPart;
            item.selected = false;  // 谨慎项默认不选中

            WIN32_FILE_ATTRIBUTE_DATA attrData{};
            if (GetFileAttributesExW(hiberfilPath.c_str(), GetFileExInfoStandard, &attrData)) {
                item.lastWriteTime = attrData.ftLastWriteTime;
            } else {
                SYSTEMTIME st;
                GetSystemTime(&st);
                SystemTimeToFileTime(&st, &item.lastWriteTime);
            }

            category.items.push_back(item);
            category.totalSize = fileSize.QuadPart;
        }
    }

    return category;
}

bool HibernationScanner::IsAvailable() const {
    // 检查休眠功能是否启用（hiberfil.sys 是否存在）
    DWORD highSize = 0;
    DWORD lowSize = GetCompressedFileSizeW(L"C:\\hiberfil.sys", &highSize);

    if (lowSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        return false;
    }

    ULARGE_INTEGER fileSize;
    fileSize.LowPart = lowSize;
    fileSize.HighPart = highSize;
    return fileSize.QuadPart > 0;
}

} // namespace IceClean::Core::Scanner
