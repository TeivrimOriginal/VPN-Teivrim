# VPN-TEIVRIM v2.4.0 — One-Click Privacy VPN

Self-hosted WireGuard VPN Server for Windows 10/11. Один .exe, мастер в 3 шага, QR в 1 клик. 1-5 пиров, без зависимостей.

## Quick Start (One-Click)

**Вариант A — Installer (рекомендуется):**
```
1. Скачать VPN-TEIVRIM-v2.4.0-setup.exe
2. Запустить от имени администратора → Next-Next-Finish
3. Мастер 3 шага: Install → Port Forward → QR
```

**Вариант B — ZIP:**
```
1. Скачать VPN-TEIVRIM-v2.4.0-win64.zip → распаковать
2. Запустить build/VPN-TEIVRIM.exe (или install.bat)
3. Кнопка "Мастер" в GUI
```

**Вариант C — Legacy:**
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
| `VPN-TEIVRIM-v2.4.0-setup.exe` | **One-Click Installer** (Inno Setup, один exe) |
| `VPN-TEIVRIM.exe` | GUI 1.1MB static (модули src/exec,config,peers,privacy,wizard) |
| `vpn-full-setup.bat` | **Legacy one-click** (server + hardening + client) |
| `vpn-setup.bat` | Basic WireGuard installation + server |
| `vpn-privacy-setup.ps1` | Privacy-optimized server setup (встроен в C++ `src/privacy.cpp`) |
| `vpn-setup.ps1` | Basic setup script |

### Privacy
| File | Description |
|------|-------------|
| `vpn-harden.ps1` | Windows hardening (встроен в `src/privacy.cpp:122` ApplyHarden) |
| `vpn-killswitch.ps1` | Toggle Kill Switch (встроен в `src/killswitch.cpp:6` v2 persistent) |
| `vpn-leaktest.ps1` | Test for DNS/WebRTC/IP leaks |
| `src/privacy.cpp` | C++ перенос vpn-anonymity/harden (без ExecutionPolicy) |
| `src/killswitch.cpp` | Kill Switch v2 + `HKLM\Run` переживает ребут |

### Clients
| File | Description |
|------|-------------|
| `vpn-privacy-addclient.ps1` | Generate privacy-optimized client config |
| `vpn-add-client.ps1` | Basic client generator |

### Monitoring
| File | Description |
|------|-------------|
| `vpn-gui.exe` | Legacy GUI (v2.3) |
| `VPN-TEIVRIM.exe` | **v2.4 GUI** — 959 строк + `src/wizard.cpp:144` мастер, QR в 1 клик, sparkline |
| `vpn-gui.cpp` | GUI source (рефактор src/exec,config,peers) |
| `vpn-monitor.bat` | Console monitor |
| `vpn-monitor.ps1` | Console monitor script |
| `vpn-status.ps1` | Quick status check |

## GUI Dashboard v2.4.0

Dark-themed real-time monitor + Master 3 шага:
- **Мастер** `src/wizard.cpp:58` — 1 Install (ген keys `wg genkey`), 2 Port Forward (LAN/WAN), 3 QR
- Server ONLINE/OFFLINE `vpn-gui.cpp:216` + `IsWireGuardInstalled()` `src/privacy.cpp:86`
- Public key, port, LAN/WAN IP `FetchPublicIpThread` `vpn-gui.cpp:111`
- Connected peers `src/peers.cpp` + transfer `ParseSize` `src/exec.cpp`
- Live log `ReadLog` `src/config.cpp`, DPAPI `ProtectFileDPAPI` `src/config.cpp:108`
- Start/Stop/Add Client/QR Copy/Remove + Kill Switch v2 `src/killswitch.cpp`
- Sparkline трафика `DrawSparkline`, tray `WM_TRAYICON`
- Auto-refresh 5s, single-instance mutex `wWinMain` `vpn-gui.cpp:910`

## Manual Setup (v2.4 no-PS — все в C++)

1. Запусти `VPN-TEIVRIM.exe` → Мастер Шаг 1 (авто `SetupPrivacyServer` `src/privacy.cpp:137` если `wg0.conf` нет)
2. Шаг 2 — пробрось UDP 51820 на роутере `192.168.0.1` → LAN IP (`g_lanIp` `vpn-gui.cpp:106`)
3. Шаг 3 — QR/Копировать `client0.conf` `ShowQRDialog`
4. Или кнопки: `1: Basic`/`2: +DNS Shield`/`3: MAXIMUM` `ApplyAnonymityLevel` `src/privacy.cpp:93` / `Full Harden` `ApplyHarden`
5. Test: https://dnsleaktest.com + `vpn-leaktest.ps1`

Legacy PS остался для совместимости, но GUI больше не требует `ExecutionPolicy Bypass`.

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
