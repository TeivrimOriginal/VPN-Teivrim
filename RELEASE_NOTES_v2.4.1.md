# VPN-TEIVRIM v2.4.1 — Hotfix

## Fix
- `uninstall.bat:30` — clean всех KS v2 правил: `KS-Allow-*` `KS-Block-All` + `VPN-KillSwitch-*` + `L3-KS-*`, удаление маркера `C:\WireGuard\.ks_enabled` и `HKLM\Run\VPN-TEIVRIM-KS` (`d556ba3`).
- `ROADMAP.md:44` — отмечен done, исключены VPS/Telegram/монетизация по ТЗ.
- `CMakeLists.txt:2` `installer.iss:4` `vpn-gui.cpp:668,693,940` бамп `2.4.0 → 2.4.1`.

## Build
- `VPN-TEIVRIM.exe` 1.107 MB `<2MB` static, `build.yml:11` permissions + Inno Setup 6.7.1 → `release/VPN-TEIVRIM-v2.4.1-setup.exe` 2.29 MB.

## Нет изменений в core (wizard/privacy/killswitch)

Основа `v2.4.0` `RELEASE_NOTES_v2.4.0.md` — One-Click 3 шага, KS v2 persistent, DPAPI, CI зеленый.
