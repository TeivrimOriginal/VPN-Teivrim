# VPN-TEIVRIM v2.4.2 — CI fix

## Fix
- `.github/workflows/build.yml:55` — fix ZIP version parsing `VERSION ([0-9.]+)` → `project\(VPN-TEIVRIM VERSION` (был `3.20` из `cmake_minimum_required`). Теперь `VPN-TEIVRIM-v2.4.2-win64.zip` корректно.
- Бамп `2.4.1 → 2.4.2` `CMakeLists.txt:2` `installer.iss:4` `vpn-gui.cpp:668,693,940`.

## Build
- `VPN-TEIVRIM.exe` 1.107 MB `<2MB`
- `release/VPN-TEIVRIM-v2.4.2-setup.exe` 2.29 MB + `release/VPN-TEIVRIM-v2.4.2-win64.zip` 0.40 MB — оба в Release via `build.yml:80` `release/VPN-TEIVRIM-*.exe` + `VPN-TEIVRIM-*.zip`.

## Предыдущий релиз
- `v2.4.1` содержал артефакт `v3.20` из-за бага парсинга — оставлен для истории, `v2.4.2` — исправленный Latest.
