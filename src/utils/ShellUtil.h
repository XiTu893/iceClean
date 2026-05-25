#pragma once
#include <string>
#include <cstdint>

namespace IceClean::Utils {

class ShellUtil {
public:
    // 清空回收站
    static bool EmptyRecycleBin();

    // 获取回收站大小
    static uint64_t GetRecycleBinSize();

    // 在资源管理器中打开路径
    static bool OpenInExplorer(const std::wstring& path);
};

} // namespace IceClean::Utils
