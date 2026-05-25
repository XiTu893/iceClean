#include "DismUtil.h"
#include <windows.h>
#include <sstream>

namespace IceClean::Utils {

bool DismUtil::StartComponentCleanup(bool resetBase,
                                     std::function<void(const std::wstring&)> outputCallback) {
    std::wstring args = L"/Online /Cleanup-Image /StartComponentCleanup";
    if (resetBase) {
        args += L" /ResetBase";
    }
    return ExecuteDism(args, outputCallback);
}

bool DismUtil::CompactOS(std::function<void(const std::wstring&)> outputCallback) {
    return ExecuteDism(L"/Online /Cleanup-Image /CompactOS=always", outputCallback);
}

bool DismUtil::ExecuteDism(const std::wstring& arguments,
                           std::function<void(const std::wstring&)> outputCallback) {
    // 创建管道用于捕获输出
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hReadPipe = nullptr;
    HANDLE hWritePipe = nullptr;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
        return false;
    }

    // 确保读取端不被继承
    SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

    // 构建命令行
    std::wstring cmdLine = L"dism.exe " + arguments;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {};

    // 创建进程
    BOOL success = CreateProcessW(
        nullptr,
        cmdLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi
    );

    // 关闭写入端(子进程的副本)
    CloseHandle(hWritePipe);

    if (!success) {
        CloseHandle(hReadPipe);
        return false;
    }

    // 读取输出
    char buffer[4096] = {};
    DWORD bytesRead = 0;
    std::string utf8Output;

    while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        utf8Output.append(buffer, bytesRead);

        // 尝试将已读取的内容转换为宽字符串并回调
        if (outputCallback) {
            // DISM输出通常是系统默认编码(中文Windows为GBK)
            int wLen = MultiByteToWideChar(CP_ACP, 0, utf8Output.c_str(), -1, nullptr, 0);
            if (wLen > 0) {
                std::wstring wideOutput(wLen - 1, L'\0');
                MultiByteToWideChar(CP_ACP, 0, utf8Output.c_str(), -1, wideOutput.data(), wLen);

                // 按行回调
                std::wistringstream wss(wideOutput);
                std::wstring line;
                while (std::getline(wss, line)) {
                    if (!line.empty()) {
                        outputCallback(line);
                    }
                }
            }
        }

        utf8Output.clear();
    }

    CloseHandle(hReadPipe);

    // 等待进程结束
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

} // namespace IceClean::Utils
