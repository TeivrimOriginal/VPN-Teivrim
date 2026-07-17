# ============================================
# Revert Windows Hardening
# ============================================
# Run as Administrator!
# ============================================

Write-Host "========================================" -ForegroundColor Yellow
Write-Host "  Reverting Hardening..." -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow
Write-Host ""

# Re-enable IPv6
Write-Host "Re-enabling IPv6..." -ForegroundColor Gray
Get-NetAdapterBinding -ComponentID ms_tcpip6 -ErrorAction SilentlyContinue | ForEach-Object {
    Enable-NetAdapterBinding -Name $_.Name -ComponentID ms_tcpip6 -ErrorAction SilentlyContinue
}
Remove-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Services\Tcpip6\Parameters" -Name "DisabledComponents" -ErrorAction SilentlyContinue
Write-Host "  -> IPv6 re-enabled" -ForegroundColor Green

# Remove Kill Switch
Write-Host "Removing Kill Switch..." -ForegroundColor Gray
netsh advfirewall firewall delete rule name="VPN-KillSwitch-Allow-WireGuard" 2>$null
netsh advfirewall firewall delete rule name="VPN-KillSwitch-Allow-WireGuard-In" 2>$null
netsh advfirewall firewall delete rule name="VPN-KillSwitch-Allow-Loopback" 2>$null
netsh advfirewall firewall delete rule name="VPN-KillSwitch-Allow-Loopback-In" 2>$null
netsh advfirewall firewall delete rule name="VPN-KillSwitch-Allow-LAN" 2>$null
netsh advfirewall firewall delete rule name="VPN-KillSwitch-Allow-LAN-In" 2>$null
netsh advfirewall firewall delete rule name="VPN-KillSwitch-Allow-Tunnel" 2>$null
netsh advfirewall firewall delete rule name="VPN-KillSwitch-Allow-Tunnel-In" 2>$null
netsh advfirewall firewall delete rule name="VPN-KillSwitch-Block-Out" 2>$null
netsh advfirewall firewall delete rule name="VPN-KillSwitch-Block-In" 2>$null
Write-Host "  -> Kill Switch removed" -ForegroundColor Green

# Re-enable NetBIOS
Write-Host "Re-enabling NetBIOS..." -ForegroundColor Gray
$regPath = "HKLM:\SYSTEM\CurrentControlSet\Services\NetBT\Parameters\Interfaces"
Get-ChildItem $regPath -ErrorAction SilentlyContinue | ForEach-Object {
    Set-ItemProperty -Path $_.PSPath -Name "NetbiosOptions" -Value 0 -Type DWord -Force -ErrorAction SilentlyContinue
}
Write-Host "  -> NetBIOS re-enabled" -ForegroundColor Green

# Re-enable LLMNR
Write-Host "Re-enabling LLMNR..." -ForegroundColor Gray
Remove-ItemProperty -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows NT\DNSClient" -Name "EnableMulticast" -ErrorAction SilentlyContinue
Write-Host "  -> LLMNR re-enabled" -ForegroundColor Green

# Re-enable telemetry
Write-Host "Re-enabling telemetry..." -ForegroundColor Gray
Remove-ItemProperty -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows\DataCollection" -Name "AllowTelemetry" -ErrorAction SilentlyContinue
Remove-ItemProperty -Path "HKLM:\SOFTWARE\Policies\Microsoft\Windows\DataCollection" -Name "MaxTelemetryAllowed" -ErrorAction SilentlyContinue
Set-Service -Name "DiagTrack" -StartupType Manual -ErrorAction SilentlyContinue
Start-Service -Name "DiagTrack" -ErrorAction SilentlyContinue
Write-Host "  -> Telemetry re-enabled" -ForegroundColor Green

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "  All hardening reverted!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Note: DNS settings were not changed (still Cloudflare)" -ForegroundColor Yellow
Write-Host "To change DNS manually: Control Panel > Network > IPv4 > Properties" -ForegroundColor White
Write-Host ""
