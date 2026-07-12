@echo off
echo ========================================
echo   WireGuard VPN Monitor
echo ========================================
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-monitor.ps1"
pause
