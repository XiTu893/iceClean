#pragma once
#include <string>
#include <vector>
#include <windows.h>

namespace IceClean::Utils {

class RegistryUtil {
public:
    // 读取注册表字符串值
    static std::wstring ReadStringValue(HKEY rootKey, const std::wstring& subKey,
                                        const std::wstring& valueName);

    // 读取注册表DWORD值
    static DWORD ReadDwordValue(HKEY rootKey, const std::wstring& subKey,
                                const std::wstring& valueName);

    // 写入注册表字符串值
    static bool WriteStringValue(HKEY rootKey, const std::wstring& subKey,
                                 const std::wstring& valueName, const std::wstring& value);

    // 枚举注册表子键
    static std::vector<std::wstring> EnumSubKeys(HKEY rootKey, const std::wstring& subKey);

    // 枚举注册表值
    static std::vector<std::wstring> EnumValues(HKEY rootKey, const std::wstring& subKey);

    // 删除注册表值
    static bool DeleteValue(HKEY rootKey, const std::wstring& subKey,
                           const std::wstring& valueName);

    // 检查注册表键是否存在
    static bool KeyExists(HKEY rootKey, const std::wstring& subKey);
};

} // namespace IceClean::Utils
