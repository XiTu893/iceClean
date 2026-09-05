@echo off
rem Incremental build of IceClean target (no reconfigure)
call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
set VCPKG_ROOT=f:\project\iceClean\vcpkg
set CMAKE="C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
%CMAKE% --build f:\project\iceClean\build\x64-debug --target IceClean
echo BAT_EXIT=%ERRORLEVEL%