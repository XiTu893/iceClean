#pragma once
#include <string>
#include <vector>

namespace IceClean::Core::Safety {

class WhitelistProvider {
public:
    // 检查路径是否在白名单中（不应被清理）
    static bool IsWhitelisted(const std::wstring& path);

    // 获取白名单路径列表
    static const std::vector<std::wstring>& GetWhitelist();

private:
    static std::vector<std::wstring> InitializeWhitelist();
    static std::vector<std::wstring> s_whitelist;
};

} // namespace IceClean::Core::Safety
