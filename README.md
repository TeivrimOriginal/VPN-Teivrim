# VPN-TEIVRIM

WireGuard VPN Server for Windows.

## Setup

1. Run `vpn-setup.bat` as Administrator
2. Forward UDP port 51820 on your router to your LAN IP
3. Install WireGuard on phone/PC and import `C:\WireGuard\client0.conf`

## Files

| File | Description |
|------|-------------|
| `vpn-gui.exe` | GUI monitor with Start/Stop/Add Client buttons |
| `vpn-gui.cpp` | Source code for the GUI (compile with MinGW) |
| `vpn-setup.bat` | One-click server setup (installs WireGuard + generates keys) |
| `vpn-setup.ps1` | Setup script (PowerShell) |
| `vpn-add-client.ps1` | Generate config for a new client |
| `vpn-monitor.bat` | Console monitor with live stats |
| `vpn-monitor.ps1` | Console monitor script |
| `vpn-status.ps1` | Quick server status check |

## GUI

The `vpn-gui.exe` provides a dark-themed dashboard:
- Server ONLINE/OFFLINE status
- Public key, port, LAN IP
- Connected peers table with transfer stats
- Live log viewer
- Start/Stop server buttons
- Add Client / Open Log buttons

## Requirements

- Windows 10/11
- Administrator privileges
- UDP port 51820 forwarded on router
