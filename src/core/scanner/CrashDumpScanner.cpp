#include "CrashDumpScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Scanner {

Models::ScanCategory CrashDumpScanner::Scan() {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    // 扫描 C:\Windows\Minidump
    std::wstring minidumpPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\Minidump");
    if (Utils::FileUtil::Exists(minidumpPath)) {
        ScanDirectory(minidumpPath, L"*.dmp", false, false, category);
    }

    // 扫描 C:\Windows\MEMORY.DMP
    std::wstring memoryDmp = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\MEMORY.DMP");
    if (Utils::FileUtil::Exists(memoryDmp)) {
        Models::ScanFileItem item;
        item.path = memoryDmp;
        item.size = Utils::FileUtil::GetFileSize(memoryDmp);

        WIN32_FILE_ATTRIBUTE_DATA attrData{};
        if (GetFileAttributesExW(memoryDmp.c_str(), GetFileExInfoStandard, &attrData)) {
            item.lastWriteTime = attrData.ftLastWriteTime;
        }

        item.selected = true;
        category.items.push_back(item);
        category.totalSize += item.size;
    }

    // 扫描 C:\Windows\LiveKernelReports
    std::wstring liveKernelPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\LiveKernelReports");
    if (Utils::FileUtil::Exists(liveKernelPath)) {
        ScanDirectory(liveKernelPath, L"*.dmp", true, false, category);
    }

    return category;
}

bool CrashDumpScanner::IsAvailable() const {
    // 至少有一个崩溃转储目录或文件存在
    std::wstring minidumpPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\Minidump");
    std::wstring memoryDmp = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\MEMORY.DMP");
    std::wstring liveKernelPath = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\LiveKernelReports");

    return Utils::FileUtil::Exists(minidumpPath) ||
           Utils::FileUtil::Exists(memoryDmp) ||
           Utils::FileUtil::Exists(liveKernelPath);
}

} // namespace IceClean::Core::Scanner
