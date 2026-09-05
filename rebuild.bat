@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=%PATH%;C:\Windows\System32\WindowsPowerShell\v1.0
set VCPKG_ROOT=f:\project\iceClean\vcpkg
set VCPKG_HOST_TRIPLET=x64-windows-static
set VCPKG_TARGET_TRIPLET=x64-windows-static
cd /d f:\project\iceClean
cmake --build build/x64-debug 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)
echo Build succeeded!
