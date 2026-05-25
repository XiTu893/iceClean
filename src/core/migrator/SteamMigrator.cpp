#include "SteamMigrator.h"
#include "utils/RegistryUtil.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace IceClean::Core::Migrator {

std::wstring SteamMigrator::GetName() const {
    return L"Steam游戏迁移";
}

Models::MigrationType SteamMigrator::GetMigrationType() const {
    return Models::MigrationType::SteamGame;
}

std::wstring SteamMigrator::GetSteamInstallPath() const {
    // 尝试从WOW6432Node读取(64位系统上的32位Steam)
    std::wstring path = Utils::RegistryUtil::ReadStringValue(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\WOW6432Node\\Valve\\Steam",
        L"InstallPath");

    if (!path.empty()) return path;

    // 尝试从32位注册表读取
    path = Utils::RegistryUtil::ReadStringValue(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Valve\\Steam",
        L"InstallPath");

    return path;
}

std::vector<std::wstring> SteamMigrator::ParseLibraryFolders(const std::wstring& steamPath) const {
    std::vector<std::wstring> libraries;

    // Steam安装目录本身也是一个库
    libraries.push_back(steamPath);

    // 读取libraryfolders.vdf
    std::wstring vdfPath = steamPath;
    if (vdfPath.back() != L'\\') vdfPath += L'\\';
    vdfPath += L"steamapps\\libraryfolders.vdf";

    std::ifstream file(vdfPath);
    if (!file.is_open()) return libraries;

    std::string line;
    while (std::getline(file, line)) {
        // 查找 "path" 行
        size_t pathPos = line.find("\"path\"");
        if (pathPos == std::string::npos) {
            // 也尝试查找 "Path"
            pathPos = line.find("\"Path\"");
        }
        if (pathPos != std::string::npos) {
            // 提取路径值
            size_t firstQuote = line.find('\"', pathPos + 6);
            if (firstQuote != std::string::npos) {
                size_t secondQuote = line.find('\"', firstQuote + 1);
                if (secondQuote != std::string::npos) {
                    std::string pathStr = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                    // 替换双反斜杠为单反斜杠
                    std::string cleaned;
                    for (size_t i = 0; i < pathStr.size(); ++i) {
                        if (pathStr[i] == '\\' && i + 1 < pathStr.size() && pathStr[i + 1] == '\\') {
                            cleaned += '\\';
                            ++i;
                        } else {
                            cleaned += pathStr[i];
                        }
                    }
                    std::wstring wpath(cleaned.begin(), cleaned.end());
                    // 避免重复添加
                    if (std::find(libraries.begin(), libraries.end(), wpath) == libraries.end()) {
                        libraries.push_back(wpath);
                    }
                }
            }
        }
    }

    return libraries;
}

std::wstring SteamMigrator::ExtractQuotedValue(const std::string& line, const std::string& key) const {
    size_t keyPos = line.find(key);
    if (keyPos == std::string::npos) return L"";

    size_t firstQuote = line.find('\"', keyPos + key.size());
    if (firstQuote == std::string::npos) return L"";

    size_t secondQuote = line.find('\"', firstQuote + 1);
    if (secondQuote == std::string::npos) return L"";

    std::string value = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
    return std::wstring(value.begin(), value.end());
}

SteamGameInfo SteamMigrator::ParseAppManifest(const std::wstring& manifestPath, const std::wstring& libraryPath) const {
    SteamGameInfo info;
    info.libraryPath = libraryPath;
    info.size = 0;

    std::ifstream file(manifestPath);
    if (!file.is_open()) return info;

    std::string line;
    while (std::getline(file, line)) {
        // 提取appid
        if (info.appId.empty()) {
            std::wstring appId = ExtractQuotedValue(line, "\"appid\"");
            if (!appId.empty()) {
                info.appId = appId;
            }
        }

        // 提取游戏名称
        std::wstring name = ExtractQuotedValue(line, "\"name\"");
        if (!name.empty() && line.find("\"name\"") != std::string::npos) {
            info.name = name;
        }

        // 提取安装目录
        std::wstring installDir = ExtractQuotedValue(line, "\"installdir\"");
        if (!installDir.empty() && line.find("\"installdir\"") != std::string::npos) {
            info.installDir = installDir;
        }

        // 提取大小(字节数)
        size_t sizePos = line.find("\"SizeOnDisk\"");
        if (sizePos != std::string::npos) {
            // SizeOnDisk的值可能不带引号
            size_t eqPos = line.find_first_not_of(" \t", sizePos + 12);
            if (eqPos != std::string::npos) {
                std::string sizeStr;
                while (eqPos < line.size() && (isdigit(line[eqPos]) || line[eqPos] == '\"')) {
                    if (isdigit(line[eqPos])) sizeStr += line[eqPos];
                    ++eqPos;
                }
                if (!sizeStr.empty()) {
                    try {
                        info.size = std::stoull(sizeStr);
                    } catch (...) {
                        info.size = 0;
                    }
                }
            }
        }
    }

    // 如果没有从manifest获取到大小，尝试从磁盘计算
    if (info.size == 0 && !info.installDir.empty()) {
        std::wstring gamePath = libraryPath;
        if (gamePath.back() != L'\\') gamePath += L'\\';
        gamePath += L"steamapps\\common\\";
        gamePath += info.installDir;
        info.size = Utils::FileUtil::GetFolderSize(gamePath);
    }

    return info;
}

bool SteamMigrator::IsSteamRunning() const {
    return IsProcessRunning(L"steam.exe");
}

std::vector<Models::MigrationItem> SteamMigrator::Detect() {
    std::vector<Models::MigrationItem> items;

    std::wstring steamPath = GetSteamInstallPath();
    if (steamPath.empty()) return items;

    auto libraries = ParseLibraryFolders(steamPath);

    std::wstring systemDrive = Utils::Win32Util::GetSystemDrive();

    for (const auto& libraryPath : libraries) {
        // 只检测C盘上的Steam库中的游戏
        if (libraryPath.size() < 2 || towlower(libraryPath[0]) != towlower(systemDrive[0])) {
            continue;
        }

        std::wstring manifestDir = libraryPath;
        if (manifestDir.back() != L'\\') manifestDir += L'\\';
        manifestDir += L"steamapps\\";

        if (!Utils::FileUtil::Exists(manifestDir)) continue;

        // 查找所有appmanifest_*.acf文件
        std::vector<std::wstring> manifests;
        Utils::FileUtil::ScanFiles(manifestDir, L"appmanifest_*.acf", manifests, false);

        for (const auto& manifestPath : manifests) {
            auto gameInfo = ParseAppManifest(manifestPath, libraryPath);

            if (gameInfo.name.empty() || gameInfo.installDir.empty()) continue;

            // 构建游戏完整路径
            std::wstring gamePath = libraryPath;
            if (gamePath.back() != L'\\') gamePath += L'\\';
            gamePath += L"steamapps\\common\\";
            gamePath += gameInfo.installDir;

            if (!Utils::FileUtil::Exists(gamePath)) continue;

            // 检查是否已经是联接链接
            if (Utils::JunctionPoint::IsJunction(gamePath)) continue;

            // 如果没有从manifest获取到大小，从磁盘计算
            if (gameInfo.size == 0) {
                gameInfo.size = Utils::FileUtil::GetFolderSize(gamePath);
            }

            Models::MigrationItem item;
            item.name = gameInfo.name;
            item.sourcePath = gamePath;
            item.size = gameInfo.size;
            item.type = Models::MigrationType::SteamGame;
            item.advice = gameInfo.size > 1024ULL * 1024 * 1024
                ? Models::MigrationAdvice::Recommended
                : Models::MigrationAdvice::Possible;
            item.selected = false;
            item.migrated = false;

            items.push_back(item);
        }
    }

    return items;
}

Models::MigrationResult SteamMigrator::Migrate(const std::vector<Models::MigrationItem>& items,
                                                 const std::wstring& targetDrive,
                                                 std::function<void(const Models::MigrationProgress&)> progressCallback) {
    Models::MigrationResult result;

    if (IsSteamRunning()) {
        result.errorMessage = L"Steam正在运行，请先关闭Steam后再迁移";
        return result;
    }

    if (items.empty()) {
        result.errorMessage = L"没有需要迁移的项目";
        return result;
    }

    // 检查目标驱动器空间
    uint64_t driveTotal = 0, driveFree = 0;
    if (Utils::Win32Util::GetDiskSpace(targetDrive, driveTotal, driveFree)) {
        uint64_t totalNeeded = 0;
        for (const auto& item : items) {
            if (item.selected) totalNeeded += item.size;
        }
        if (driveFree < totalNeeded) {
            result.errorMessage = L"目标驱动器空间不足";
            return result;
        }
    }

    int totalItems = static_cast<int>(items.size());
    int migratedCount = 0;
    int failedCount = 0;
    uint64_t totalMigratedSize = 0;

    for (int i = 0; i < totalItems; ++i) {
        if (!items[i].selected) continue;

        if (progressCallback) {
            Models::MigrationProgress progress;
            progress.currentItem = i + 1;
            progress.totalItems = totalItems;
            progress.migratedSize = totalMigratedSize;
            progress.totalSize = items[i].size;
            progress.currentFile = items[i].sourcePath;
            progress.isRunning = true;
            progressCallback(progress);
        }

        bool success = MoveAndCreateJunction(items[i].sourcePath, targetDrive,
                                              progressCallback, i + 1, totalItems);

        if (success) {
            ++migratedCount;
            totalMigratedSize += items[i].size;
        } else {
            ++failedCount;
        }
    }

    result.success = failedCount == 0;
    result.migratedCount = migratedCount;
    result.failedCount = failedCount;
    result.totalMigratedSize = totalMigratedSize;

    if (failedCount > 0 && migratedCount == 0) {
        result.errorMessage = L"所有迁移项目均失败";
    }

    return result;
}

} // namespace IceClean::Core::Migrator
