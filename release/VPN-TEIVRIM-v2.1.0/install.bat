# ============================================
# VPN-TEIVRIM Installer
# ============================================
# Run as Administrator!
# ============================================

$ErrorActionPreference = "Stop"
$installDir = "C:\VPN-TEIVRIM"
$startMenu = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\VPN-TEIVRIM"
$desktop = [Environment]::GetFolderPath("Desktop")

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  VPN-TEIVRIM Installer" -ForegroundColor Cyan
Write-Host "  v2.1 Release" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Admin check
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "[!] Run as Administrator!" -ForegroundColor Red; exit 1
}

# Kill running instances
Write-Host "[1/8] Cleaning up..." -ForegroundColor Yellow
Get-Process -Name "vpn-gui" -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process -Name "wireguard" -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep 1

# Install WireGuard via Chocolatey
Write-Host "[2/8] Checking WireGuard..." -ForegroundColor Yellow
$wgExe = "C:\Program Files\WireGuard\wg.exe"
if (-not (Test-Path $wgExe)) {
    Write-Host "  -> Installing WireGuard via Chocolatey..." -ForegroundColor Gray
    if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
        Write-Host "  -> Installing Chocolatey..." -ForegroundColor Gray
        Set-ExecutionPolicy Bypass -Scope Process -Force
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-Expression ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
        refreshenv
    }
    choco install wireguard -y --force
    refreshenv
}
Write-Host "  -> WireGuard OK" -ForegroundColor Green

# Create install directory
Write-Host "[3/8] Creating install directory..." -ForegroundColor Yellow
if (-not (Test-Path $installDir)) { New-Item -ItemType Directory -Path $installDir -Force | Out-Null }

# Copy files
Write-Host "[4/8] Copying files..." -ForegroundColor Yellow
$sourceDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$files = @(
    "vpn-gui.exe", "vpn-gui.cpp",
    "vpn-setup.ps1", "vpn-setup.bat",
    "vpn-privacy-setup.ps1", "vpn-privacy-addclient.ps1",
    "vpn-harden.ps1", "vpn-harden-revert.ps1",
    "vpn-killswitch.ps1", "vpn-leaktest.ps1",
    "vpn-anonymity.ps1", "vpn-monitor.ps1", "vpn-monitor.bat",
    "vpn-status.ps1", "vpn-control.bat",
    "vpn-full-setup.bat", "uninstall.bat"
)
foreach ($f in $files) {
    $src = Join-Path $sourceDir $f
    if (Test-Path $src) {
        Copy-Item $src $installDir -Force
        Write-Host "  -> $f" -ForegroundColor Gray
    }
}

# Start menu
Write-Host "[5/8] Creating shortcuts..." -ForegroundColor Yellow
if (-not (Test-Path $startMenu)) { New-Item -ItemType Directory -Path $startMenu -Force | Out-Null }

# Desktop shortcut
$shell = New-Object -ComObject WScript.Shell
$shortcut = $shell.CreateShortcut("$desktop\VPN-TEIVRIM.lnk")
$shortcut.TargetPath = "$installDir\vpn-gui.exe"
$shortcut.WorkingDirectory = $installDir
$shortcut.IconLocation = "$installDir\vpn-gui.exe,0"
$shortcut.Description = "VPN-TEIVRIM Monitor"
$shortcut.Save()

# Start menu shortcut
$startShortcut = $shell.CreateShortcut("$startMenu\VPN-TEIVRIM.lnk")
$startShortcut.TargetPath = "$installDir\vpn-gui.exe"
$startShortcut.WorkingDirectory = $installDir
$startShortcut.IconLocation = "$installDir\vpn-gui.exe,0"
$startShortcut.Save()

Write-Host "  -> Desktop shortcut created" -ForegroundColor Gray
Write-Host "  -> Start menu shortcut created" -ForegroundColor Gray

# Autostart
Write-Host "[6/8] Setting up autostart..." -ForegroundColor Yellow
$autostartDir = "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\Startup"
$autoShortcut = $shell.CreateShortcut("$autostartDir\VPN-TEIVRIM.lnk")
$autoShortcut.TargetPath = "$installDir\vpn-gui.exe"
$autoShortcut.WorkingDirectory = $installDir
$autoShortcut.WindowStyle = 7
$autoShortcut.Save()

# Also register in registry
$regPath = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run"
Set-ItemProperty -Path $regPath -Name "VPN-TEIVRIM" -Value "`"$installDir\vpn-gui.exe`"" -Type String -Force
Write-Host "  -> Autostart enabled (Startup + Registry)" -ForegroundColor Green

# Firewall rules
Write-Host "[7/8] Configuring firewall..." -ForegroundColor Yellow
netsh advfirewall firewall add rule name="VPN-TEIVRIM-In" dir=in action=allow protocol=udp localport=51820 2>$null
netsh advfirewall firewall add rule name="VPN-TEIVRIM-Out" dir=out action=allow protocol=udp remoteport=51820 2>$null
netsh advfirewall firewall add rule name="VPN-TEIVRIM-GUI" dir=out action=allow program="$installDir\vpn-gui.exe" 2>$null
netsh advfirewall firewall add rule name="VPN-TEIVRIM-GUI-In" dir=in action=allow program="$installDir\vpn-gui.exe" 2>$null
Write-Host "  -> Firewall rules added" -ForegroundColor Green

# Setup server if needed
Write-Host "[8/8] Setting up VPN server..." -ForegroundColor Yellow
if (-not (Test-Path "C:\WireGuard\wg0.conf")) {
    Write-Host "  -> Running first-time setup..." -ForegroundColor Gray
    & "$installDir\vpn-privacy-setup.ps1"
} else {
    Write-Host "  -> Server config exists, skipping" -ForegroundColor Gray
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  VPN-TEIVRIM INSTALLED!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Install dir:  $installDir" -ForegroundColor White
Write-Host "  GUI:          $installDir\vpn-gui.exe" -ForegroundColor White
Write-Host "  Autostart:    YES (on boot)" -ForegroundColor White
Write-Host "  Firewall:     Configured" -ForegroundColor White
Write-Host ""
Write-Host "  Next steps:" -ForegroundColor Cyan
Write-Host "    1. Forward UDP 51820 on router to your LAN IP" -ForegroundColor White
Write-Host "    2. Run GUI and click 'Start'" -ForegroundColor White
Write-Host "    3. Import client config on phone" -ForegroundColor White
Write-Host ""
Write-Host "  Launch VPN-TEIVRIM now? (Y/N)" -ForegroundColor Yellow
$ans = Read-Host
if ($ans -eq "Y" -or $ans -eq "y") {
    Start-Process "$installDir\vpn-gui.exe"
}
Write-Host ""
