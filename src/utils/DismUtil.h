#pragma once
#include <string>
#include <functional>

namespace IceClean::Utils {

class DismUtil {
public:
    // WinSxS组件清理
    static bool StartComponentCleanup(bool resetBase = false,
                                      std::function<void(const std::wstring&)> outputCallback = nullptr);

    // CompactOS压缩
    static bool CompactOS(std::function<void(const std::wstring&)> outputCallback = nullptr);

    // 执行DISM命令并获取输出
    static bool ExecuteDism(const std::wstring& arguments,
                           std::function<void(const std::wstring&)> outputCallback = nullptr);
};

} // namespace IceClean::Utils
