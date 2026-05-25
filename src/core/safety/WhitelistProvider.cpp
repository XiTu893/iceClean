#include "WhitelistProvider.h"
#include "utils/Win32Util.h"
#include <algorithm>
#include <cwctype>

namespace IceClean::Core::Safety {

std::vector<std::wstring> WhitelistProvider::s_whitelist = WhitelistProvider::InitializeWhitelist();

std::vector<std::wstring> WhitelistProvider::InitializeWhitelist() {
    std::vector<std::wstring> list = {
        L"C:\\Windows\\System32",
        L"C:\\Windows\\SysWOW64",
        L"C:\\Windows\\WinSxS",
        L"C:\\Windows\\System",
        L"C:\\Program Files",
        L"C:\\Program Files (x86)",
        L"C:\\ProgramData",
        L"C:\\Windows\\Fonts",
        L"C:\\Windows\\Assembly",
        L"C:\\Windows\\Microsoft.NET",
        L"C:\\Windows\\Boot",
        L"C:\\Windows\\Branding",
        L"C:\\Windows\\Debug",
        L"C:\\Windows\\Diagnostic",
        L"C:\\Windows\\DigitalLocker",
        L"C:\\Windows\\Ehome",
        L"C:\\Windows\\EHome",
        L"C:\\Windows\\Explorer",
        L"C:\\Windows\\ImmersiveControlPanel",
        L"C:\\Windows\\InputMethod",
        L"C:\\Windows\\L2Schemas",
        L"C:\\Windows\\LiveKernelReports",
        L"C:\\Windows\\Media",
        L"C:\\Windows\\ModemLogs",
        L"C:\\Windows\\Offline Web Pages",
        L"C:\\Windows\\Panther",
        L"C:\\Windows\\Performance",
        L"C:\\Windows\\PLA",
        L"C:\\Windows\\PolicyDefinitions",
        L"C:\\Windows\\Provisioning",
        L"C:\\Windows\\Registration",
        L"C:\\Windows\\RemotePackages",
        L"C:\\Windows\\Resources",
        L"C:\\Windows\\SchCache",
        L"C:\\Windows\\schemas",
        L"C:\\Windows\\Security",
        L"C:\\Windows\\ServiceProfiles",
        L"C:\\Windows\\Servicing",
        L"C:\\Windows\\Setup",
        L"C:\\Windows\\ShellComponents",
        L"C:\\Windows\\ShellExperiences",
        L"C:\\Windows\\SoftwareDistribution",  // 整体不删，只清理子目录
        L"C:\\Windows\\Speech",
        L"C:\\Windows\\SystemApps",
        L"C:\\Windows\\SystemResources",
        L"C:\\Windows\\TAPI",
        L"C:\\Windows\\Temp\\~DFA",  // 某些系统临时文件不应删除
        L"C:\\Windows\\twain_32",
        L"C:\\Windows\\Vss",
        L"C:\\Windows\\Web",
        L"C:\\Windows\\WinSxS",
        L"C:\\Windows\\winsxs",
    };
    return list;
}

const std::vector<std::wstring>& WhitelistProvider::GetWhitelist() {
    return s_whitelist;
}

static std::wstring ToLower(const std::wstring& s) {
    std::wstring result = s;
    std::transform(result.begin(), result.end(), result.begin(), std::towlower);
    return result;
}

bool WhitelistProvider::IsWhitelisted(const std::wstring& path) {
    if (path.empty()) return false;

    // 展开环境变量
    std::wstring expandedPath = Utils::Win32Util::ExpandEnvVars(path);
    std::wstring lowerPath = ToLower(expandedPath);

    // 去除末尾反斜杠以统一比较
    while (!lowerPath.empty() && lowerPath.back() == L'\\') {
        lowerPath.pop_back();
    }

    for (const auto& whitelisted : s_whitelist) {
        std::wstring lowerWhitelisted = ToLower(whitelisted);
        while (!lowerWhitelisted.empty() && lowerWhitelisted.back() == L'\\') {
            lowerWhitelisted.pop_back();
        }

        // 路径完全匹配或路径是白名单路径的子路径
        if (lowerPath == lowerWhitelisted) return true;
        if (lowerPath.length() > lowerWhitelisted.length() &&
            lowerPath.compare(0, lowerWhitelisted.length(), lowerWhitelisted) == 0 &&
            lowerPath[lowerWhitelisted.length()] == L'\\') {
            return true;
        }
    }

    return false;
}

} // namespace IceClean::Core::Safety
