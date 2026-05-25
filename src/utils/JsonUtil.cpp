#include "JsonUtil.h"
#include "Win32Util.h"
#include "FileUtil.h"
#include <fstream>
#include <filesystem>
#include <shlobj.h>

namespace IceClean::Utils {

nlohmann::json JsonUtil::LoadJson(const std::wstring& filePath) {
    try {
        // 使用UTF-8模式打开文件
        std::ifstream file(filePath, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            return nlohmann::json::object();
        }

        nlohmann::json data;
        file >> data;
        return data;
    } catch (const nlohmann::json::exception&) {
        return nlohmann::json::object();
    } catch (const std::exception&) {
        return nlohmann::json::object();
    }
}

bool JsonUtil::SaveJson(const std::wstring& filePath, const nlohmann::json& data) {
    try {
        // 确保目录存在
        std::filesystem::path p(filePath);
        std::filesystem::path parentDir = p.parent_path();
        if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
            std::filesystem::create_directories(parentDir);
        }

        // 写入文件(4空格缩进, UTF-8编码)
        std::ofstream file(filePath, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;

        file << data.dump(4, ' ', true);
        return file.good();
    } catch (const std::exception&) {
        return false;
    }
}

std::wstring JsonUtil::GetConfigPath() {
    // 在Roaming AppData下创建IceClean目录
    std::wstring appData = Win32Util::GetSpecialFolder(CSIDL_APPDATA);
    if (appData.empty()) return L"";

    std::wstring configDir = appData + L"\\IceClean";
    if (!FileUtil::Exists(configDir)) {
        FileUtil::CreateDirectoryRecursive(configDir);
    }

    return configDir + L"\\config.json";
}

std::wstring JsonUtil::GetLogPath() {
    // 在Local AppData下创建IceClean日志目录
    std::wstring localAppData = Win32Util::GetSpecialFolder(CSIDL_LOCAL_APPDATA);
    if (localAppData.empty()) return L"";

    std::wstring logDir = localAppData + L"\\IceClean\\Logs";
    if (!FileUtil::Exists(logDir)) {
        FileUtil::CreateDirectoryRecursive(logDir);
    }

    return logDir;
}

} // namespace IceClean::Utils
