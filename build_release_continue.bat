@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64 >nul 2>&1
cd /d F:\project\iceClean
cmake --build build/x64-release --config Release
echo ExitCode=%ERRORLEVEL%
