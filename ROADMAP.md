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

### P0 — Критично (делаем первыми)
1. **CI/CD** — `.github/workflows/build.yml` уже добавлен, проверить сборку
2. **Рефактор vpn-gui.cpp** — разбить на модули:
   - `src/exec.cpp` — ExecCmd/ExecWG
   - `src/peers.cpp` — Peer parsing, Append/Remove/Toggle
   - `src/config.cpp` — Read/Write wg0.conf, шифрование DPAPI
   - `src/ui_tabs.cpp` — отрисовка табов
3. **Инсталлятор** — Inno Setup скрипт `installer.iss` (один exe, автопроверка админа)
4. **Kill Switch v2** — Windows Filtering Platform / persistent route + служба

### P1 — Важно (недели 3-4)
5. Мастер настройки (3 шага в GUI)
6. Перенос vpn-privacy-setup.ps1 и vpn-harden.ps1 в C++ (убрать зависимость от ExecutionPolicy)
7. DPAPI шифрование ключей
8. VPS-режим: поле "Endpoint IP" + кнопка "Deploy to VPS (SSH)"

### P2 — Удобство (недели 5-6)
9. Telegram-бот алерты (опционально, токен в настройках)
10. Улучшение QR: Save .png, Copy, Send to phone
11. Бэкап/восстановление в 1 клик

### P3 — Монетизация (недели 7-8)
12. Лицензия: файл `license.key`, проверка RSA, лимит 3 пира бесплатно / 20 с лицензией
13. Лендинг + видео

## Метрики готовности v2.4.0
- [ ] `build.yml` зеленый
- [ ] `VPN-TEIVRIM.exe` собирается статически <2MB, без зависимостей
- [ ] `install.bat` + `installer.exe` оба работают с нуля на чистой VM
- [ ] Kill Switch переживает ребут
- [ ] Мастер проходит за <3 минут у не-технаря

## Следующий шаг
Запустить: `cmake -B build -G "MinGW Makefiles" && cmake --build build` и проверить CI.
