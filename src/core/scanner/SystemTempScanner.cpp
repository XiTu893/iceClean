#include "SystemTempScanner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <shlobj.h>

namespace IceClean::Core::Scanner {

Models::ScanCategory SystemTempScanner::Scan() {
    Models::ScanCategory category;
    category.name = GetName();
    category.description = GetDescription();
    category.safety = GetSafetyRating();
    category.icon = GetIcon();
    category.selected = true;

    // 扫描用户临时文件夹 %TEMP%
    std::wstring userTemp = Utils::Win32Util::ExpandEnvVars(L"%TEMP%");
    if (Utils::FileUtil::Exists(userTemp)) {
        ScanDirectory(userTemp, L"*", true, true, category);
    }

    // 扫描系统临时文件夹 C:\Windows\Temp
    std::wstring sysTemp = Utils::Win32Util::ExpandEnvVars(L"%SystemRoot%\\Temp");
    if (Utils::FileUtil::Exists(sysTemp)) {
        ScanDirectory(sysTemp, L"*", true, true, category);
    }

    return category;
}

bool SystemTempScanner::IsAvailable() const {
    // 临时文件夹在所有Windows系统上都存在
    return true;
}

} // namespace IceClean::Core::Scanner
