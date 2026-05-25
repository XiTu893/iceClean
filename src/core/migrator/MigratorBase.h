#pragma once
#include "IMigrator.h"
#include "utils/FileUtil.h"
#include "utils/JunctionPoint.h"
#include "utils/Win32Util.h"
#include "utils/FormatUtil.h"
#include <chrono>

namespace IceClean::Core::Migrator {

class MigratorBase : public IMigrator {
public:
    // 将文件夹移动到目标驱动器并创建联接链接
    // sourcePath: 源文件夹路径(C盘)
    // targetDrive: 目标驱动器(如 "D:\")
    // 返回是否成功
    bool MoveAndCreateJunction(const std::wstring& sourcePath, const std::wstring& targetDrive);

    // 带进度的文件夹移动+联接
    bool MoveAndCreateJunction(const std::wstring& sourcePath, const std::wstring& targetDrive,
                               std::function<void(const Models::MigrationProgress&)> progressCallback,
                               int currentItem, int totalItems);

    // 构建目标路径：将C盘路径替换为目标驱动器路径
    // 例如 "C:\Users\xxx\Documents" + "D:\" -> "D:\Users\xxx\Documents"
    std::wstring BuildTargetPath(const std::wstring& sourcePath, const std::wstring& targetDrive) const;

    // 检查源路径和目标路径是否在不同驱动器上
    bool IsOnDifferentDrive(const std::wstring& sourcePath, const std::wstring& targetDrive) const;

    // 检查指定进程是否正在运行
    bool IsProcessRunning(const std::wstring& processName) const;
};

} // namespace IceClean::Core::Migrator
