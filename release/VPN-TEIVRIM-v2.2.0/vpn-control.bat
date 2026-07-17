@echo off
chcp 65001 >nul
title VPN-TEIVRIM Control Panel
color 0B

:menu
cls
echo.
echo  ╔══════════════════════════════════════════╗
echo  ║        VPN-TEIVRIM Control Panel        ║
echo  ╚══════════════════════════════════════════╝
echo.
echo  [1] Full Setup (install everything)
echo  [2] Start VPN Server
echo  [3] Stop VPN Server
echo  [4] Open GUI Dashboard
echo.
echo  --- Anonymity Levels ---
echo  [5] Level 1: Basic VPN
echo  [6] Level 2: + DNS Protection
echo  [7] Level 3: Maximum Privacy
echo.
echo  --- Tools ---
echo  [8] Add New Client
echo  [9] Kill Switch ON
echo  [0] Kill Switch OFF
echo  [T] Test for DNS Leaks
echo  [L] Open Logs
echo.
echo  [X] Exit
echo.
set /p choice="  Select: "

if "%choice%"=="1" goto fullsetup
if "%choice%"=="2" goto start
if "%choice%"=="3" goto stop
if "%choice%"=="4" goto gui
if "%choice%"=="5" goto level1
if "%choice%"=="6" goto level2
if "%choice%"=="7" goto level3
if "%choice%"=="8" goto addclient
if "%choice%"=="9" goto kson
if "%choice%"=="0" goto ksoff
if /i "%choice%"=="T" goto leaktest
if /i "%choice%"=="L" goto logs
if /i "%choice%"=="X" goto end
goto menu

:fullsetup
cls
echo Running full setup...
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-setup.ps1"
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-privacy-setup.ps1"
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-harden.ps1"
echo.
echo Setup complete!
pause
goto menu

:start
cls
echo Starting VPN server...
"C:\Program Files\WireGuard\wireguard.exe" /installtunnelservice "C:\WireGuard\wg0.conf"
timeout /t 2 >nul
echo.
"C:\Program Files\WireGuard\wg.exe" show wg0
echo.
pause
goto menu

:stop
cls
echo Stopping VPN server...
"C:\Program Files\WireGuard\wireguard.exe" /uninstalltunnelservice wg0
timeout /t 1 >nul
echo Server stopped.
pause
goto menu

:gui
start "" "%~dp0vpn-gui.exe"
goto menu

:level1
cls
echo Applying Anonymity Level 1...
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-anonymity.ps1" -Level 1
pause
goto menu

:level2
cls
echo Applying Anonymity Level 2...
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-anonymity.ps1" -Level 2
pause
goto menu

:level3
cls
echo Applying Anonymity Level 3 (Maximum)...
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-anonymity.ps1" -Level 3
pause
goto menu

:addclient
cls
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-privacy-addclient.ps1"
pause
goto menu

:kson
cls
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-killswitch.ps1" -Enable
pause
goto menu

:ksoff
cls
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-killswitch.ps1" -Disable
pause
goto menu

:leaktest
cls
powershell -ExecutionPolicy Bypass -File "%~dp0vpn-leaktest.ps1"
pause
goto menu

:logs
start notepad "C:\WireGuard\vpn-gui.log"
goto menu

:end
exit
