#pragma once
#include <string>
#include <vector>
#include <chrono>

namespace IceClean::Core::Safety {

class RestorePointManager {
public:
    // 创建系统还原点
    // description: 还原点描述
    // 返回是否成功
    static bool CreateRestorePoint(const std::wstring& description);

    // 检查系统还原是否可用
    static bool IsSystemRestoreAvailable();
};

} // namespace IceClean::Core::Safety
