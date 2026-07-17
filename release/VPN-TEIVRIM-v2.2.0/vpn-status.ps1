# ============================================
# WireGuard - Проверка статуса сервера
# ============================================

$wgExe = "C:\Program Files\WireGuard\wg.exe"

if (-not (Test-Path $wgExe)) {
    Write-Host "[!] WireGuard не установлен" -ForegroundColor Red
    exit 1
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  WireGuard Server Status" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Состояние интерфейса:" -ForegroundColor Yellow
& $wgExe show wg0

Write-Host ""
Write-Host "Состояние сервиса:" -ForegroundColor Yellow
Get-Service -Name "WireGuard*" -ErrorAction SilentlyContinue | Format-Table Name, Status -AutoSize

Write-Host ""
Write-Host "Подключённые пиры:" -ForegroundColor Yellow
& $wgExe show wg0 | Select-String "peer|endpoint|allowed|transfer" | ForEach-Object { Write-Host "  $_" }
