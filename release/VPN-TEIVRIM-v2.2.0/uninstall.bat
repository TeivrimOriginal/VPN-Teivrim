@echo off
chcp 65001 >nul
echo ========================================
echo   VPN-TEIVRIM Uninstaller
echo ========================================
echo.
echo This will:
echo   - Stop VPN server
echo   - Remove autostart
echo   - Remove firewall rules
echo   - Remove install directory
echo.
echo WireGuard and configs in C:\WireGuard will NOT be deleted.
echo.
set /p confirm="Continue? (Y/N): "
if /i not "%confirm%"=="Y" exit

echo.
echo [1/5] Stopping services...
taskkill /f /im vpn-gui.exe 2>nul
"C:\Program Files\WireGuard\wireguard.exe" /uninstalltunnelservice wg0 2>nul

echo [2/5] Removing autostart...
reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" /v "VPN-TEIVRIM" /f 2>nul
del "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\VPN-TEIVRIM.lnk" 2>nul
del "%APPDATA%\Microsoft\Windows\Start Menu\Programs\VPN-TEIVRIM\VPN-TEIVRIM.lnk" 2>nul
del "%USERPROFILE%\Desktop\VPN-TEIVRIM.lnk" 2>nul

echo [3/5] Removing firewall rules...
netsh advfirewall firewall delete rule name="VPN-TEIVRIM-In" 2>nul
netsh advfirewall firewall delete rule name="VPN-TEIVRIM-Out" 2>nul
netsh advfirewall firewall delete rule name="VPN-TEIVRIM-GUI" 2>nul
netsh advfirewall firewall delete rule name="VPN-TEIVRIM-GUI-In" 2>nul
netsh advfirewall firewall delete rule name="L3-KS-Allow-WG" 2>nul
netsh advfirewall firewall delete rule name="L3-KS-Allow-WG-In" 2>nul
netsh advfirewall firewall delete rule name="L3-KS-Allow-Loop" 2>nul
netsh advfirewall firewall delete rule name="L3-KS-Allow-Loop-In" 2>nul
netsh advfirewall firewall delete rule name="L3-KS-Allow-LAN" 2>nul
netsh advfirewall firewall delete rule name="L3-KS-Allow-LAN-In" 2>nul
netsh advfirewall firewall delete rule name="L3-KS-Allow-Tun" 2>nul
netsh advfirewall firewall delete rule name="L3-KS-Allow-Tun-In" 2>nul
netsh advfirewall firewall delete rule name="L3-KS-Block-Out" 2>nul
netsh advfirewall firewall delete rule name="L3-KS-Block-In" 2>nul

echo [4/5] Removing install directory...
rmdir /s /q "C:\VPN-TEIVRIM" 2>nul

echo [5/5] Done.
echo.
echo VPN-TEIVRIM removed.
echo WireGuard configs kept in C:\WireGuard
echo.
pause
