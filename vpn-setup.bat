@echo off
echo ========================================
echo   WireGuard VPN Server - Quick Setup
echo ========================================
echo.
echo Запуск от имени администратора...
echo.
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-setup.ps1"
pause
