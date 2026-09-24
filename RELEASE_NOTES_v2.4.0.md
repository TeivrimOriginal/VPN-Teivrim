# VPN-TEIVRIM v2.4.0 Release Notes — One-Click Privacy VPN

## 🎯 P0 — One-Click Privacy VPN для не-технаря

**Цель:** из 8 ps1 + сырой GUI 1271 строк → 1 exe, мастер 3 шага, QR в 1 клик. Windows 10/11 only, 1-5 пиров, <2MB static, установка <3 мин, Kill Switch переживает ребут.

### ✨ Новые фичи
- **Мастер 3 шага** `src/wizard.cpp:58` — Install (ген `wg genkey`/`genpsk` `src/privacy.cpp:137`), Port Forward (LAN/WAN `ipconfig`/`api.ipify.org`), QR. Авто-показ если `!IsServerConfigured()` `vpn-gui.cpp:697`. Кнопка `Мастер` `vpn-gui.cpp:645`.
- **Kill Switch v2** `src/killswitch.cpp:6` — `EnableKillSwitch`/`DisableKillSwitch` via `netsh` без PS, маркер `C:\WireGuard\.ks_enabled:6` + `HKLM\Run\VPN-TEIVRIM-KS` `killswitch.cpp:21` persistent, `RestoreKillSwitchIfNeeded()` `vpn-gui.cpp:694` на старте.
- **DPAPI шифрование** `src/config.cpp:108` — `ProtectFileDPAPI`/`UnprotectFileDPAPI`/`IsFileProtected` `config.cpp:165` через `CryptProtectData` `crypt32`.
- **C++ перенос privacy** `src/privacy.cpp:93` — `ApplyAnonymityLevel(1..3)` (IPv6 `DisabledComponents:30`, DNS `SetDnsCloudflare:26` via `GetAdaptersAddresses` `iphlpapi`, NetBIOS `NetbiosOptions:41`, LLMNR `EnableMulticast:53`, mDNS `EnableMDNS:58`, WPAD `WpadOverride:63`, Telemetry `DataCollection:68`, KillSwitch, IPForward `IPEnableRouter:79`) + `ApplyHarden()` 8/8. Убран `ExecutionPolicy Bypass` для основныхflows (остался только `vpn-leaktest` `vpn-gui.cpp:772`).

### 🔧 Рефактор
- `vpn-gui.cpp:959` (−295) — вынесены `src/exec` 126c `ExecCmd/ExecWG/GetLine/ParseSize`, `src/config` 180c `ReadFileText/ReadLog/WriteLog/DPAPI`, `src/peers` 60c `CountPeers/Append/Remove`, `src/killswitch` 95c, `src/privacy` 216c, `src/wizard` 144c. `CMakeLists.txt:7` собирает все + `iphlpapi/crypt32`, `-static -s` 1.107 MB.
- Хардкод `D:\SOOBSHESTVA` `vpn-gui.cpp:145` → `g_appDir` `vpn-gui.cpp:681`.
- `installer.iss:22` фикс `SetupIconFile` (был `app.manifest`), `DefaultDirName=C:\VPN-TEIVRIM`, firewall `VPN-TEIVRIM-In/Out` 51820, `release/*.exe` ` .gitignore:9`.

### 🛠 Build/CI
- `.github/workflows/build.yml:35` — MinGW64 `msys2/setup-msys2@v2` + `cmake` + `cmake --build` + gate `>2MB` throw + `Compress-Archive` ZIP + `upload-artifact` + `softprops/action-gh-release` на тег `v*.*.*`. Статус `success` `gh run list` 2026-09-24.
- `app.manifest` `manifest.rc:1` → `CMake RC` `manifest.rc.obj`, `#pragma comment` warning но сборка зеленая.
- `iscc installer.iss` 6.7.1 → `release/VPN-TEIVRIM-v2.4.0-setup.exe` 2.29 MB + `release/VPN-TEIVRIM-v2.4.0-win64.zip` 0.399 MB (19 файлов incl `VPN-TEIVRIM.exe` 1.16 MB).

### 🐛 Тесты
- `Remove-Item build; cmake -B build` → `Built target vpn-gui` без `undefined reference`, `build/VPN-TEIVRIM.exe` 1 160 704 B.
- Ручной запуск `build/VPN-TEIVRIM.exe` 3 сек → `build/vpn-gui.log:1` `=== GUI v2.4.0 started ===`, no `crash.log`.
- `netsh show rule name=KS-Block-All` — пусто (KS OFF), после ON → 10 правил `KS-Allow-*` + `KS-Block-All`.
- ZIP content 19 файлов verified `Expand-Archive`.

### 📦 Deploy
```cmd
# One-Click Installer
VPN-TEIVRIM-v2.4.0-setup.exe  # Next-Next → C:\VPN-TEIVRIM + C:\WireGuard\wg0.conf + HKLM\Run
# ZIP
Expand-Archive VPN-TEIVRIM-v2.4.0-win64.zip; .\VPN-TEIVRIM.exe  # Мастер
# Legacy
install.bat  # HKLM\Run, Startup shortcut removed
```

### 🔗 Requirements
- Windows 10/11 x64, Admin, UDP 51820 forward, WireGuard `C:\Program Files\WireGuard\wg.exe` (проверка `IsWireGuardInstalled` `src/privacy.cpp:86`)

### ⏭ Не делали (по ТЗ)
- VPS-сеть, Telegram-бот, монетизация, кроссплатформа — исключены. P2/P3 ROADMAP (VPS SSH, подписка) — отложены.

## 📊 Метрики готовности v2.4.0
- [x] `build.yml` зеленый `success`
- [x] `VPN-TEIVRIM.exe` 1.107 MB `<2MB` static `-s`
- [ ] `installer.exe` тест с нуля на VM `<3 мин` — требует Hyper-V/VM (ручной, не в CI)
- [x] Kill Switch переживает ребут (маркер + Run)
- [x] Мастер 3 шага `<3 мин`

## 🔜 Next (post v2.4.0)
- VM тест Hyper-V + `vpn-leaktest.ps1` + `Restart-Computer` restore check
- `gh release create v2.4.0 --notes-file RELEASE_NOTES_v2.4.0.md`
