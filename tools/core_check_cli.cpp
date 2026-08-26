// core_check_cli —— IceClean 核心功能命令行验证工具（无 GUI 依赖）
//
// 用法：core_check_cli.exe
// 覆盖：注册表工具 / StartupApproved / 大文件夹检测（含父子去重）/ Junction 迁移往返 /
//       服务枚举（只读）/ 启动项读取（只读）
// 退出码 0 = 全部通过

#include <windows.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

#include "core/migrator/LargeFolderDetector.h"
#include "core/migrator/FolderMigrator.h"
#include "core/optimizer/ServiceOptimizer.h"
#include "core/optimizer/StartupOptimizer.h"
#include "core/safety/SoftwareRecommendFetcher.h"
#include "core/safety/SoftwareRecommendSeed.h"
#include "core/driver/DeviceDriverScanner.h"
#include "core/driver/PnpUtilRunner.h"
#include "utils/JunctionPoint.h"
#include "utils/RegistryUtil.h"

using namespace IceClean::Core::Migrator;
using namespace IceClean::Core::Optimizer;
using namespace IceClean::Models;
namespace Utils = IceClean::Utils;
using Utils::RegistryUtil;

int g_pass = 0, g_fail = 0;

void Check(bool ok, const char* what) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << what << "\n";
    if (ok) ++g_pass; else ++g_fail;
}

std::wstring TempRoot() {
    wchar_t buf[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, buf);
    return std::wstring(buf);
}

bool WriteDummy(const std::wstring& path, DWORD sizeMB) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    const DWORD chunk = 1024 * 1024;
    std::vector<char> zeros(chunk, 0);
    for (DWORD i = 0; i < sizeMB; ++i) {
        DWORD written = 0;
        if (!WriteFile(h, zeros.data(), chunk, &written, nullptr)) { CloseHandle(h); return false; }
    }
    CloseHandle(h);
    return true;
}

// ── 1. 注册表工具沙箱往返 ──
void TestRegistry() {
    std::cout << "\n-- RegistryUtil sandbox --\n";
    const std::wstring key = L"Software\\IceClean\\CliTests\\Reg";
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\IceClean\\CliTests");

    bool w = RegistryUtil::WriteStringValue(HKEY_CURRENT_USER, key, L"Value1", L"hello");
    Check(w && RegistryUtil::ReadStringValue(HKEY_CURRENT_USER, key, L"Value1") == L"hello",
          "write/read string value round-trip");
    Check(RegistryUtil::DeleteValue(HKEY_CURRENT_USER, key, L"Value1"),
          "delete value");
    Check(RegistryUtil::ReadStringValue(HKEY_CURRENT_USER, key, L"Value1").empty(),
          "deleted value reads empty");

    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\IceClean\\CliTests");
}

// ── 2. StartupApproved 机制 ──
void TestStartupApproved() {
    std::cout << "\n-- StartupApproved flags --\n";
    const std::wstring root = L"Software\\IceClean\\CliTests";
    const std::wstring approved = root + L"\\Approved";
    const std::wstring name = L"cli.test.item";
    RegDeleteTreeW(HKEY_CURRENT_USER, root.c_str());

    Check(!StartupOptimizer::IsStartupApprovedDisabled(HKEY_CURRENT_USER, approved, name),
          "absent flag means enabled");
    Check(StartupOptimizer::SetStartupApproved(HKEY_CURRENT_USER, approved, name, false),
          "write disable flag");
    Check(StartupOptimizer::IsStartupApprovedDisabled(HKEY_CURRENT_USER, approved, name),
          "flag marks disabled");
    Check(StartupOptimizer::SetStartupApproved(HKEY_CURRENT_USER, approved, name, true),
          "enable clears flag");
    Check(!StartupOptimizer::IsStartupApprovedDisabled(HKEY_CURRENT_USER, approved, name),
          "enabled after clear");

    RegDeleteTreeW(HKEY_CURRENT_USER, root.c_str());
}

// ── 3. 大文件夹检测（阈值注入 + 父子去重）──
void TestLargeFolderDetector() {
    std::cout << "\n-- LargeFolderDetector --\n";
    namespace fs = std::filesystem;

    std::wstring root = TempRoot() + L"IceCleanCliDetect";
    std::wstring bigDir = root + L"\\BigCache";
    std::wstring sub = bigDir + L"\\nested";
    CreateDirectoryW(root.c_str(), nullptr);
    CreateDirectoryW(bigDir.c_str(), nullptr);
    CreateDirectoryW(sub.c_str(), nullptr);

    bool made = WriteDummy(sub + L"\\a.bin", 3) && WriteDummy(bigDir + L"\\b.bin", 2)
                && WriteDummy(root + L"\\small.bin", 0);   // <1MB 不影响
    Check(made, "prepare detection sandbox tree");

    LargeFolderDetector detector(1);  // 阈值 1MB
    int callbacks = 0;
    bool deepTickSeen = false;
    auto items = detector.DetectAt(root,
        [&callbacks, &deepTickSeen, &sub](const std::wstring& path, int found, uint64_t bytes) {
            callbacks++;
            // 流式测量应触及深层子目录，而不只是顶层
            if (path.find(sub) != std::wstring::npos) deepTickSeen = true;
        });

    // 父目录 BigCache 命中后不再下钻 → 只应有一条，且为父目录
    bool single = items.size() == 1;
    Check(single, "detect finds exactly one entry (parent-stop dedup)");
    if (!single) {
        for (auto& it : items) {
            std::wcout << L"       got: " << it.sourcePath << L" ("
                       << it.size << L" bytes)\n";
        }
    }
    bool rightOne = single && items[0].sourcePath == bigDir;
    Check(rightOne, "entry is the parent dir above threshold");
    Check(callbacks >= 2, "progress callback fired multiple times");
    Check(deepTickSeen, "streaming ticks reach nested subdirectories");

    fs::remove_all(fs::path(root));
}

// ── 4a. Junction 原语（同卷沙箱内，零风险）──
void TestJunctionPrimitives() {
    std::cout << "\n-- Junction primitives (same-volume sandbox) --\n";

    std::wstring base = TempRoot() + L"IceCleanCliJct";
    std::wstring real = base + L"\\real";
    std::wstring link = base + L"\\link";

    // 清理历史残留
    Utils::JunctionPoint::Remove(link);
    Utils::FileUtil::DeleteFolder(base);
    CreateDirectoryW(base.c_str(), nullptr);
    CreateDirectoryW(real.c_str(), nullptr);
    bool made = WriteDummy(real + L"\\data.bin", 1);
    Check(made, "prepare junction sandbox");

    Check(Utils::JunctionPoint::Create(link, real), "junction create");
    Check(Utils::JunctionPoint::IsJunction(link), "junction detected");

    std::wstring got = Utils::JunctionPoint::GetTarget(link);
    // 目标串可能带 \\?\ 前缀或尾部反斜杠，做归一化包含判断
    bool targetOk = got.find(L"real") != std::wstring::npos;
    Check(targetOk, "junction target resolves to real dir");
    Check(Utils::FileUtil::Exists(link + L"\\data.bin"), "data readable through junction");

    Check(Utils::JunctionPoint::Remove(link), "junction remove");
    Check(!Utils::JunctionPoint::IsJunction(link), "junction gone after remove");
    Check(Utils::FileUtil::Exists(real + L"\\data.bin"),
          "real data intact after junction removal");

    Utils::FileUtil::DeleteFolder(base);
}

// ── 4b. 完整迁移端到端（会在目标盘创建镜像树并删除源目录，需显式 --with-migration 启用）──
void TestJunctionMigrationE2E() {
    std::cout << "\n-- Folder migration E2E (opt-in) --\n";

    // 源必须在系统盘（迁移语义假设 C: 源）；TEMP 不在系统盘则跳过
    wchar_t srcDrive = (wchar_t)towupper(TempRoot()[0]);
    if (srcDrive != L'C') {
        std::cout << "[SKIP] TEMP not on C:, mirror-path semantics not exercised\n";
        return;
    }

    // 选一个非源盘的固定磁盘作目标
    std::wstring targetDrive;
    for (wchar_t d = L'D'; d <= L'Z' && targetDrive.empty(); ++d) {
        wchar_t r[4] = { d, L':', L'\\', 0 };
        if (GetDriveTypeW(r) == DRIVE_FIXED && towupper(d) != srcDrive) {
            ULARGE_INTEGER freeBytes{};
            if (GetDiskFreeSpaceExW(r, &freeBytes, nullptr, nullptr) &&
                freeBytes.QuadPart > 200ULL * 1024 * 1024) {
                targetDrive = std::wstring(1, d) + L":\\";
            }
        }
    }
    if (targetDrive.empty()) {
        std::cout << "[SKIP] no second fixed drive available for junction test\n";
        return;
    }

    std::wstring src = TempRoot() + L"IceCleanCliMigrate";
    std::wstring inner = src + L"\\payload";
    CreateDirectoryW(src.c_str(), nullptr);
    CreateDirectoryW(inner.c_str(), nullptr);
    bool made = WriteDummy(inner + L"\\data.bin", 2);
    Check(made, "prepare source folder");

    FolderMigrator migrator(src, L"CliMigrationItem");
    MigrationItem item;
    item.name = L"CliMigrationItem";
    item.sourcePath = src;
    item.size = 2ull * 1024 * 1024 * 1024;  // 触发 Recommended；实际写入仅 2MB
    item.type = MigrationType::CustomFolder;
    item.selected = true;

    auto result = migrator.Migrate({ item }, targetDrive, nullptr);

    std::wstring expectedTarget = targetDrive +
        src.substr(2);  // 去掉盘符: \Users\...\IceCleanCliMigrate

    Check(result.success && result.migratedCount == 1, "migration reports success");
    Check(Utils::JunctionPoint::IsJunction(src), "source path is now a junction");
    Check(Utils::FileUtil::Exists(expectedTarget + L"\\payload\\data.bin"),
          "data accessible at target location");
    // 通过 junction 读回数据
    Check(Utils::FileUtil::Exists(src + L"\\payload\\data.bin"),
          "data readable through junction");

    // 清理：先摘 junction 再删目标数据
    Utils::JunctionPoint::Remove(src);
    Check(!Utils::JunctionPoint::IsJunction(src), "junction removed on cleanup");
    Utils::FileUtil::DeleteFolder(expectedTarget);
    if (!Utils::FileUtil::Exists(src)) Utils::FileUtil::DeleteFolder(src);
}

// ── 5. 服务枚举（只读）──
void TestServicesReadOnly() {
    std::cout << "\n-- ServiceOptimizer (read-only) --\n";
    ServiceOptimizer opt;
    auto disablable = opt.GetDisablableServices();
    std::wcout << L"       disablable auto-start services: " << disablable.size() << L"\n";
    Check(true, "service enumeration completed without crash");
}

// ── 6. 启动项读取（只读）──
void TestStartupItemsReadOnly() {
    std::cout << "\n-- StartupOptimizer (read-only) --\n";
    StartupOptimizer opt;
    auto items = opt.GetStartupItems();
    int off = static_cast<int>(std::count_if(items.begin(), items.end(),
                                             [](const StartupItem& i){ return !i.isEnabled; }));
    std::wcout << L"       startup items: " << items.size()
               << L", disabled-by-flag: " << off << L"\n";
    Check(true, "startup enumeration completed without crash");
}

// ── 7. 软件推荐：种子 JSON 离线解析（nlohmann + UTF-8 中文链路）──
void TestRecommendSeed() {
    std::cout << "\n-- Software recommend seed (offline parse) --\n";

    IceClean::Core::Safety::SoftwareRecommendFetcher& fetcher =
        IceClean::Core::Safety::SoftwareRecommendFetcher::Instance();
    IceClean::Models::RecommendData data;
    if (!fetcher.ParseJson(IceClean::Core::Safety::GetSeedJsonUtf8(), data)) {
        Check(false, "seed json parses via nlohmann");
        return;
    }
    Check(true, "seed json parses via nlohmann");

    Check(data.version > 0, "seed has version");
    Check(data.categories.size() >= 4,
          ("seed categories >= 4 (got " + std::to_string(data.categories.size()) + ")").c_str());
    Check(data.software.size() >= 10,
          ("seed software count >= 10 (got " + std::to_string(data.software.size()) + ")").c_str());

    auto hasName = [&data](const wchar_t* n) {
        for (const auto& s : data.software) {
            if (s.name == n) {
                // 中文描述必须完整（UTF-8 链路验证：长度>4 且首字符非乱码问号）
                return s.description.size() > 4 && s.description[0] != L'?';
            }
        }
        return false;
    };
    Check(hasName(L"7-Zip"), "contains 7-Zip with intact description");
    Check(hasName(L"FileZilla"), "contains FileZilla with intact description");
    Check(hasName(L"RustDesk"), "contains RustDesk with intact description");
    Check(hasName(L"ONLYOFFICE"), "contains ONLYOFFICE with intact description");

    // 分类中文名完整性抽查
    bool catOk = false;
    for (const auto& c : data.categories) {
        if (c.id == L"network" && c.name == L"网络传输") catOk = true;
    }
    Check(catOk, "category Chinese name round-trips (Utf8ToWide)");
}

// ── 8.5 驱动：SetupDi 设备&驱动枚举（只读）──
void TestDriversReadOnly() {
    std::cout << "\n-- Driver (SetupDi + pnputil, read-only) --\n";

    auto devices = IceClean::Core::Driver::DeviceDriverScanner::Enumerate();
    std::wcout << L"       enumerated devices/drivers: " << devices.size() << L"\n";
    Check(devices.size() >= 1, "SetupDi enumeration returns >=1 entry");

    bool fields = std::any_of(devices.begin(), devices.end(), [](const auto& d) {
        return !d.deviceName.empty() || !d.driverDesc.empty();
    });
    Check(fields, "at least one entry has non-empty device/driver name");
    Check(std::any_of(devices.begin(), devices.end(),
                      [](const auto& d) { return d.statusText == L"正常"; }),
          "normal-status devices present");

    auto packages = IceClean::Core::Driver::PnpUtilRunner::EnumDrivers();
    std::wcout << L"       driver packages: " << packages.size() << L"\n";
    Check(packages.size() >= 1, "pnputil /enum-drivers returns >=1 package");
    Check(std::all_of(packages.begin(), packages.end(),
                      [](const auto& p) { return !p.publishedName.empty(); }),
          "every package has a published name (oemXX.inf)");
}

// ── 8. 软件推荐：在线拉取（可选，--with-online 启用）──
void TestRecommendOnline() {
    std::cout << "\n-- Software recommend online fetch --\n";
    auto& fetcher = IceClean::Core::Safety::SoftwareRecommendFetcher::Instance();
    IceClean::Models::RecommendData data;
    if (!fetcher.FetchSync(data)) {
        std::cout << "[SKIP] network unavailable or remote file not yet pushed\n";
        return;
    }
    Check(!data.software.empty(), "remote recommend list fetched and parsed");
}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    bool withMigration = argc > 1 && std::string(argv[1]) == "--with-migration";
    bool withOnline = (argc > 1 && std::string(argv[1]) == "--with-online") ||
                      (argc > 2 && std::string(argv[2]) == "--with-online");

    std::cout << "=== IceClean core self-check ===\n";
    std::cout << "(writes confined to HKCU\\Software\\IceClean\\CliTests and %TEMP%\\IceCleanCli*)\n";

    TestRegistry();
    TestStartupApproved();
    TestLargeFolderDetector();
    TestJunctionPrimitives();
    if (withMigration) {
        TestJunctionMigrationE2E();   // 显式启用：跨盘镜像树 + 删源 + 建联接
    } else {
        std::cout << "\n[SKIP] migration E2E (pass --with-migration to enable)\n";
    }
    TestServicesReadOnly();
    TestStartupItemsReadOnly();
    TestDriversReadOnly();
    TestRecommendSeed();
    if (withOnline) {
        TestRecommendOnline();        // 显式启用：真实访问 GitHub 原始文件
    } else {
        std::cout << "[SKIP] recommend online fetch (pass --with-online to enable)\n";
    }

    std::cout << "\n=== result: " << g_pass << " passed, " << g_fail << " failed ===\n";
    return g_fail == 0 ? 0 : 1;
}
