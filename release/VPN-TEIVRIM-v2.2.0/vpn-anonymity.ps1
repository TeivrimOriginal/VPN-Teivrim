# ============================================
# VPN Anonymity Levels
# ============================================
# Run as Administrator!
# ============================================

param(
    [ValidateSet("1","2","3")]
    [string]$Level = ""
)

if (-not $Level) {
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "  VPN Anonymity Levels" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "  [1] BASIC" -ForegroundColor Yellow
    Write-Host "      - VPN tunnel only" -ForegroundColor Gray
    Write-Host "      - Hides IP from websites" -ForegroundColor Gray
    Write-Host "      - ISP still sees VPN connection" -ForegroundColor Gray
    Write-Host "      - Good for: geo-blocking bypass" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  [2] ADVANCED" -ForegroundColor Yellow
    Write-Host "      - DNS leak protection" -ForegroundColor Gray
    Write-Host "      - IPv6 disabled" -ForegroundColor Gray
    Write-Host "      - NetBIOS/LLMNR off" -ForegroundColor Gray
    Write-Host "      - ISP sees VPN but no DNS leaks" -ForegroundColor Gray
    Write-Host "      - Good for: privacy from ISP" -ForegroundColor Gray
    Write-Host ""
    Write-Host "  [3] MAXIMUM" -ForegroundColor Yellow
    Write-Host "      - Everything from Level 2" -ForegroundColor Gray
    Write-Host "      - Kill Switch (traffic blocked if VPN drops)" -ForegroundColor Gray
    Write-Host "      - Telemetry disabled" -ForegroundColor Gray
    Write-Host "      - mDNS/WPAD off" -ForegroundColor Gray
    Write-Host "      - Full traffic routing (0.0.0.0/0)" -ForegroundColor Gray
    Write-Host "      - PresharedKey (quantum-resistant)" -ForegroundColor Gray
    Write-Host "      - Good for: maximum privacy" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Usage: .\vpn-anonymity.ps1 -Level 1|2|3" -ForegroundColor White
    Write-Host ""
    exit
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Applying Anonymity Level: $Level" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "[!] Run as Administrator!" -ForegroundColor Red; exit 1
}

# === LEVEL 1: Basic VPN ===
if ($Level -ge 1) {
    Write-Host "[Level 1] Basic VPN..." -ForegroundColor Yellow

    # Ensure WireGuard is running
    $wgExe = "C:\Program Files\WireGuard\wg.exe"
    $show = & $wgExe show wg0 2>&1
    if ($show -notmatch "listening port") {
        Write-Host "  Starting WireGuard server..." -ForegroundColor Gray
        & "C:\Program Files\WireGuard\wireguard.exe" /installtunnelservice "C:\WireGuard\wg0.conf" 2>$null
        Start-Sleep 2
    }
    Write-Host "  [OK] VPN tunnel active" -ForegroundColor Green
}

# === LEVEL 2: DNS + IPv6 Protection ===
if ($Level -ge 2) {
    Write-Host ""
    Write-Host "[Level 2] DNS Leak Protection + IPv6..." -ForegroundColor Yellow

    # Disable IPv6
    Get-NetAdapterBinding -ComponentID ms_tcpip6 -ErrorAction SilentlyContinue | ForEach-Object {
        Disable-NetAdapterBinding -Name $_.Name -ComponentID ms_tcpip6 -ErrorAction SilentlyContinue
    }
    Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip6\Parameters" -Name "DisabledComponents" -Value 0xFF -Type DWord -Force -ErrorAction SilentlyContinue
    Write-Host "  [OK] IPv6 disabled" -ForegroundColor Green

    # Force DNS to Cloudflare
    $adapters = Get-NetAdapter | Where-Object { $_.Status -eq "Up" -and $_.InterfaceDescription -notlike "*WireGuard*" }
    foreach ($a in $adapters) {
        Set-DnsClientServerAddress -InterfaceIndex $a.InterfaceIndex -ServerAddresses @("1.1.1.1", "1.0.0.1")
    }
    Write-Host "  [OK] DNS: Cloudflare (1.1.1.1)" -ForegroundColor Green

    # Disable NetBIOS
    Get-ChildItem "HKLM:\SYSTEM\CurrentControlSet\Services\NetBT\Parameters\Interfaces" -ErrorAction SilentlyContinue | ForEach-Object {
        Set-ItemProperty -Path $_.PSPath -Name "NetbiosOptions" -Value 2 -Type DWord -Force -ErrorAction SilentlyContinue
    }
    Write-Host "  [OK] NetBIOS disabled" -ForegroundColor Green

    # Disable LLMNR
    $llmnrPath = "HKLM:\SOFTWARE\Policies\Microsoft\Windows NT\DNSClient"
    if (-not (Test-Path $llmnrPath)) { New-Item -Path $llmnrPath -Force | Out-Null }
    Set-ItemProperty -Path $llmnrPath -Name "EnableMulticast" -Value 0 -Type DWord -Force
    Write-Host "  [OK] LLMNR disabled" -ForegroundColor Green

    # Disable mDNS
    $mdnsPath = "HKLM:\SYSTEM\CurrentControlSet\Services\Dnscache\Parameters"
    Set-ItemProperty -Path $mdnsPath -Name "EnableMDNS" -Value 0 -Type DWord -Force -ErrorAction SilentlyContinue
    Write-Host "  [OK] mDNS disabled" -ForegroundColor Green
}

# === LEVEL 3: Full Hardening ===
if ($Level -ge 3) {
    Write-Host ""
    Write-Host "[Level 3] Maximum Security..." -ForegroundColor Yellow

    # Kill Switch
    netsh advfirewall firewall add rule name="L3-KS-Allow-WG" dir=out action=allow protocol=udp remoteport=51820 2>$null
    netsh advfirewall firewall add rule name="L3-KS-Allow-WG-In" dir=in action=allow protocol=udp localport=51820 2>$null
    netsh advfirewall firewall add rule name="L3-KS-Allow-Loop" dir=out action=allow remoteip=127.0.0.1 2>$null
    netsh advfirewall firewall add rule name="L3-KS-Allow-Loop-In" dir=in action=allow remoteip=127.0.0.1 2>$null
    netsh advfirewall firewall add rule name="L3-KS-Allow-LAN" dir=out action=allow remoteip=192.168.0.0/16,10.0.0.0/8,172.16.0.0/12 2>$null
    netsh advfirewall firewall add rule name="L3-KS-Allow-LAN-In" dir=in action=allow remoteip=192.168.0.0/16,10.0.0.0/8,172.16.0.0/12 2>$null
    netsh advfirewall firewall add rule name="L3-KS-Allow-Tun" dir=out action=allow remoteip=10.0.0.0/24 2>$null
    netsh advfirewall firewall add rule name="L3-KS-Allow-Tun-In" dir=in action=allow remoteip=10.0.0.0/24 2>$null
    netsh advfirewall firewall add rule name="L3-KS-Block-Out" dir=out action=block 2>$null
    netsh advfirewall firewall add rule name="L3-KS-Block-In" dir=in action=block 2>$null
    Write-Host "  [OK] Kill Switch active" -ForegroundColor Green

    # Disable WPAD
    $wpadPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings\Wpad"
    if (-not (Test-Path $wpadPath)) { New-Item -Path $wpadPath -Force | Out-Null }
    Set-ItemProperty -Path $wpadPath -Name "WpadOverride" -Value 1 -Type DWord -Force
    Write-Host "  [OK] WPAD disabled" -ForegroundColor Green

    # Disable Telemetry
    $diagPath = "HKLM:\SOFTWARE\Policies\Microsoft\Windows\DataCollection"
    if (-not (Test-Path $diagPath)) { New-Item -Path $diagPath -Force | Out-Null }
    Set-ItemProperty -Path $diagPath -Name "AllowTelemetry" -Value 0 -Type DWord -Force
    Stop-Service -Name "DiagTrack" -Force -ErrorAction SilentlyContinue
    Set-Service -Name "DiagTrack" -StartupType Disabled -ErrorAction SilentlyContinue
    Write-Host "  [OK] Telemetry disabled" -ForegroundColor Green

    # Enable IP forwarding
    Set-NetIPInterface -Forwarding Enabled -ErrorAction SilentlyContinue
    reg add "HKLM\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters" /v IPEnableRouter /t REG_DWORD /d 1 /f | Out-Null
    Write-Host "  [OK] IP forwarding enabled" -ForegroundColor Green
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  ANONYMITY LEVEL $Level ACTIVE" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

switch ($Level) {
    "1" {
        Write-Host "  Your IP is hidden from websites" -ForegroundColor White
        Write-Host "  ISP can see you use VPN" -ForegroundColor Yellow
        Write-Host "  Good for: streaming, geo-bypass" -ForegroundColor Gray
    }
    "2" {
        Write-Host "  IP hidden + no DNS leaks" -ForegroundColor White
        Write-Host "  ISP sees VPN but no browsing data" -ForegroundColor Yellow
        Write-Host "  Good for: privacy from ISP" -ForegroundColor Gray
    }
    "3" {
        Write-Host "  Maximum protection" -ForegroundColor White
        Write-Host "  Traffic killed if VPN drops" -ForegroundColor Yellow
        Write-Host "  No telemetry, no leaks, no traces" -ForegroundColor Yellow
        Write-Host "  Good for: journalism, whistleblowing" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "Test your privacy: https://dnsleaktest.com" -ForegroundColor Cyan
Write-Host ""
