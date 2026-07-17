## VPN-TEIVRIM v2.2.0

### What's New
- Real-time throughput (speed): shows current ↓/↑ rate (B/s, KiB/s, MiB/s) alongside cumulative traffic
- System tray icon: minimize to tray, double-click to restore, right-click menu (Показать / Выход)
- Fixed duplicate autostart (was launching twice → flashed "already running" window)
- Single-instance now silently focuses the existing window instead of popping a message box
- Crash handler logs to C:\VPN-TEIVRIM\crash.log instead of dying silently
- Fixed traffic parsing offsets (correct per-peer and total values)

### Features
- WireGuard VPN Server Monitor (GUI + tray)
- 3 Anonymity Levels (Basic / DNS Shield / Maximum)
- Kill Switch
- DNS Leak Test
- Full Windows Hardening
- Client Generator
- Export configs
- Uptime, Traffic and Live Speed stats

### Installation
1. Download VPN-TEIVRIM-v2.2.0-win64.zip
2. Extract anywhere
3. Run install.bat as Administrator
4. Forward UDP 51820 on router to your LAN IP
5. Import client config on phone

### Build from Source
```
cmake -B build -G "MinGW Makefiles"
cmake --build build
```
