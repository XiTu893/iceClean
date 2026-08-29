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
    fwprintf(stderr, L"[cli] entered wmain, argc=%d\n", argc);
    fflush(stderr);
    uint64_t minSizeMB = 500;
    if (argc >= 2) {
        minSizeMB = _wtoi64(argv[1]);
    }
    wprintf(L"IceClean CLI 烟雾测试\n");
    wprintf(L"=====================\n");
    wprintf(L"最小大小阈值: %llu MB\n\n", static_cast<unsigned long long>(minSizeMB));
    fflush(stdout);

    try {
        fwprintf(stderr, L"[cli] step 1/5: LargeFolderDetector ctor\n"); fflush(stderr);
        IceClean::Core::Migrator::LargeFolderDetector detector(minSizeMB);
        fwprintf(stderr, L"[cli] step 1/5: ctor ok, calling Detect\n"); fflush(stderr);
        int progressCount = 0;
        auto items = detector.Detect(
            [&progressCount](const std::wstring& path, int found, uint64_t) {
                ++progressCount;
                if (progressCount % 50 == 0) {
                    wprintf(L"  progress #%d: %ls (found=%d)\n", progressCount, path.c_str(), found);
                }
            });
        fwprintf(stderr, L"[cli] step 1/5: Detect done, items=%zu progress=%d\n", items.size(), progressCount); fflush(stderr);
        wprintf(L"  -> 完成，进度回调 %d 次，找到 %zu 项\n\n", progressCount, items.size());
        for (const auto& it : items) PrintItem(it);

        fwprintf(stderr, L"[cli] step 2/5: WeChat\n"); fflush(stderr);
        IceClean::Core::Migrator::WeChatMigrator wechat;
        auto wechatItems = wechat.Detect();
        wprintf(L"[2/5] 微信 -> %zu 项\n", wechatItems.size());

        fwprintf(stderr, L"[cli] step 3/5: QQ\n"); fflush(stderr);
        IceClean::Core::Migrator::QQMigrator qq;
        auto qqItems = qq.Detect();
        wprintf(L"[3/5] QQ -> %zu 项\n", qqItems.size());

        fwprintf(stderr, L"[cli] step 4/5: Steam\n"); fflush(stderr);
        IceClean::Core::Migrator::SteamMigrator steam;
        auto steamItems = steam.Detect();
        wprintf(L"[4/5] Steam -> %zu 项\n", steamItems.size());

        fwprintf(stderr, L"[cli] step 5/5: UserFolder\n"); fflush(stderr);
        IceClean::Core::Migrator::UserFolderMigrator user;
        auto userItems = user.Detect();
        wprintf(L"[5/5] 用户文件夹 -> %zu 项\n", userItems.size());

        wprintf(L"\n=====================\n");
        wprintf(L"所有检测器完成。共 %zu 项可迁移。\n",
            items.size() + wechatItems.size() + qqItems.size()
            + steamItems.size() + userItems.size());
        return 0;
    } catch (const std::exception& e) {
        fwprintf(stderr, L"[cli] 异常: %hs\n", e.what());
        return 1;
    } catch (...) {
        fwprintf(stderr, L"[cli] 未知异常\n");
        return 2;
    }
}
