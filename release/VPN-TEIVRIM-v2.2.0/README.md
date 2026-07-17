# VPN-TEIVRIM

Self-hosted WireGuard VPN Server for Windows with privacy hardening.

## Quick Start

```
vpn-full-setup.bat
```

Runs everything: installs WireGuard, creates server, hardens Windows, generates client config.

## Privacy Features

- **DNS Leak Protection** - All DNS forced through Cloudflare (1.1.1.1)
- **IPv6 Disabled** - No IPv6 leaks
- **Kill Switch** - Blocks all traffic except VPN + LAN
- **NetBIOS/LLMNR/mDNS Disabled** - No local network discovery leaks
- **Telemetry Disabled** - No Windows data collection
- **PresharedKey** - Quantum-resistant encryption layer
- **Full Tunnel** - All traffic (0.0.0.0/0) routed through VPN
- **MTU Optimized** - 1420 for encrypted packets

## Files

### Setup
| File | Description |
|------|-------------|
| `vpn-full-setup.bat` | **One-click full setup** (server + hardening + client) |
| `vpn-setup.bat` | Basic WireGuard installation + server |
| `vpn-privacy-setup.ps1` | Privacy-optimized server setup |
| `vpn-setup.ps1` | Basic setup script |

### Privacy
| File | Description |
|------|-------------|
| `vpn-harden.ps1` | Windows hardening (IPv6, DNS, Kill Switch, Telemetry) |
| `vpn-killswitch.ps1` | Toggle Kill Switch on/off |
| `vpn-leaktest.ps1` | Test for DNS/WebRTC/IP leaks |

### Clients
| File | Description |
|------|-------------|
| `vpn-privacy-addclient.ps1` | Generate privacy-optimized client config |
| `vpn-add-client.ps1` | Basic client generator |

### Monitoring
| File | Description |
|------|-------------|
| `vpn-gui.exe` | GUI dashboard (dark theme) |
| `vpn-gui.cpp` | GUI source code |
| `vpn-monitor.bat` | Console monitor |
| `vpn-monitor.ps1` | Console monitor script |
| `vpn-status.ps1` | Quick status check |

## GUI Dashboard

Dark-themed real-time monitor:
- Server ONLINE/OFFLINE status
- Public key, port, LAN IP
- Connected peers with transfer stats
- Live log viewer
- Start/Stop/Add Client buttons
- Auto-refresh every 5 seconds

## Manual Setup

1. Run `vpn-privacy-setup.ps1` as Administrator
2. Run `vpn-harden.ps1` for full Windows hardening
3. Forward UDP 51820 on router to your LAN IP
4. Import `C:\WireGuard\client0.conf` on phone/PC
5. Test: https://dnsleaktest.com

## Kill Switch

```powershell
# Enable (blocks all traffic except VPN)
.\vpn-killswitch.ps1 -Enable

# Disable (restore normal traffic)
.\vpn-killswitch.ps1 -Disable
```

## Requirements

- Windows 10/11
- Administrator privileges
- UDP port 51820 forwarded on router
- WireGuard client on phone/PC
