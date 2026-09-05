@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=%PATH%;C:\Windows\System32\WindowsPowerShell\v1.0
set VCPKG_ROOT=f:\project\iceClean\vcpkg
set VCPKG_HOST_TRIPLET=x64-windows-static
set VCPKG_TARGET_TRIPLET=x64-windows-static
cd /d f:\project\iceClean

:configure
cmake -B build/x64-release -S . -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DVCPKG_TARGET_TRIPLET=x64-windows-static ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"
if %ERRORLEVEL% NEQ 0 (
    echo CMake configure failed, retrying in 10 seconds...
    timeout /t 10 /nobreak >nul
    goto configure
)

cmake --build build/x64-release --config Release --parallel
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)
echo Build succeeded!
echo Output: build\x64-release\Release\IceClean.exe
