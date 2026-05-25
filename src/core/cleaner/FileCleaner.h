#pragma once
#include "CleanerBase.h"

namespace IceClean::Core::Cleaner {

class FileCleaner : public CleanerBase {
public:
    std::wstring GetName() const override { return L"文件清理器"; }

    Models::CleanResult Clean(const std::vector<std::wstring>& paths,
                               std::function<void(const Models::CleanProgress&)> progressCallback = nullptr) override;
};

} // namespace IceClean::Core::Cleaner
