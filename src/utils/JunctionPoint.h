#pragma once
#include <string>

namespace IceClean::Utils {

class JunctionPoint {
public:
    // 创建目录联接(Junction)
    static bool Create(const std::wstring& junctionPath, const std::wstring& targetPath);

    // 删除目录联接
    static bool Remove(const std::wstring& junctionPath);

    // 检查路径是否为Junction
    static bool IsJunction(const std::wstring& path);

    // 获取Junction指向的目标路径
    static std::wstring GetTarget(const std::wstring& junctionPath);
};

} // namespace IceClean::Utils
