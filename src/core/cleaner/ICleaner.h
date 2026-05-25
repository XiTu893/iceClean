#pragma once
#include <string>
#include <vector>
#include <functional>
#include "models/CleanItem.h"

namespace IceClean::Core::Cleaner {

class ICleaner {
public:
    virtual ~ICleaner() = default;

    // Get cleaner name
    virtual std::wstring GetName() const = 0;

    // Clean the specified items
    virtual Models::CleanResult Clean(const std::vector<std::wstring>& paths,
                                       std::function<void(const Models::CleanProgress&)> progressCallback = nullptr) = 0;
};

} // namespace IceClean::Core::Cleaner
