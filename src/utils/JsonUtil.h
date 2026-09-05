#pragma once
#include <string>
#include <nlohmann/json.hpp>

namespace IceClean::Utils {

class JsonUtil {
public:
    // 读取JSON配置文件
    static nlohmann::json LoadJson(const std::wstring& filePath);

    // 保存JSON配置文件
    static bool SaveJson(const std::wstring& filePath, const nlohmann::json& data);

    // 获取配置文件路径(在 exe 同级 data/config 目录下)
    static std::wstring GetConfigPath();

    // 获取操作日志路径
    static std::wstring GetLogPath();

    // ── 编码转换（nlohmann/json 为 UTF-8，Windows 宽字符为 UTF-16，边界必经）──
    static std::wstring Utf8ToWide(const std::string& utf8);
    static std::string WideToUtf8(const std::wstring& wide);
};

} // namespace IceClean::Utils
