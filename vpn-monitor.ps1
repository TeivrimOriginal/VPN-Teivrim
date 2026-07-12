$wgExe = "C:\Program Files\WireGuard\wg.exe"
$logFile = "C:\WireGuard\vpn-monitor.log"

if (-not (Test-Path $logFile)) { New-Item -Path $logFile -ItemType File -Force | Out-Null }

function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $entry = "[$ts] [$Level] $Message"
    Add-Content -Path $logFile -Value $entry -Encoding UTF8
}

function Show-Header {
    Write-Host ""
    Write-Host "  ==========================================" -ForegroundColor Cyan
    Write-Host "     WireGuard VPN Server Monitor" -ForegroundColor Cyan
    Write-Host "  ==========================================" -ForegroundColor Cyan
    Write-Host ""
}

function Show-ServerStatus {
    $svc = Get-Service -Name "WireGuard*" -ErrorAction SilentlyContinue
    if ($svc -and $svc.Status -eq "Running") {
        Write-Host "  Server: RUNNING" -ForegroundColor Green
    } else {
        Write-Host "  Server: STOPPED" -ForegroundColor Red
    }
    Write-Host ""
}

function Show-InterfaceInfo {
    $out = & $wgExe show wg0 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  Interface: ERROR reading" -ForegroundColor Red
        return
    }
    $port = ""
    $pk = ""
    foreach ($line in $out) {
        $s = $line.ToString().Trim()
        if ($s.StartsWith("listening port:")) { $port = $s.Split(":")[1].Trim() }
        if ($s.StartsWith("public key:")) { $pk = $s.Split(":")[1].Trim() }
    }
    Write-Host "  --- Interface ---" -ForegroundColor DarkCyan
    Write-Host "  Public Key:     $pk" -ForegroundColor Yellow
    Write-Host "  Listening Port: $port" -ForegroundColor Yellow
    Write-Host ""
}

function Show-Peers {
    $out = & $wgExe show wg0 2>&1
    if ($LASTEXITCODE -ne 0) { return }

    Write-Host "  --- Peers ---" -ForegroundColor DarkCyan

    $inPeer = $false
    $peerKey = ""
    $peerEndpoint = ""
    $peerAllowed = ""
    $peerTransfer = ""
    $peerHandshake = ""
    $anyPeer = $false

    foreach ($line in $out) {
        $s = $line.ToString().Trim()

        if ($s.StartsWith("peer:")) {
            if ($inPeer -and $peerKey -ne "") {
                $anyPeer = $true
                $short = $peerKey.Substring(0, [Math]::Min(16, $peerKey.Length))
                $connected = if ($peerHandshake -ne "") { "CONNECTED" } else { "WAITING" }
                $cc = if ($connected -eq "CONNECTED") { "Green" } else { "Yellow" }

                Write-Host "  Peer: $short..." -ForegroundColor White
                Write-Host "    Status:       $connected" -ForegroundColor $cc
                if ($peerEndpoint -ne "") { Write-Host "    Endpoint:     $peerEndpoint" -ForegroundColor White }
                if ($peerAllowed -ne "") { Write-Host "    Allowed IPs:  $peerAllowed" -ForegroundColor White }
                if ($peerTransfer -ne "") { Write-Host "    Transfer:     $peerTransfer" -ForegroundColor White }
                if ($peerHandshake -ne "") { Write-Host "    Handshake:    $peerHandshake" -ForegroundColor White }
                Write-Host ""
            }
            $inPeer = $true
            $peerKey = ""
            $peerEndpoint = ""
            $peerAllowed = ""
            $peerTransfer = ""
            $peerHandshake = ""
        }

        if ($inPeer) {
            if ($s.StartsWith("public key:")) { $peerKey = $s.Substring(11).Trim() }
            if ($s.StartsWith("endpoint:")) { $peerEndpoint = $s.Substring(9).Trim() }
            if ($s.StartsWith("allowed ips:")) { $peerAllowed = $s.Substring(12).Trim() }
            if ($s.StartsWith("transfer:")) { $peerTransfer = $s.Substring(9).Trim() }
            if ($s.StartsWith("latest handshake:")) { $peerHandshake = $s.Substring(17).Trim() }
        }
    }

    if ($inPeer -and $peerKey -ne "") {
        $anyPeer = $true
        $short = $peerKey.Substring(0, [Math]::Min(16, $peerKey.Length))
        $connected = if ($peerHandshake -ne "") { "CONNECTED" } else { "WAITING" }
        $cc = if ($connected -eq "CONNECTED") { "Green" } else { "Yellow" }

        Write-Host "  Peer: $short..." -ForegroundColor White
        Write-Host "    Status:       $connected" -ForegroundColor $cc
        if ($peerEndpoint -ne "") { Write-Host "    Endpoint:     $peerEndpoint" -ForegroundColor White }
        if ($peerAllowed -ne "") { Write-Host "    Allowed IPs:  $peerAllowed" -ForegroundColor White }
        if ($peerTransfer -ne "") { Write-Host "    Transfer:     $peerTransfer" -ForegroundColor White }
        if ($peerHandshake -ne "") { Write-Host "    Handshake:    $peerHandshake" -ForegroundColor White }
        Write-Host ""
    }

    if (-not $anyPeer) {
        Write-Host "  No peers configured" -ForegroundColor DarkGray
        Write-Host ""
    }
}

function Show-Network {
    Write-Host "  --- Network ---" -ForegroundColor DarkCyan
    $adapters = Get-NetAdapter -ErrorAction SilentlyContinue | Where-Object { $_.Status -eq "Up" }
    foreach ($a in $adapters) {
        $ip = (Get-NetIPAddress -InterfaceIndex $a.InterfaceIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue).IPAddress
        if ($ip) {
            Write-Host "  $($a.Name): $ip" -ForegroundColor White
        }
    }
    Write-Host ""
}

function Show-LogTail {
    Write-Host "  --- Log (last 5) ---" -ForegroundColor DarkCyan
    if (Test-Path $logFile) {
        $lines = Get-Content $logFile -Tail 5 -ErrorAction SilentlyContinue
        foreach ($l in $lines) {
            Write-Host "  $l" -ForegroundColor DarkGray
        }
    }
    Write-Host ""
}

function Show-Controls {
    Write-Host "  [R] Refresh  [Q] Quit  [L] Open Log File" -ForegroundColor White
    Write-Host ""
}

Write-Log "Monitor started"

while ($true) {
    Clear-Host
    Show-Header
    Show-ServerStatus
    Show-InterfaceInfo
    Show-Peers
    Show-Network
    Show-LogTail
    Show-Controls

    $now = Get-Date -Format "HH:mm:ss"
    Write-Host "  Updated: $now" -ForegroundColor DarkGray

    if ([Console]::KeyAvailable) {
        $key = [Console]::ReadKey($true)
        if ($key.Key -eq "Q") {
            Write-Log "Monitor stopped"
            Write-Host ""
            Write-Host "  Bye!" -ForegroundColor Cyan
            break
        }
        if ($key.Key -eq "L") {
            Start-Process notepad.exe $logFile
        }
    }

    Start-Sleep -Seconds 3
}
