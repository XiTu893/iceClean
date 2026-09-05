#include "JsonUtil.h"
#include "Win32Util.h"
#include "FileUtil.h"
#include <fstream>
#include <filesystem>
#include <shlobj.h>
#include <windows.h>

namespace IceClean::Utils {

namespace {
    std::wstring GetDataDir() {
        wchar_t exePath[MAX_PATH] = {};
        DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
        if (len == 0) return L"";

        std::filesystem::path p(exePath);
        std::filesystem::path parent = p.parent_path();
        std::filesystem::path dataDir = parent / L"data";

        std::error_code ec;
        std::filesystem::create_directories(dataDir, ec);
        return dataDir.wstring();
    }
}

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
    std::wstring dataDir = GetDataDir();
    if (dataDir.empty()) return L"";

    std::filesystem::path configDir = std::filesystem::path(dataDir) / L"config";
    std::error_code ec;
    std::filesystem::create_directories(configDir, ec);

    return (configDir / L"config.json").wstring();
}

std::wstring JsonUtil::GetLogPath() {
    std::wstring dataDir = GetDataDir();
    if (dataDir.empty()) return L"";

    std::filesystem::path logDir = std::filesystem::path(dataDir) / L"logs";
    std::error_code ec;
    std::filesystem::create_directories(logDir, ec);

    return logDir.wstring();
}

std::wstring JsonUtil::Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                static_cast<int>(utf8.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring wide(n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                        static_cast<int>(utf8.size()), wide.data(), n);
    return wide;
}

std::string JsonUtil::WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string utf8(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                        static_cast<int>(wide.size()), utf8.data(), n, nullptr, nullptr);
    return utf8;
}

} // namespace IceClean::Utils
