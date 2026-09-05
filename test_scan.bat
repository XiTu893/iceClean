# Test script for migration scan
$ErrorActionPreference = "SilentlyContinue"
Remove-Item "IceClean.log" -Force

Write-Host "Starting IceClean..."
$proc = Start-Process "F:\project\iceClean\build\x64-debug\src\IceClean.exe" -PassThru
Start-Sleep 3

Write-Host "Process started with PID: $($proc.Id)"
Write-Host "Waiting for user to click scan button..."
Write-Host "Press Enter in 30 seconds to capture log..."
Start-Sleep 30

if ((Test-Path "IceClean.log") -and (Get-Content "IceClean.log" -Raw) -match "\[MainWindow\]|\[MigrationPanel\]") {
    Write-Host "=== DEBUG LOG FOUND ==="
    Get-Content "IceClean.log" | Select-String "MainWindow|MigrationPanel"
} else {
    Write-Host "=== NO DEBUG LOG (scan may not have started) ==="
    Get-Content "IceClean.log" -Tail 20
}

Write-Host "Stopping process..."
Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
