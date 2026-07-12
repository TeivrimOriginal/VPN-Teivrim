# ============================================
# WireGuard - Генератор нового клиента
# ============================================
# Запуск от имени администратора!
# ============================================

param(
    [string]$ClientName = "client1",
    [string]$ServerPublicKey = "",
    [string]$ServerEndpoint = "91.212.68.231:51820",
    [string]$DNS = "1.1.1.1, 8.8.8.8",
    [string]$Subnet = "10.0.0"
)

$wgExe = "C:\Program Files\WireGuard\wg.exe"
$wgDir = "C:\WireGuard"
$serverConfigPath = "$wgDir\wg0.conf"

if (-not (Test-Path $wgExe)) {
    Write-Host "[!] WireGuard не установлен. Сначала запусти vpn-setup.ps1" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $serverConfigPath)) {
    Write-Host "[!] Конфиг сервера не найден. Запусти vpn-setup.ps1" -ForegroundColor Red
    exit 1
}

# Читаем публичный ключ сервера из конфига
if (-not $ServerPublicKey) {
    $serverContent = Get-Content $serverConfigPath -Raw
    if ($serverContent -match "PublicKey = (.+)") {
        $ServerPublicKey = $Matches[1].Trim()
    } else {
        Write-Host "[!] Не удалось прочитать PublicKey сервера" -ForegroundColor Red
        exit 1
    }
}

# Определяем следующий IP
$existingPeers = & $wgExe showconf wg0 2>$null | Select-String "AllowedIPs"
$clientNum = ($existingPeers | Measure-Object).Count + 1
$clientIP = "$Subnet.$($clientNum + 1)/32"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Генерация клиента: $ClientName" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Генерация ключей
$clientPrivateKey = & $wgExe genkey
$clientPublicKey = $clientPrivateKey | & $wgExe pubkey
$presharedKey = & $wgExe genpsk

Write-Host "  Client Public Key: $clientPublicKey" -ForegroundColor Gray
Write-Host "  Client IP: $clientIP" -ForegroundColor Gray

# Конфиг клиента
$clientConfig = @"
[Interface]
PrivateKey = $clientPrivateKey
Address = $clientIP
DNS = $DNS

[Peer]
PublicKey = $ServerPublicKey
PresharedKey = $presharedKey
Endpoint = $ServerEndpoint
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 25
"@

# Сохраняем конфиг
$clientConfigPath = "$wgDir\$ClientName.conf"
Set-Content -Path $clientConfigPath -Value $clientConfig -Encoding UTF8

# Добавляем пира в сервер
$peerConfig = @"

[Peer]
PublicKey = $clientPublicKey
PresharedKey = $presharedKey
AllowedIPs = $clientIP
"@
Add-Content -Path $serverConfigPath -Value $peerConfig

# Перезапуск сервера
Write-Host "  Перезапускаю WireGuard сервер..." -ForegroundColor Gray
& "C:\Program Files\WireGuard\wireguard.exe" /installtunnelservice $serverConfigPath 2>$null

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  Клиент $ClientName создан!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Конфиг: $clientConfigPath" -ForegroundColor White
Write-Host "  IP: $clientIP" -ForegroundColor White
Write-Host ""
Write-Host "Для QR-кода:" -ForegroundColor Cyan
Write-Host "  Открой https://qrenco.de/" -ForegroundColor White
Write-Host "  Вставь содержимое файла $clientConfigPath" -ForegroundColor White
Write-Host ""
