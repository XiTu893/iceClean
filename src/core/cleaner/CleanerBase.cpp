#include "CleanerBase.h"
#include "utils/FileUtil.h"

namespace IceClean::Core::Cleaner {

bool CleanerBase::IsWhitelisted(const std::wstring& path) const {
    return Safety::WhitelistProvider::IsWhitelisted(path);
}

bool CleanerBase::DeleteFile(const std::wstring& path) const {
    if (IsWhitelisted(path)) return false;
    return Utils::FileUtil::DeleteFilePermanently(path);
}

bool CleanerBase::DeleteDirectory(const std::wstring& path) const {
    if (IsWhitelisted(path)) return false;
    return Utils::FileUtil::DeleteFolder(path);
}

uint64_t CleanerBase::GetFileSize(const std::wstring& path) const {
    return Utils::FileUtil::GetFileSize(path);
}

} // namespace IceClean::Core::Cleaner
