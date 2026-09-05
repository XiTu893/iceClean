// CLI 烟雾测试 - 直接测试 LargeFolderDetector，不经过 GUI
// 目标：验证迁移扫描核心逻辑是否在 CLI 环境下工作
// 用法: IceCleanCliTest.exe [minSizeMB]
#include "core/migrator/LargeFolderDetector.h"
#include "core/migrator/WeChatMigrator.h"
#include "core/migrator/QQMigrator.h"
#include "core/migrator/SteamMigrator.h"
#include "core/migrator/UserFolderMigrator.h"
#include "core/migrator/DevCacheMigrator.h"
#include "core/migrator/FolderMigrator.h"

#include <cstdio>
#include <exception>

namespace {

const wchar_t* MigrationTypeStr(IceClean::Models::MigrationType t) {
    using T = IceClean::Models::MigrationType;
    switch (t) {
        case T::SteamGame:    return L"SteamGame";
        case T::UserFolder:   return L"UserFolder";
        case T::WeChatCache:  return L"WeChatCache";
        case T::QQCache:      return L"QQCache";
        case T::CustomFolder: return L"CustomFolder";
        case T::DevCache:     return L"DevCache";
        case T::LargeSoftware:return L"LargeSoftware";
    }
    return L"Unknown";
}

void PrintItem(const IceClean::Models::MigrationItem& item) {
    wprintf(L"  - [%ls] %ls (%llu bytes)\n",
        MigrationTypeStr(item.type),
        item.sourcePath.c_str(),
        static_cast<unsigned long long>(item.size));
}

} // namespace

int wmain(int argc, wchar_t* argv[]) {
    uint64_t minSizeMB = 100;
    if (argc >= 2) {
        minSizeMB = _wtoi64(argv[1]);
    }
    wprintf(L"IceClean CLI 烟雾测试 (阈值=%llu MB)\n", static_cast<unsigned long long>(minSizeMB));
    wprintf(L"=====================\n");
    fflush(stdout);

    try {
        IceClean::Core::Migrator::LargeFolderDetector detector(minSizeMB);
        auto items = detector.Detect(nullptr);
        wprintf(L"  -> LargeFolderDetector 完成，找到 %zu 项\n", items.size());
        for (const auto& it : items) {
            wprintf(L"    [%ls] %ls (%llu MB)\n",
                MigrationTypeStr(it.type),
                it.sourcePath.c_str(),
                static_cast<unsigned long long>(it.size / (1024 * 1024)));
        }

        IceClean::Core::Migrator::WeChatMigrator wechat;
        auto wechatItems = wechat.Detect();
        wprintf(L"[2] 微信 -> %zu 项\n", wechatItems.size());
        for (const auto& it : wechatItems) {
            wprintf(L"    [%ls] %ls (%llu MB)\n",
                MigrationTypeStr(it.type), it.sourcePath.c_str(),
                static_cast<unsigned long long>(it.size / (1024 * 1024)));
        }

        IceClean::Core::Migrator::QQMigrator qq;
        auto qqItems = qq.Detect();
        wprintf(L"[3] QQ -> %zu 项\n", qqItems.size());
        for (const auto& it : qqItems) {
            wprintf(L"    [%ls] %ls (%llu MB)\n",
                MigrationTypeStr(it.type), it.sourcePath.c_str(),
                static_cast<unsigned long long>(it.size / (1024 * 1024)));
        }

        IceClean::Core::Migrator::SteamMigrator steam;
        auto steamItems = steam.Detect();
        wprintf(L"[4] Steam -> %zu 项\n", steamItems.size());
        for (const auto& it : steamItems) {
            wprintf(L"    [%ls] %ls (%llu MB)\n",
                MigrationTypeStr(it.type), it.sourcePath.c_str(),
                static_cast<unsigned long long>(it.size / (1024 * 1024)));
        }

        IceClean::Core::Migrator::UserFolderMigrator user;
        auto userItems = user.Detect();
        wprintf(L"[5] 用户文件夹 -> %zu 项\n", userItems.size());
        for (const auto& it : userItems) {
            wprintf(L"    [%ls] %ls (%llu MB)\n",
                MigrationTypeStr(it.type), it.sourcePath.c_str(),
                static_cast<unsigned long long>(it.size / (1024 * 1024)));
        }

        wprintf(L"\n=====================\n");
        size_t total = items.size() + wechatItems.size() + qqItems.size()
                     + steamItems.size() + userItems.size();
        wprintf(L"总计 %zu 项可迁移。\n", total);
        return 0;
    } catch (const std::exception& e) {
        fwprintf(stderr, L"[cli] 异常: %hs\n", e.what());
        return 1;
    } catch (...) {
        fwprintf(stderr, L"[cli] 未知异常\n");
        return 2;
    }
}
