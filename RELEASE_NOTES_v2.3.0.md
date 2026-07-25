# VPN-TEIVRIM v2.3.0 Release Notes

## 🎯 Critical Fixes
- **Fixed COMCTL32 v6 ListView crash** — Replaced SysListView32 with standard ListBox (user32 control) to eliminate unfixable AV in comctl32.dll v6 subclass chain
- **Fixed ShowQRDialog heap overflow** — 1-byte buffer overflow in WideCharToMultiByte (n bytes into n-1 buffer) that corrupted heap and triggered delayed crashes
- **Fixed CreateWindowEx dwExStyle bug** — Fixed incorrect parameter order causing WS_EX_CLIENTEDGE to be passed as class name

## 🔧 Stability Improvements
- **CrashFilter enhanced** — Now captures stack traces with module+offset using CaptureStackBackTrace + VirtualQuery + GetModuleFileNameW (requires -lpsapi)
- **All MessageBox owners → NULL** — Prevents WM_ENABLE cascade that disabled child controls during modal dialogs
- **DoRefresh guarded by IsWindowEnabled** — Skips refresh when main window disabled by modal dialogs
- **Selection persistence** — g_selPeer tracks ListBox selection across refreshes via LBN_SELCHANGE

## 🎨 UI/UX Changes
- **ListView → ListBox migration** — Native themed ListBox (WS_CHILD|WS_VISIBLE|WS_VSCROLL|LBS_NOTIFY|LBS_HASSTRINGS) replaces comctl32 ListView
- **QR Code display fixed** — ShowQRDialog now runs on main thread via WM_USER+102; QRWin class registered once, persists until user closes
- **Selection via LBN_SELCHANGE** — Replaced WM_NOTIFY/NM_CLICK with proper ListBox notification
- **Remove button robust** — Falls back to LB_GETCURSEL if g_selPeer unset

## 🛠 Build/Deploy
- **Static linking** — Fully statically linked (-static -O2 -s) ~1.1MB binary
- **Manifest v6** — app.manifest + manifest.rc for common-controls v6 (DPI awareness)
- **Autostart cleanup** — install.bat removes legacy Startup shortcut, uses single HKLM\Run entry
- **Static CRT** — No VC++ redistributable required

## 🐛 Testing Results
- **Stress test**: 10 cycles Add→QR→Remove with 0 crashes
- **60s stability**: GUI running with active selection + timer refreshes, no crash.log
- **Release binary**: vpn-gui-release.exe (1.1MB static) runs without crash
- **Config management**: Clean wg0.conf (0 peers) after test cycles

## 📦 Deploy
`cmd
# Install
install.bat

# Or manual
copy vpn-gui-release.exe C:\VPN-TEIVRIM\vpn-gui.exe
sc create VPN-TEIVRIM binPath= "C:\VPN-TEIVRIM\vpn-gui.exe" start= auto
`

## 🔗 Requirements
- Windows 10/11 x64
- WireGuard service installed
- Admin rights for install
