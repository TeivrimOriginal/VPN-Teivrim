## VPN-TEIVRIM v2.1.0

### What's New
- CMake build system
- One-click installer (install.bat)
- Uninstaller (uninstall.bat)
- Autostart on Windows boot
- Background threads (no more crashes)
- Fixed all UI freezes

### Features
- WireGuard VPN Server Monitor (GUI)
- 3 Anonymity Levels (Basic / DNS Shield / Maximum)
- Kill Switch
- DNS Leak Test
- Full Windows Hardening
- Client Generator
- Export configs
- Uptime and Traffic stats

### Installation
1. Download VPN-TEIVRIM-v2.1.0-win64.zip
2. Extract anywhere
3. Run install.bat as Administrator
4. Forward UDP 51820 on router to your LAN IP
5. Import client config on phone

### Build from Source
```
cmake -B build -G "MinGW Makefiles"
cmake --build build
```
