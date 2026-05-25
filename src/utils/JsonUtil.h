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

    // 获取配置文件路径(在AppData中)
    static std::wstring GetConfigPath();

    // 获取操作日志路径
    static std::wstring GetLogPath();
};

} // namespace IceClean::Utils
