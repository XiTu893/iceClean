@echo off
rem 一键验证启动优化核心功能：构建 CLI 工具并执行自检
rem 用法：run_startup_checks.bat          （跑沙箱自检）
rem       run_startup_checks.bat --list  （只读列出真实启动项）

call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
set VCPKG_ROOT=f:\project\iceClean\vcpkg
set CMAKE="C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

%CMAKE% --build f:\project\iceClean\build\x64-debug --target startup_opt_cli
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)

f:\project\iceClean\build\x64-debug\startup_opt_cli.exe %1
set RC=%ERRORLEVEL%
echo.
echo CHECK_EXIT=%RC%
exit /b %RC%
