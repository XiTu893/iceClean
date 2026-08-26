#pragma once
#include <string>
#include <vector>
#include <functional>
#include <windows.h>
#include "core/driver/DeviceDriverInfo.h"

namespace IceClean::Core::Driver {

// pnputil 封装：负责驱动包的枚举、导出（备份）、导入（还原）。
// 使用系统绝对路径 %WINDIR%\System32\pnputil.exe，隐藏窗口执行并捕获输出。
class PnpUtilRunner {
public:
    // 枚举 DriverStore 中全部第三方驱动包
    static std::vector<DriverPackageInfo> EnumDrivers();

    // 一键导出全部第三方驱动包到 destDir（destDir 需已存在）
    // progress: currentFileCount / estimatedTotal / 当前文件名
    static bool ExportAll(const std::wstring& destDir,
                          std::function<void(int, int, const std::wstring&)> progress = nullptr);

    // 导出单个驱动包（按发布名称 oemXX.inf）
    static bool ExportOne(const std::wstring& oemInf, const std::wstring& destDir);

    // 从目录导入并安装全部 INF（递归）。返回 pnputil 进程退出码（0 表示成功）
    static int AddDrivers(const std::wstring& sourceDir);

private:
    // 取得 pnputil 绝对路径
    static std::wstring GetPnpUtilPath();

    // 隐藏窗口执行命令，捕获 OEM 编码输出，转换为宽字符
    static bool Run(const std::wstring& args, std::wstring& output, DWORD timeoutMs = 120000);

    // 解析 pnputil /enum-drivers 文本输出为结构化列表
    static std::vector<DriverPackageInfo> ParseEnumOutput(const std::wstring& output);
};

} // namespace IceClean::Core::Driver
