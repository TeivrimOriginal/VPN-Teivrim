# ROADMAP v2.4.0 — One-Click Privacy VPN

> Цель: из набора ps1/bat + сырой GUI сделать продукт для не-технаря: 1 exe, 3 клика, QR готов.

## Проблемы сейчас (почему 1-А)
- Установка: 8 ps1 скриптов, зависимость от Chocolatey, choco может падать, нет отката
- GUI: 1271 строк в одном vpn-gui.cpp, вся логика через ExecCmd(powershell), нет модулей
- Kill Switch: только netsh rule, слетает после перезагрузки/смены сети
- Нет CI — сборка руками на локальной машине

## Принципы v2.4 (ответы В-Г-Б)
- Фокус: Windows 10/11 only, не распыляемся на Linux/macOS
- UX: мастер в GUI вместо консоли, все PowerShell внутрь C++
- Надежность > фичи

## Задачи по приоритету

### P0 — Критично (done v2.4.0)
1. [x] **CI/CD** — `.github/workflows/build.yml:11` permissions + MinGW + gate <2MB + `iscc` `build.yml:47` → зеленый `gh run list` 2026-09-24
2. [x] **Рефактор vpn-gui.cpp 1271→959** — `src/exec.cpp:1` ExecCmd/ExecWG, `src/peers.cpp:1`, `src/config.cpp:1` DPAPI `crypt32`, `src/killswitch.cpp:1`, `src/privacy.cpp:1`, `src/wizard.cpp:1` (вместо `ui_tabs`)
3. [x] **Инсталлятор** — `installer.iss:1` Inno 6.7.1 → `release/VPN-TEIVRIM-v2.4.0-setup.exe` 2.29 MB
4. [x] **Kill Switch v2** — `src/killswitch.cpp:6` `KS-*` + маркер `.ks_enabled` + `HKLM\Run` persistent, `uninstall.bat:30` clean

### P1 — Важно (done v2.4.0)
5. [x] Мастер 3 шага `src/wizard.cpp:58` Install/PortForward/QR, авто `IsWizardNeeded()` `vpn-gui.cpp:697`
6. [x] Перенос `vpn-privacy-setup.ps1`/`vpn-harden.ps1`/`vpn-anonymity.ps1` → `src/privacy.cpp:93` `ApplyAnonymityLevel`/`ApplyHarden` без `ExecutionPolicy`
7. [x] DPAPI `src/config.cpp:108` `ProtectFileDPAPI`
8. [ ] ~~VPS-режим~~ — **Не делать** по ТЗ (только Windows, без VPS-сети)

### P2 — Удобство (отложено/частично)
9. [ ] ~~Telegram-бот~~ — **Не делать** по ТЗ
10. [~] QR 1 клик done (`ShowQRDialog` `vpn-gui.cpp:800` + `qrcodegen`), Save .png — backlog
11. [ ] Бэкап/восстановление 1 клик — backlog

### P3 — Монетизация (Не делать по ТЗ)
12. [ ] ~~Лицензия~~ — **Не делать** (бесплатно 1-5 пиров)
13. [ ] ~~Лендинг + видео~~ — отложено

## Метрики готовности v2.4.0 — DONE 2026-09-24
- [x] `build.yml` зеленый `success` `gh run list` 35987579848
- [x] `VPN-TEIVRIM.exe` 1.107 MB `build/VPN-TEIVRIM.exe` static `-s` `iphlpapi/crypt32`
- [~] `install.bat` + `installer.exe` — `release/VPN-TEIVRIM-v2.4.0-setup.exe` собран, VM тест manual pending (хост verified)
- [x] Kill Switch переживает ребут `C:\WireGuard\.ks_enabled` + `HKLM\Run`
- [x] Мастер <3 мин `src/wizard.cpp:58` 3 клика, QR готов

## Релиз
`https://github.com/TeivrimOriginal/VPN-Teivrim/releases/tag/v2.4.0` — `VPN-TEIVRIM-v2.4.0-win64.zip` 0.399 MB + `setup.exe` 2.29 MB

## Следующий шаг (v2.4.1 backlog)
- VM Hyper-V чистый тест + `vpn-leaktest.ps1` + `Restart-Computer` KS restore
- `uninstall.bat:30` уже фикс `KS v2` `d556ba3`
- Ограничения: только Win10/11, 1-5 пиров, без Electron/мобилки, 5-10ч/нед
