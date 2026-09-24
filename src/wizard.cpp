#ifndef UNICODE
#define UNICODE
#endif
#include "wizard.h"
#include "privacy.h"
#include "exec.h"
#include "config.h"
#include <string>

extern HWND g_hWnd;
extern wchar_t g_appDir[MAX_PATH];

static HWND g_wiz = NULL;
static int g_step = 1;
static HWND g_lblTitle, g_lblBody, g_btnNext, g_btnBack, g_btnClose;

static void UpdateWizardStep() {
    if (!g_wiz) return;
    std::wstring title, body, nextTxt;
    bool showBack = true;
    if (g_step == 1) {
        title = L"Шаг 1/3 — Установка сервера";
        bool wg = IsWireGuardInstalled();
        bool cfg = IsServerConfigured();
        if (!wg) body = L"❌ WireGuard не установлен.\n\nУстановите WireGuard с https://www.wireguard.com/install/\nИли запустите install.bat от имени администратора.\n\nПосле установки нажмите Далее.";
        else if (!cfg) body = L"✓ WireGuard найден.\n\nНажмите Далее чтобы сгенерировать ключи и создать\nC:\\WireGuard\\wg0.conf + client0.conf\n\nЗаймет ~5 сек.";
        else body = L"✓ Сервер уже настроен (wg0.conf найден).\n\nНажмите Далее для проверки порта.";
        nextTxt = cfg ? L"Далее →" : L"Создать →";
        showBack = false;
    } else if (g_step == 2) {
        title = L"Шаг 2/3 — Проброс порта";
        std::wstring pip = ExecCmd(L"curl.exe -s --max-time 4 https://api.ipify.org", 5000);
        pip = TrimWS(pip);
        if (pip.empty()) pip = L"—";
        // LAN IP
        std::wstring ipRaw = ExecCmd(L"cmd.exe /c ipconfig", 2000);
        size_t p = ipRaw.find(L"192.168");
        if (p == std::wstring::npos) p = ipRaw.find(L"10.0");
        std::wstring lan = L"—";
        if (p != std::wstring::npos) {
            size_t e = ipRaw.find_first_of(L"\r\n", p);
            lan = ipRaw.substr(p, e != std::wstring::npos ? e - p : 20);
        }
        wchar_t buf[1024];
        wsprintfW(buf, L"LAN: %s\nWAN: %s\n\nОткройте на роутере проброс UDP 51820 → %s\n\nИнструкция: зайти в 192.168.0.1/1.1 → Port Forwarding\nПроверить: https://www.yougetsignal.com/tools/open-ports/ 51820", lan.c_str(), pip.c_str(), lan.c_str());
        body = buf;
        nextTxt = L"Далее →";
    } else {
        title = L"Шаг 3/3 — QR для телефона";
        bool cfg = IsServerConfigured();
        if (cfg) body = L"✓ Сервер ONLINE.\n\nНажмите «Показать QR» в главном окне или\n«Копировать» чтобы импортировать client0.conf\n\nQR содержит полный конфиг для WireGuard на телефоне.\n\nГотово — мастер завершен!";
        else body = L"Сначала завершите шаг 1.";
        nextTxt = L"Готово ✓";
    }
    SetWindowTextW(g_lblTitle, title.c_str());
    SetWindowTextW(g_lblBody, body.c_str());
    SetWindowTextW(g_btnNext, nextTxt.c_str());
    EnableWindow(g_btnBack, showBack);
    ShowWindow(g_btnBack, showBack ? SW_SHOW : SW_HIDE);
}

static LRESULT CALLBACK WizProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_COMMAND: {
        int id = LOWORD(w);
        if (id == 101) { // Next
            if (g_step == 1) {
                if (!IsWireGuardInstalled()) {
                    MessageBoxW(h, L"Установите WireGuard сначала", L"Мастер", MB_ICONWARNING);
                    break;
                }
                if (!IsServerConfigured()) {
                    SetWindowTextW(g_lblBody, L"⏳ Генерирую ключи...");
                    EnableWindow(g_btnNext, FALSE);
                    bool ok = SetupPrivacyServer();
                    EnableWindow(g_btnNext, TRUE);
                    if (!ok) MessageBoxW(h, L"Ошибка создания wg0.conf. Проверьте логи.", L"Мастер", MB_ICONERROR);
                }
                g_step = 2;
            } else if (g_step == 2) {
                g_step = 3;
            } else {
                DestroyWindow(h);
                g_wiz = NULL;
                return 0;
            }
            UpdateWizardStep();
        } else if (id == 102) { // Back
            if (g_step > 1) g_step--;
            UpdateWizardStep();
        } else if (id == 103) {
            DestroyWindow(h);
            g_wiz = NULL;
        }
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(h);
        g_wiz = NULL;
        return 0;
    case WM_DESTROY:
        g_wiz = NULL;
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

void ShowWizard(HWND parent) {
    if (g_wiz) { SetForegroundWindow(g_wiz); return; }
    g_step = IsServerConfigured() ? 2 : 1;
    if (!g_step) g_step = 1;
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WizProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"WizWin";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(18,18,24));
    static bool reg = false;
    if (!reg) { RegisterClassW(&wc); reg = true; }
    g_wiz = CreateWindowExW(WS_EX_DLGMODALFRAME, L"WizWin", L"Мастер VPN-TEIVRIM — 3 шага",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 0, 0, 500, 360, parent, 0, GetModuleHandle(NULL), 0);
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(g_wiz, NULL, (sw-500)/2, (sh-360)/2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    HFONT fHead = CreateFontW(18,0,0,0,FW_BOLD,0,0,0,0,0,0,0,0,L"Segoe UI");
    HFONT fBody = CreateFontW(14,0,0,0,FW_NORMAL,0,0,0,0,0,0,0,0,L"Segoe UI");
    HFONT fMono = CreateFontW(12,0,0,0,FW_NORMAL,0,0,0,FIXED_PITCH,0,0,0,0,L"Consolas");
    g_lblTitle = CreateWindowW(L"STATIC", L"", WS_CHILD|WS_VISIBLE, 16,16,468,26, g_wiz,0,GetModuleHandle(NULL),0);
    SendMessage(g_lblTitle, WM_SETFONT, (WPARAM)fHead,1);
    g_lblBody = CreateWindowW(L"STATIC", L"", WS_CHILD|WS_VISIBLE, 16,48,468,200, g_wiz,0,GetModuleHandle(NULL),0);
    SendMessage(g_lblBody, WM_SETFONT, (WPARAM)fMono,1);
    g_btnBack = CreateWindowW(L"BUTTON", L"← Назад", WS_CHILD|WS_VISIBLE, 16,280,100,30, g_wiz,(HMENU)102,GetModuleHandle(NULL),0);
    g_btnNext = CreateWindowW(L"BUTTON", L"Далее →", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON, 200,280,140,30, g_wiz,(HMENU)101,GetModuleHandle(NULL),0);
    g_btnClose = CreateWindowW(L"BUTTON", L"Закрыть", WS_CHILD|WS_VISIBLE, 384,280,100,30, g_wiz,(HMENU)103,GetModuleHandle(NULL),0);
    SendMessage(g_btnNext, WM_SETFONT, (WPARAM)fBody,1);
    SendMessage(g_btnBack, WM_SETFONT, (WPARAM)fBody,1);
    SendMessage(g_btnClose, WM_SETFONT, (WPARAM)fBody,1);
    UpdateWizardStep();
    ShowWindow(g_wiz, SW_SHOW);
    SetForegroundWindow(g_wiz);
}

bool IsWizardNeeded() {
    return !IsServerConfigured();
}
