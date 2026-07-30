#pragma once
#include <string>
#include <vector>

namespace IceClean::Models {

// 重复文件信息
struct DuplicateFileGroup {
    uint64_t fileSize = 0;              // 文件大小
    std::wstring fileHash;              // 文件哈希(SHA256)
    std::vector<std::wstring> filePaths; // 所有相同文件的路径
    uint64_t wastedSpace = 0;           // 浪费的空间(重复文件占用的额外空间)
};

// 重复文件扫描进度
struct DuplicateScanProgress {
    int scannedFiles = 0;       // 已扫描文件数
    int totalFiles = 0;        // 总文件数(预估)
    int duplicateGroups = 0;   // 已发现的重复组数
    std::wstring currentPath;  // 当前扫描路径
};

} // namespace IceClean::Models
