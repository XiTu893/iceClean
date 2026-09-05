// startup_optimizer_cli —— 核心功能命令行验证工具（无 GUI 依赖）
//
// 用法：
//   startup_opt_cli --selftest      在 HKCU 沙箱键内跑核心功能自检（PASS/FAIL，退出码 0=全过）
//   startup_opt_cli --list          只读列出真实启动项及启用状态（诊断用，不修改任何东西）
//
// 背景：修复"LM Studio 等启动项禁用后刷新仍显示未优化"——
//       禁用改为任务管理器同款 StartupApproved 非破坏式标志，本工具逐项验证该机制。

#include <windows.h>
#include <algorithm>
#include <iostream>
#include <string>
#include "core/optimizer/StartupOptimizer.h"
#include "utils/RegistryUtil.h"

using namespace IceClean::Core::Optimizer;
using namespace IceClean::Models;
using IceClean::Utils::RegistryUtil;

namespace {

int g_pass = 0;
int g_fail = 0;

void Check(bool ok, const char* what) {
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << what << "\n";
    if (ok) ++g_pass; else ++g_fail;
}

constexpr wchar_t kSandboxRoot[] = L"Software\\IceClean\\Tests";
const std::wstring kRunKey = std::wstring(kSandboxRoot) + L"\\Run";
const std::wstring kApprovedKey = std::wstring(kSandboxRoot) + L"\\Approved";

// 模拟 Electron 应用（如 LM Studio）写入 HKCU Run 的典型条目
const std::wstring kAppName = L"electron.app.LM Studio";
const std::wstring kAppCmd =
    L"\"C:\\Users\\tester\\AppData\\Local\\Programs\\LM Studio\\LM Studio.exe\"";

void PrintSelfTestHeader() {
    std::wcout << L"=== StartupOptimizer self-test (sandbox: HKCU\\"
               << kSandboxRoot << L") ===\n";
}

// ── 自检场景 ──

void TestAbsentFlagMeansEnabled() {
    bool disabled = StartupOptimizer::IsStartupApprovedDisabled(
        HKEY_CURRENT_USER, kApprovedKey, L"Not.Exist");
    Check(!disabled, "absent approval flag means enabled");
}

void TestDisableEnableRoundTrip() {
    bool ok = StartupOptimizer::SetStartupApproved(
        HKEY_CURRENT_USER, kApprovedKey, kAppName, false);
    ok = ok && StartupOptimizer::IsStartupApprovedDisabled(HKEY_CURRENT_USER, kApprovedKey, kAppName);
    ok = ok && StartupOptimizer::SetStartupApproved(HKEY_CURRENT_USER, kApprovedKey, kAppName, true);
    ok = ok && !StartupOptimizer::IsStartupApprovedDisabled(HKEY_CURRENT_USER, kApprovedKey, kAppName);
    Check(ok, "approval flag disable->enable round trip");
}

void TestEnableWhenAlreadyEnabled() {
    // 标志本就不存在时恢复启用应幂等成功（旧实现的边界缺陷）
    bool ok = StartupOptimizer::SetStartupApproved(
        HKEY_CURRENT_USER, kApprovedKey, kAppName, true);
    Check(ok, "enable when already enabled succeeds (idempotent)");
}

void TestNonDestructiveDisable() {
    RegistryUtil::WriteStringValue(HKEY_CURRENT_USER, kRunKey, kAppName, kAppCmd);

    StartupOptimizer opt;
    bool step1 = opt.DisableRegistryItemAt(HKEY_CURRENT_USER, kRunKey, kApprovedKey, kAppName);

    // 关键断言：Run 值必须原样保留 —— 应用下次启动重写 Run 也无法复活自启
    std::wstring after = RegistryUtil::ReadStringValue(HKEY_CURRENT_USER, kRunKey, kAppName);
    bool preserved = (after == kAppCmd);
    bool flagged = StartupOptimizer::IsStartupApprovedDisabled(
        HKEY_CURRENT_USER, kApprovedKey, kAppName);

    Check(step1 && preserved,
          preserved ? "disable keeps Run value intact (non-destructive)"
                    : "disable DELETED Run value (regression!)");
    Check(step1 && flagged, "disable sets approval flag");
    if (!(step1 && preserved)) {
        std::cout << "       expected: ";
        std::wcout << kAppCmd << L"\n"
                   << L"       actual  : " << after << L"\n";
    }
}

void TestDisableIdempotent() {
    StartupOptimizer opt;
    bool ok = opt.DisableRegistryItemAt(HKEY_CURRENT_USER, kRunKey, kApprovedKey, kAppName);
    Check(ok, "repeated disable succeeds (idempotent)");
}

void TestEnableRestores() {
    StartupOptimizer opt;
    bool step1 = opt.EnableRegistryItemAt(HKEY_CURRENT_USER, kRunKey, kApprovedKey, kAppName);
    bool notFlagged = !StartupOptimizer::IsStartupApprovedDisabled(
        HKEY_CURRENT_USER, kApprovedKey, kAppName);
    std::wstring value = RegistryUtil::ReadStringValue(HKEY_CURRENT_USER, kRunKey, kAppName);
    Check(step1 && notFlagged && value == kAppCmd, "enable clears flag and preserves command");
}

void TestDisableMissingValueFails() {
    StartupOptimizer opt;
    bool ok = !opt.DisableRegistryItemAt(HKEY_CURRENT_USER, kRunKey, kApprovedKey, L"Ghost.App");
    Check(ok, "disable of unknown item fails honestly");
}

void TestLegacyBackupRecognized() {
    // 兼容旧版"备份+删除"机制产生的禁用态
    RegistryUtil::DeleteValue(HKEY_CURRENT_USER,
                              L"Software\\IceClean\\DisabledStartup", kAppName);
    RegistryUtil::WriteStringValue(HKEY_CURRENT_USER,
                                   L"Software\\IceClean\\DisabledStartup",
                                   kAppName, kAppCmd);

    StartupOptimizer opt;
    bool ok = opt.DisableRegistryItemAt(HKEY_CURRENT_USER, kRunKey, kApprovedKey, kAppName);

    RegistryUtil::DeleteValue(HKEY_CURRENT_USER,
                              L"Software\\IceClean\\DisabledStartup", kAppName);
    Check(ok, "legacy-backup disabled state recognized (migration compat)");
}

int RunSelfTest() {
    PrintSelfTestHeader();

    RegDeleteTreeW(HKEY_CURRENT_USER, kSandboxRoot);

    TestAbsentFlagMeansEnabled();
    TestDisableEnableRoundTrip();
    TestEnableWhenAlreadyEnabled();
    TestNonDestructiveDisable();
    TestDisableIdempotent();
    TestEnableRestores();
    TestDisableMissingValueFails();
    TestLegacyBackupRecognized();

    RegDeleteTreeW(HKEY_CURRENT_USER, kSandboxRoot);
    RegistryUtil::DeleteValue(HKEY_CURRENT_USER,
                              L"Software\\IceClean\\DisabledStartup", kAppName);

    std::cout << "\n=== result: " << g_pass << " passed, " << g_fail << " failed ===\n";
    return g_fail == 0 ? 0 : 1;
}

// ── 真实环境只读诊断 ──

int RunList() {
    std::cout << "=== Real startup items (read-only) ===\n";

    StartupOptimizer opt;
    auto items = opt.GetStartupItems();
    if (items.empty()) {
        std::cout << "(none found)\n";
        return 0;
    }

    for (const auto& it : items) {
        const wchar_t* type = L"other";
        switch (it.type) {
            case StartupItemType::Registry:      type = L"registry"; break;
            case StartupItemType::StartupFolder: type = L"folder";   break;
            case StartupItemType::ScheduledTask: type = L"task";     break;
            case StartupItemType::Service:       type = L"service";  break;
        }
        std::wcout << (it.isEnabled ? L"[ON ] " : L"[OFF] ")
                   << it.name << L"  (" << type << L")"
                   << L"  cmd=" << it.path << L"\n";
    }

    std::wcout << L"\ntotal: " << items.size()
               << L", disabled-by-flag: "
               << std::count_if(items.begin(), items.end(),
                                [](const auto& i){ return !i.isEnabled; })
               << L"\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);

    std::string mode = argc > 1 ? argv[1] : "--selftest";
    if (mode == "--selftest") return RunSelfTest();
    if (mode == "--list")     return RunList();

    std::cout << "usage:\n"
              << "  startup_opt_cli --selftest   run sandbox self-checks\n"
              << "  startup_opt_cli --list       list real startup items (read-only)\n";
    return argc > 1 ? 2 : 0;
}
