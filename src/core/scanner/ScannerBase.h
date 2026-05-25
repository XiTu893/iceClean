#pragma once
#include "IScanner.h"
#include "utils/FileUtil.h"
#include "utils/Win32Util.h"
#include "core/safety/WhitelistProvider.h"
#include <windows.h>

namespace IceClean::Core::Scanner {

class ScannerBase : public IScanner {
public:
    // 计算文件项列表的总大小
    static uint64_t CalculateTotalSize(const std::vector<Models::ScanFileItem>& items);

    // 扫描目录并将文件添加到ScanCategory中
    // pattern: 通配符模式，如 L"*" 或 L"*.pf"
    // recursive: 是否递归扫描子目录
    // skipLocked: 是否跳过被锁定的文件
    void ScanDirectory(const std::wstring& directoryPath,
                       const std::wstring& pattern,
                       bool recursive,
                       bool skipLocked,
                       Models::ScanCategory& category);

    // 检查路径是否在白名单中
    bool IsWhitelisted(const std::wstring& path) const;

    // 检查文件是否被锁定（无法删除）
    static bool IsFileLocked(const std::wstring& path);
};

} // namespace IceClean::Core::Scanner
