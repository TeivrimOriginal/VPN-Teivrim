# ============================================
# DNS Leak Test
# ============================================

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  DNS Leak Test" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

Write-Host "Testing DNS resolvers..." -ForegroundColor Yellow
Write-Host ""

# Test 1: Check DNS servers
Write-Host "  [1] Your DNS servers:" -ForegroundColor White
$dns = Get-DnsClientServerAddress -AddressFamily IPv4 | Where-Object { $_.ServerAddresses }
foreach ($d in $dns) {
    foreach ($addr in $d.ServerAddresses) {
        Write-Host "      $($d.InterfaceAlias): $addr" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "  [2] Testing against public DNS..." -ForegroundColor White

# Test DNS resolution
$testDomains = @("google.com", "github.com", "cloudflare.com")
foreach ($domain in $testDomains) {
    $result = Resolve-DnsName -Name $domain -Type A -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($result) {
        Write-Host "      $domain -> $($result.IPAddress)" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "  [3] Checking for DNS leaks..." -ForegroundColor White

# Check if DNS is going through VPN
$wgDNS = @("1.1.1.1", "1.0.0.1", "8.8.8.8", "8.8.4.4")
$localDNS = Get-DnsClientServerAddress -AddressFamily IPv4 | ForEach-Object { $_.ServerAddresses } | Sort-Object -Unique

$isLeaking = $false
foreach ($dns in $localDNS) {
    if ($dns -notin $wgDNS -and $dns -ne "127.0.0.1") {
        Write-Host "      [!] POTENTIAL LEAK: $dns" -ForegroundColor Red
        $isLeaking = $true
    }
}

if (-not $isLeaking) {
    Write-Host "      [OK] DNS servers look clean" -ForegroundColor Green
}

Write-Host ""
Write-Host "  [4] WebRTC leak check..." -ForegroundColor White
Write-Host "      Open: https://browserleaks.com/webrtc" -ForegroundColor Gray
Write-Host "      Your public IP should show VPN IP, not real IP" -ForegroundColor Gray

Write-Host ""
Write-Host "  [5] IP leak check..." -ForegroundColor White
Write-Host "      Open: https://whatismyipaddress.com" -ForegroundColor Gray
Write-Host "      Should show VPN server IP" -ForegroundColor Gray

Write-Host ""
Write-Host "  [6] DNS leak test..." -ForegroundColor White
Write-Host "      Open: https://dnsleaktest.com" -ForegroundColor Gray
Write-Host "      Run extended test - should show VPN DNS servers only" -ForegroundColor Gray

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Recommended:" -ForegroundColor Cyan
Write-Host "    - Run VPN and test at dnsleaktest.com" -ForegroundColor White
Write-Host "    - Use browser extensions to block WebRTC" -ForegroundColor White
Write-Host "    - Use HTTPS Everywhere" -ForegroundColor White
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
