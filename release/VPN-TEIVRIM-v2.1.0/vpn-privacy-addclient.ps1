# ============================================
# Privacy Client Generator
# ============================================
# Run as Administrator!
# ============================================

param(
    [string]$ClientName = "phone",
    [string]$Subnet = "10.0.0",
    [string]$DNS = "1.1.1.1, 1.0.0.1",
    [string]$Endpoint = "",
    [int]$MTU = 1420
)

$wgExe = "C:\Program Files\WireGuard\wg.exe"
$wgDir = "C:\WireGuard"
$serverConf = "$wgDir\wg0.conf"

if (-not (Test-Path $wgExe)) { Write-Host "[!] WireGuard not installed" -ForegroundColor Red; exit 1 }
if (-not (Test-Path $serverConf)) { Write-Host "[!] Server not configured. Run vpn-privacy-setup.ps1 first" -ForegroundColor Red; exit 1 }

# Read server public key
$serverContent = Get-Content $serverConf -Raw
$serverPub = ""
if ($serverContent -match "PublicKey = (.+)") { $serverPub = $Matches[1].Trim() }
if (-not $serverPub) { Write-Host "[!] Cannot read server public key" -ForegroundColor Red; exit 1 }

# Auto-detect endpoint
if (-not $Endpoint) {
    $ip = (Invoke-WebRequest -Uri "https://api.ipify.org" -UseBasicParsing -TimeoutSec 5).Content
    $Endpoint = "$ip`:$($ServerPort ?? 51820)"
}

# Find next available IP
$usedIPs = & $wgExe showconf wg0 2>$null | Select-String "AllowedIPs"
$nextNum = 2
foreach ($line in $usedIPs) {
    if ($line -match "$Subnet\.(\d+)") {
        $num = [int]$Matches[1]
        if ($num -ge $nextNum) { $nextNum = $num + 1 }
    }
}
$clientIP = "$Subnet.$nextNum/32"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Privacy Client Generator" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Generate keys
$clientPriv = & $wgExe genkey
$clientPub = $clientPriv | & $wgExe pubkey
$preshared = & $wgExe genpsk

Write-Host "  Name:     $ClientName" -ForegroundColor White
Write-Host "  IP:       $clientIP" -ForegroundColor White
Write-Host "  DNS:      $DNS" -ForegroundColor White
Write-Host "  MTU:      $MTU" -ForegroundColor White
Write-Host "  Endpoint: $Endpoint" -ForegroundColor White
Write-Host ""

# Client config
$clientConf = @"
[Interface]
PrivateKey = $clientPriv
Address = $clientIP
DNS = $DNS
MTU = $MTU

[Peer]
PublicKey = $serverPub
PresharedKey = $preshared
Endpoint = $Endpoint
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 25
"@

$confPath = "$wgDir\$ClientName.conf"
Set-Content -Path $confPath -Value $clientConf -Encoding ASCII

# Add peer to server
$peerBlock = @"

[Peer]
PublicKey = $clientPub
PresharedKey = $preshared
AllowedIPs = $clientIP
"@
Add-Content -Path $serverConf -Value $peerBlock

# Restart server
Write-Host "  Restarting server..." -ForegroundColor Gray
& "C:\Program Files\WireGuard\wireguard.exe" /uninstalltunnelservice wg0 2>$null
Start-Sleep 1
& "C:\Program Files\WireGuard\wireguard.exe" /installtunnelservice $serverConf 2>$null
Start-Sleep 2

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  Client '$ClientName' created!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "  Config: $confPath" -ForegroundColor White
Write-Host "  IP: $clientIP" -ForegroundColor White
Write-Host ""
Write-Host "  Privacy features in this config:" -ForegroundColor Cyan
Write-Host "    [x] DNS: $DNS (encrypted DNS)" -ForegroundColor White
Write-Host "    [x] MTU: $MTU (anti-fragmentation)" -ForegroundColor White
Write-Host "    [x] PresharedKey: quantum-resistant" -ForegroundColor White
Write-Host "    [x] Full tunnel: 0.0.0.0/0 (all traffic)" -ForegroundColor White
Write-Host ""
Write-Host "  QR code:" -ForegroundColor Yellow
Write-Host "    Open https://qrenco.de/" -ForegroundColor White
Write-Host "    Paste: type $confPath" -ForegroundColor White
Write-Host ""
