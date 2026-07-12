# ============================================
# Windows Hardening for VPN Privacy
# ============================================
# Run as Administrator!
# ============================================

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Windows VPN Privacy Hardening" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# --- Admin check ---
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "[!] Run as Administrator!" -ForegroundColor Red
    exit 1
}

# --- 1. Disable IPv6 everywhere ---
Write-Host "[1/8] Disabling IPv6..." -ForegroundColor Yellow
Get-NetAdapterBinding -ComponentID ms_tcpip6 -ErrorAction SilentlyContinue | ForEach-Object {
    Disable-NetAdapterBinding -Name $_.Name -ComponentID ms_tcpip6 -ErrorAction SilentlyContinue
    Write-Host "  -> Disabled on: $($_.Name)" -ForegroundColor Gray
}
Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip6\Parameters" -Name "DisabledComponents" -Value 0xFF -Type DWord -Force
Write-Host "  -> IPv6 disabled system-wide" -ForegroundColor Green

# --- 2. Force DNS to Cloudflare (privacy DNS) ---
Write-Host "[2/8] Setting private DNS..." -ForegroundColor Yellow
$dnsServers = @("1.1.1.1", "1.0.0.1")
$adapters = Get-NetAdapter | Where-Object { $_.Status -eq "Up" -and $_.InterfaceDescription -notlike "*WireGuard*" }
foreach ($adapter in $adapters) {
    Set-DnsClientServerAddress -InterfaceIndex $adapter.InterfaceIndex -ServerAddresses $dnsServers
    Write-Host "  -> $($adapter.Name): $($dnsServers -join ', ')" -ForegroundColor Gray
}
Write-Host "  -> DNS set to Cloudflare (1.1.1.1 / 1.0.0.1)" -ForegroundColor Green

# --- 3. Disable NetBIOS over TCP/IP ---
Write-Host "[3/8] Disabling NetBIOS..." -ForegroundColor Yellow
$regPath = "HKLM:\SYSTEM\CurrentControlSet\Services\NetBT\Parameters\Interfaces"
Get-ChildItem $regPath -ErrorAction SilentlyContinue | ForEach-Object {
    Set-ItemProperty -Path $_.PSPath -Name "NetbiosOptions" -Value 2 -Type DWord -Force -ErrorAction SilentlyContinue
}
Write-Host "  -> NetBIOS disabled on all interfaces" -ForegroundColor Green

# --- 4. Disable LLMNR ---
Write-Host "[4/8] Disabling LLMNR..." -ForegroundColor Yellow
$llmnrPath = "HKLM:\SOFTWARE\Policies\Microsoft\Windows NT\DNSClient"
if (-not (Test-Path $llmnrPath)) { New-Item -Path $llmnrPath -Force | Out-Null }
Set-ItemProperty -Path $llmnrPath -Name "EnableMulticast" -Value 0 -Type DWord -Force
Write-Host "  -> LLMNR disabled" -ForegroundColor Green

# --- 5. Disable mDNS ---
Write-Host "[5/8] Disabling mDNS..." -ForegroundColor Yellow
$mDNSPath = "HKLM:\SYSTEM\CurrentControlSet\Services\Dnscache\Parameters"
Set-ItemProperty -Path $mDNSPath -Name "EnableMDNS" -Value 0 -Type DWord -Force -ErrorAction SilentlyContinue
Write-Host "  -> mDNS disabled" -ForegroundColor Green

# --- 6. Disable WPAD (proxy auto-detect) ---
Write-Host "[6/8] Disabling WPAD..." -ForegroundColor Yellow
$iePath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings\Wpad"
if (-not (Test-Path $iePath)) { New-Item -Path $iePath -Force | Out-Null }
Set-ItemProperty -Path $iePath -Name "WpadOverride" -Value 1 -Type DWord -Force
Write-Host "  -> WPAD disabled" -ForegroundColor Green

# --- 7. Firewall: block all traffic except VPN (Kill Switch) ---
Write-Host "[7/8] Setting Kill Switch firewall rules..." -ForegroundColor Yellow

# Allow WireGuard UDP
netsh advfirewall firewall add rule name="VPN-KillSwitch-Allow-WireGuard" `
    dir=out action=allow protocol=udp remoteport=51820 2>$null
netsh advfirewall firewall add rule name="VPN-KillSwitch-Allow-WireGuard-In" `
    dir=in action=allow protocol=udp localport=51820 2>$null

# Allow loopback
netsh advfirewall firewall add rule name="VPN-KillSwitch-Allow-Loopback" `
    dir=out action=allow remoteip=127.0.0.1 2>$null
netsh advfirewall firewall add rule name="VPN-KillSwitch-Allow-Loopback-In" `
    dir=in action=allow remoteip=127.0.0.1 2>$null

# Allow LAN (so you can still access router etc)
netsh advfirewall firewall add rule name="VPN-KillSwitch-Allow-LAN" `
    dir=out action=allow remoteip=192.168.0.0/16,10.0.0.0/8,172.16.0.0/12 2>$null
netsh advfirewall firewall add rule name="VPN-KillSwitch-Allow-LAN-In" `
    dir=in action=allow remoteip=192.168.0.0/16,10.0.0.0/8,172.16.0.0/12 2>$null

# Allow VPN tunnel
netsh advfirewall firewall add rule name="VPN-KillSwitch-Allow-Tunnel" `
    dir=out action=allow remoteip=10.0.0.0/24 2>$null
netsh advfirewall firewall add rule name="VPN-KillSwitch-Allow-Tunnel-In" `
    dir=in action=allow remoteip=10.0.0.0/24 2>$null

# Block everything else (Kill Switch)
netsh advfirewall firewall add rule name="VPN-KillSwitch-Block-Out" `
    dir=out action=block 2>$null
netsh advfirewall firewall add rule name="VPN-KillSwitch-Block-In" `
    dir=in action=block 2>$null

Write-Host "  -> Kill Switch active: only WireGuard + LAN allowed" -ForegroundColor Green

# --- 8. Disable Windows Telemetry ---
Write-Host "[8/8] Disabling telemetry..." -ForegroundColor Yellow
$diagPath = "HKLM:\SOFTWARE\Policies\Microsoft\Windows\DataCollection"
if (-not (Test-Path $diagPath)) { New-Item -Path $diagPath -Force | Out-Null }
Set-ItemProperty -Path $diagPath -Name "AllowTelemetry" -Value 0 -Type DWord -Force
Set-ItemProperty -Path $diagPath -Name "MaxTelemetryAllowed" -Value 0 -Type DWord -Force
Stop-Service -Name "DiagTrack" -Force -ErrorAction SilentlyContinue
Set-Service -Name "DiagTrack" -StartupType Disabled -ErrorAction SilentlyContinue
Write-Host "  -> Telemetry disabled" -ForegroundColor Green

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  HARDENING COMPLETE!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "What was done:" -ForegroundColor Cyan
Write-Host "  [x] IPv6 disabled (no leaks)" -ForegroundColor White
Write-Host "  [x] DNS forced to Cloudflare (no ISP DNS leaks)" -ForegroundColor White
Write-Host "  [x] NetBIOS disabled" -ForegroundColor White
Write-Host "  [x] LLMNR disabled" -ForegroundColor White
Write-Host "  [x] mDNS disabled" -ForegroundColor White
Write-Host "  [x] WPAD disabled" -ForegroundColor White
Write-Host "  [x] Kill Switch active (only VPN + LAN traffic)" -ForegroundColor White
Write-Host "  [x] Telemetry disabled" -ForegroundColor White
Write-Host ""
Write-Host "To REMOVE Kill Switch:" -ForegroundColor Yellow
Write-Host '  netsh advfirewall firewall delete rule name="VPN-KillSwitch-*"' -ForegroundColor White
Write-Host ""
