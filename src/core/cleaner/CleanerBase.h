#pragma once
#include "ICleaner.h"
#include "utils/FileUtil.h"
#include "core/safety/WhitelistProvider.h"

namespace IceClean::Core::Cleaner {

class CleanerBase : public ICleaner {
public:
    // 检查路径是否在白名单中（不应被清理）
    bool IsWhitelisted(const std::wstring& path) const;

    // 删除单个文件（永久删除）
    bool DeleteFile(const std::wstring& path) const;

    // 删除目录（递归）
    bool DeleteDirectory(const std::wstring& path) const;

    // 获取文件大小
    uint64_t GetFileSize(const std::wstring& path) const;
};

} // namespace IceClean::Core::Cleaner
