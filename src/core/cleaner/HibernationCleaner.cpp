#include "HibernationCleaner.h"
#include "utils/Win32Util.h"
#include "utils/FileUtil.h"
#include <windows.h>

namespace IceClean::Core::Cleaner {

Models::CleanResult HibernationCleaner::Clean(const std::vector<std::wstring>& paths,
                                               std::function<void(const Models::CleanProgress&)> progressCallback,
                                               const std::atomic<bool>* cancelFlag) {
    Models::CleanResult result;

    // 检查管理员权限
    if (!Utils::Win32Util::IsRunningAsAdmin()) {
        result.success = false;
        result.errorMessage = L"禁用休眠需要管理员权限";
        return result;
    }

    // 获取休眠文件大小（用于报告）
    uint64_t hiberfilSize = 0;
    DWORD highSize = 0;
    DWORD lowSize = GetCompressedFileSizeW(L"C:\\hiberfil.sys", &highSize);
    if (lowSize != INVALID_FILE_SIZE || GetLastError() == NO_ERROR) {
        ULARGE_INTEGER fileSize;
        fileSize.LowPart = lowSize;
        fileSize.HighPart = highSize;
        hiberfilSize = fileSize.QuadPart;
    }

    // 发送进度回调
    if (progressCallback) {
        Models::CleanProgress progress;
        progress.currentItem = 0;
        progress.totalItems = 1;
        progress.cleanedSize = 0;
        progress.totalSize = hiberfilSize;
        progress.currentFile = L"hiberfil.sys";
        progress.isRunning = true;
        progressCallback(progress);
    }

    // 执行 powercfg.exe /hibernate off 来禁用休眠并删除 hiberfil.sys
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};

    std::wstring cmdLine = L"powercfg.exe /hibernate off";

    BOOL success = CreateProcessW(
        nullptr,
        cmdLine.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    if (!success) {
        result.success = false;
        result.errorMessage = L"执行 powercfg 命令失败";
        result.failedFileCount = 1;
        return result;
    }

    // 等待命令执行完成
    WaitForSingleObject(pi.hProcess, 30000);  // 30秒超时

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode == 0) {
        result.success = true;
        result.totalCleanedSize = hiberfilSize;
        result.cleanedFileCount = 1;
    } else {
        result.success = false;
        result.errorMessage = L"禁用休眠失败";
        result.failedFileCount = 1;
    }

    // 发送完成回调
    if (progressCallback) {
        Models::CleanProgress progress;
        progress.currentItem = 1;
        progress.totalItems = 1;
        progress.cleanedSize = result.success ? hiberfilSize : 0;
        progress.totalSize = hiberfilSize;
        progress.isRunning = false;
        progressCallback(progress);
    }

    return result;
}

} // namespace IceClean::Core::Cleaner
