#pragma once
#include "CleanerBase.h"

namespace IceClean::Core::Cleaner {

class HibernationCleaner : public CleanerBase {
public:
    std::wstring GetName() const override { return L"休眠文件清理器"; }

    Models::CleanResult Clean(const std::vector<std::wstring>& paths,
                               std::function<void(const Models::CleanProgress&)> progressCallback = nullptr) override;
};

} // namespace IceClean::Core::Cleaner
