@echo off
echo ========================================
echo   VPN Privacy - Full Setup
echo ========================================
echo.
echo This will:
echo   1. Install WireGuard
echo   2. Create VPN server
echo   3. Harden Windows for privacy
echo   4. Generate client config
echo.
echo Must run as Administrator!
echo.
pause
echo.

echo [1/3] Installing WireGuard...
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-setup.ps1"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: WireGuard setup failed
    pause
    exit /b 1
)

echo.
echo [2/3] Privacy server setup...
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-privacy-setup.ps1"

echo.
echo [3/3] Windows hardening...
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-harden.ps1"

echo.
echo ========================================
echo   DONE! Your VPN is privacy-optimized.
echo ========================================
echo.
echo Next: Forward UDP 51820 on router to your LAN IP
echo Then: Import client0.conf on phone
echo.
pause
