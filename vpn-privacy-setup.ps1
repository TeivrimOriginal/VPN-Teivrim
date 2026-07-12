# ============================================
# WireGuard Server - Privacy Optimized Setup
# ============================================
# Run as Administrator!
# ============================================

param(
    [string]$ServerIP = "91.212.68.231",
    [int]$ServerPort = 51820,
    [string]$TunnelSubnet = "10.0.0",
    [string]$DNS = "1.1.1.1, 1.0.0.1",
    [int]$MTU = 1420
)

$ErrorActionPreference = "Stop"
$wgDir = "C:\WireGuard"
$wgExe = "C:\Program Files\WireGuard\wg.exe"
$wgGui = "C:\Program Files\WireGuard\wireguard.exe"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  WireGuard Privacy VPN Server Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Admin check
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "[!] Run as Administrator!" -ForegroundColor Red; exit 1
}

# Check WireGuard
if (-not (Test-Path $wgExe)) {
    Write-Host "[!] WireGuard not found. Run vpn-setup.bat first." -ForegroundColor Red; exit 1
}

if (-not (Test-Path $wgDir)) { New-Item -ItemType Directory -Path $wgDir -Force | Out-Null }

# Generate keys
Write-Host "[1/5] Generating server keys..." -ForegroundColor Yellow
$serverPriv = & $wgExe genkey
$serverPub = $serverPriv | & $wgExe pubkey
Write-Host "  -> OK" -ForegroundColor Green

Write-Host "[2/5] Generating client keys..." -ForegroundColor Yellow
$clientPriv = & $wgExe genkey
$clientPub = $clientPriv | & $wgExe pubkey
$preshared = & $wgExe genpsk
Write-Host "  -> OK" -ForegroundColor Green

# Server config with privacy optimizations
Write-Host "[3/5] Creating server config..." -ForegroundColor Yellow
$serverConfig = @"
[Interface]
PrivateKey = $serverPriv
Address = $TunnelSubnet.1/24
ListenPort = $ServerPort
MTU = $MTU

# NAT for internet access through VPN
PostUp = netsh advfirewall firewall add rule name=""VPN-Tunnel-In"" dir=in action=allow protocol=udp localport=$ServerPort
PostUp = netsh advfirewall firewall add rule name=""VPN-Tunnel-Allow"" dir=in action=allow remoteip=$TunnelSubnet.0/24
PostUp = netsh advfirewall firewall add rule name=""VPN-Tunnel-Out"" dir=out action=allow remoteip=$TunnelSubnet.0/24

PostDown = netsh advfirewall firewall delete rule name=""VPN-Tunnel-In""
PostDown = netsh advfirewall firewall delete rule name=""VPN-Tunnel-Allow""
PostDown = netsh advfirewall firewall delete rule name=""VPN-Tunnel-Out""

[Peer]
PublicKey = $clientPub
PresharedKey = $preshared
AllowedIPs = $TunnelSubnet.2/32
"@

Set-Content -Path "$wgDir\wg0.conf" -Value $serverConfig -Encoding ASCII
Write-Host "  -> $wgDir\wg0.conf" -ForegroundColor Gray

# Client config with full privacy
Write-Host "[4/5] Creating privacy-optimized client config..." -ForegroundColor Yellow
$clientConfig = @"
[Interface]
PrivateKey = $clientPriv
Address = $TunnelSubnet.2/24
DNS = $DNS
MTU = $MTU

[Peer]
PublicKey = $serverPub
PresharedKey = $preshared
Endpoint = ${ServerIP}:${ServerPort}
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 25
"@

Set-Content -Path "$wgDir\client0.conf" -Value $clientConfig -Encoding ASCII
Write-Host "  -> $wgDir\client0.conf" -ForegroundColor Gray

# Start server
Write-Host "[5/5] Starting server..." -ForegroundColor Yellow
Set-NetIPInterface -Forwarding Enabled -ErrorAction SilentlyContinue
reg add "HKLM\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" /v IPEnableRouter /t REG_DWORD /d 1 /f | Out-Null

& $wgGui /installtunnelservice "$wgDir\wg0.conf"
Start-Sleep 2

$show = & $wgExe show wg0 2>&1
$running = $show -match "listening port"

if ($running) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "  VPN SERVER RUNNING!" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
} else {
    Write-Host "[!] Server failed to start" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Privacy features:" -ForegroundColor Cyan
Write-Host "  [x] DNS: $DNS (no ISP DNS leaks)" -ForegroundColor White
Write-Host "  [x] MTU: $MTU (optimized for encryption)" -ForegroundColor White
Write-Host "  [x] PresharedKey: extra encryption layer" -ForegroundColor White
Write-Host "  [x] All traffic routed through VPN (0.0.0.0/0)" -ForegroundColor White
Write-Host "  [x] PersistentKeepalive: connection stays alive" -ForegroundColor White
Write-Host ""
Write-Host "Client config: $wgDir\client0.conf" -ForegroundColor Yellow
Write-Host "Port: $ServerPort/UDP" -ForegroundColor White
Write-Host "Subnet: $TunnelSubnet.0/24" -ForegroundColor White
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Cyan
Write-Host "  1. Run vpn-harden.ps1 for full Windows hardening" -ForegroundColor White
Write-Host "  2. Forward UDP $ServerPort on router to your LAN IP" -ForegroundColor White
Write-Host "  3. Import client0.conf on phone/PC" -ForegroundColor White
Write-Host "  4. Test: https://dnsleaktest.com" -ForegroundColor White
Write-Host ""
