$ErrorActionPreference = "Stop"
$wgDir = "C:\WireGuard"
$ServerIP = "91.212.68.231"
$ServerPort = 51820
$TunnelIP = "10.0.0.1/24"
$ClientIP = "10.0.0.2/32"
$DNS = "1.1.1.1, 8.8.8.8"
$wgExe = "C:\Program Files\WireGuard\wg.exe"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  WireGuard VPN Server Auto-Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# --- Проверка админа ---
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "[!] Run as Administrator!" -ForegroundColor Red
    exit 1
}

# --- Скачивание и установка WireGuard ---
Write-Host "[1/7] Checking WireGuard..." -ForegroundColor Yellow
if (-not (Test-Path $wgExe)) {
    Write-Host "  -> Downloading WireGuard..." -ForegroundColor Gray
    $url = "https://download.wireguard.com/windows-client/wireguard-installer.exe"
    $installerPath = "$env:TEMP\wireguard-installer.exe"
    Invoke-WebRequest -Uri $url -OutFile $installerPath -UseBasicParsing
    Write-Host "  -> Installing..." -ForegroundColor Gray
    Start-Process -FilePath $installerPath -ArgumentList "/install" -Wait -NoNewWindow
    Remove-Item $installerPath -ErrorAction SilentlyContinue
    if (-not (Test-Path $wgExe)) {
        Write-Host "[!] Install failed. Download manually from https://download.wireguard.com/windows-client/" -ForegroundColor Red
        exit 1
    }
}
Write-Host "  -> WireGuard installed" -ForegroundColor Green

# --- Создание директории ---
if (-not (Test-Path $wgDir)) {
    New-Item -ItemType Directory -Path $wgDir -Force | Out-Null
}

# --- Генерация ключей сервера ---
Write-Host "[2/7] Generating server keys..." -ForegroundColor Yellow
$serverPrivateKey = & $wgExe genkey
$serverPublicKey = $serverPrivateKey | & $wgExe pubkey
Write-Host "  -> Server Public Key: $serverPublicKey" -ForegroundColor Gray

# --- Генерация ключей клиента ---
Write-Host "[3/7] Generating client keys..." -ForegroundColor Yellow
$clientPrivateKey = & $wgExe genkey
$clientPublicKey = $clientPrivateKey | & $wgExe pubkey
Write-Host "  -> Client Public Key: $clientPublicKey" -ForegroundColor Gray

# --- Генерация PresharedKey ---
Write-Host "[4/7] Generating PreSharedKey..." -ForegroundColor Yellow
$presharedKey = & $wgExe genpsk
Write-Host "  -> PreSharedKey created" -ForegroundColor Gray

# --- Конфиг сервера ---
Write-Host "[5/7] Creating server config..." -ForegroundColor Yellow
$serverConfigPath = "$wgDir\wg0.conf"
$line1 = "[Interface]"
$line2 = "PrivateKey = $serverPrivateKey"
$line3 = "Address = $TunnelIP"
$line4 = "ListenPort = $ServerPort"
$line5 = ""
$line6 = "[Peer]"
$line7 = "PublicKey = $clientPublicKey"
$line8 = "PresharedKey = $presharedKey"
$line9 = "AllowedIPs = $ClientIP"
$serverConfig = "$line1`n$line2`n$line3`n$line4`n$line5`n$line6`n$line7`n$line8`n$line9"
Set-Content -Path $serverConfigPath -Value $serverConfig -Encoding ASCII
Write-Host "  -> $serverConfigPath" -ForegroundColor Gray

# --- Конфиг клиента ---
Write-Host "[6/7] Creating client config..." -ForegroundColor Yellow
$clientConfigPath = "$wgDir\client0.conf"
$cLine1 = "[Interface]"
$cLine2 = "PrivateKey = $clientPrivateKey"
$cLine3 = "Address = $ClientIP"
$cLine4 = "DNS = $DNS"
$cLine5 = ""
$cLine6 = "[Peer]"
$cLine7 = "PublicKey = $serverPublicKey"
$cLine8 = "PresharedKey = $presharedKey"
$cLine9 = "Endpoint = ${ServerIP}:${ServerPort}"
$cLine10 = "AllowedIPs = 0.0.0.0/0, ::/0"
$cLine11 = "PersistentKeepalive = 25"
$clientConfig = "$cLine1`n$cLine2`n$cLine3`n$cLine4`n$cLine5`n$cLine6`n$cLine7`n$cLine8`n$cLine9`n$cLine10`n$cLine11"
Set-Content -Path $clientConfigPath -Value $clientConfig -Encoding ASCII
Write-Host "  -> $clientConfigPath" -ForegroundColor Gray

# --- Включение IP Forwarding ---
Write-Host "[7/7] Enabling IP Forwarding and starting server..." -ForegroundColor Yellow
Set-NetIPInterface -Forwarding Enabled -ErrorAction SilentlyContinue
reg add "HKLM\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" /v IPEnableRouter /t REG_DWORD /d 1 /f | Out-Null

# Файрвол
netsh advfirewall firewall add rule name="WireGuard UDP In" dir=in action=allow protocol=udp localport=$ServerPort 2>$null

# Запуск WireGuard
& "C:\Program Files\WireGuard\wireguard.exe" /installtunnelservice $serverConfigPath

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  DONE! VPN Server is running!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Server settings:" -ForegroundColor Cyan
Write-Host "  Endpoint:    ${ServerIP}:${ServerPort}" -ForegroundColor White
Write-Host "  Tunnel IP:   $TunnelIP" -ForegroundColor White
Write-Host "  Client IP:   $ClientIP" -ForegroundColor White
Write-Host ""
Write-Host "Config files:" -ForegroundColor Cyan
Write-Host "  Server:  $serverConfigPath" -ForegroundColor White
Write-Host "  Client:  $clientConfigPath" -ForegroundColor White
Write-Host ""
Write-Host "To connect phone:" -ForegroundColor Cyan
Write-Host "  1. Install WireGuard on phone" -ForegroundColor White
Write-Host "  2. Import client0.conf or scan QR code" -ForegroundColor White
Write-Host "  3. Enable tunnel" -ForegroundColor White
Write-Host ""
Write-Host "For QR code:" -ForegroundColor Cyan
Write-Host "  Open https://qrenco.de/ and paste client0.conf" -ForegroundColor White
Write-Host ""
Write-Host "IMPORTANT: Forward UDP port $ServerPort on your router to 192.168.0.102" -ForegroundColor Yellow
Write-Host ""
