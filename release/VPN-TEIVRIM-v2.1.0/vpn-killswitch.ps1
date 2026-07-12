# ============================================
# Kill Switch Toggle
# ============================================

param(
    [switch]$Enable,
    [switch]$Disable
)

if (-not $Enable -and -not $Disable) {
    Write-Host "Usage:" -ForegroundColor Yellow
    Write-Host "  .\vpn-killswitch.ps1 -Enable" -ForegroundColor White
    Write-Host "  .\vpn-killswitch.ps1 -Disable" -ForegroundColor White
    exit
}

if ($Enable) {
    Write-Host "Enabling Kill Switch..." -ForegroundColor Yellow

    # Allow WireGuard
    netsh advfirewall firewall add rule name="KS-Allow-WG" dir=out action=allow protocol=udp remoteport=51820 2>$null
    netsh advfirewall firewall add rule name="KS-Allow-WG-In" dir=in action=allow protocol=udp localport=51820 2>$null

    # Allow loopback
    netsh advfirewall firewall add rule name="KS-Allow-Loop" dir=out action=allow remoteip=127.0.0.1 2>$null
    netsh advfirewall firewall add rule name="KS-Allow-Loop-In" dir=in action=allow remoteip=127.0.0.1 2>$null

    # Allow LAN
    netsh advfirewall firewall add rule name="KS-Allow-LAN" dir=out action=allow remoteip=192.168.0.0/16,10.0.0.0/8,172.16.0.0/12 2>$null
    netsh advfirewall firewall add rule name="KS-Allow-LAN-In" dir=in action=allow remoteip=192.168.0.0/16,10.0.0.0/8,172.16.0.0/12 2>$null

    # Allow VPN tunnel
    netsh advfirewall firewall add rule name="KS-Allow-Tunnel" dir=out action=allow remoteip=10.0.0.0/24 2>$null
    netsh advfirewall firewall add rule name="KS-Allow-Tunnel-In" dir=in action=allow remoteip=10.0.0.0/24 2>$null

    # Block everything
    netsh advfirewall firewall add rule name="KS-Block-All" dir=out action=block 2>$null
    netsh advfirewall firewall add rule name="KS-Block-All-In" dir=in action=block 2>$null

    Write-Host "Kill Switch ENABLED" -ForegroundColor Green
    Write-Host "Only WireGuard + LAN traffic allowed" -ForegroundColor White
}

if ($Disable) {
    Write-Host "Disabling Kill Switch..." -ForegroundColor Yellow
    netsh advfirewall firewall delete rule name="KS-Allow-WG" 2>$null
    netsh advfirewall firewall delete rule name="KS-Allow-WG-In" 2>$null
    netsh advfirewall firewall delete rule name="KS-Allow-Loop" 2>$null
    netsh advfirewall firewall delete rule name="KS-Allow-Loop-In" 2>$null
    netsh advfirewall firewall delete rule name="KS-Allow-LAN" 2>$null
    netsh advfirewall firewall delete rule name="KS-Allow-LAN-In" 2>$null
    netsh advfirewall firewall delete rule name="KS-Allow-Tunnel" 2>$null
    netsh advfirewall firewall delete rule name="KS-Allow-Tunnel-In" 2>$null
    netsh advfirewall firewall delete rule name="KS-Block-All" 2>$null
    netsh advfirewall firewall delete rule name="KS-Block-All-In" 2>$null
    Write-Host "Kill Switch DISABLED" -ForegroundColor Green
    Write-Host "All traffic allowed" -ForegroundColor White
}
