@echo off
rem Build + run core self-check CLI (sandbox mode by default).
rem Usage: run_core_checks.bat              -> safe sandbox checks only
rem        run_core_checks.bat --with-migration  -> also run cross-drive migration E2E

call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
set VCPKG_ROOT=f:\project\iceClean\vcpkg
set CMAKE="C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

%CMAKE% --build f:\project\iceClean\build\x64-debug --target core_check_cli
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)

f:\project\iceClean\build\x64-debug\core_check_cli.exe %*
set RC=%ERRORLEVEL%
echo.
echo CHECK_EXIT=%RC%
exit /b %RC%
