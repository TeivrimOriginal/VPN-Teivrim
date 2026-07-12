#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <cstdio>

#pragma comment(lib, "comctl32.lib")

#define ID_BTN_REFRESH 101
#define ID_BTN_START   102
#define ID_BTN_STOP    103
#define ID_BTN_ADD     104
#define ID_BTN_LOG     105
#define ID_LIST_PEER   106
#define ID_LIST_LOG    107
#define ID_LBL_STATUS  108
#define ID_LBL_PORT    109
#define ID_LBL_KEY     110
#define ID_LBL_IP      111
#define ID_TIMER       112

const wchar_t CLASS_NAME[] = L"WGMon2";
HINSTANCE g_hInst;
HWND g_hWnd;
HWND g_lblStatus, g_lblPort, g_lblKey, g_lblIp;
HWND g_listPeer, g_listLog;
HWND g_btnRefresh, g_btnStart, g_btnStop, g_btnAdd, g_btnLog;
HFONT g_hFont, g_hFontBold, g_hFontMono;
HBRUSH g_hBrushBg;
bool g_refreshing = false;

void LogMsg(const char* msg) {
    FILE* f = NULL;
    fopen_s(&f, "C:\\WireGuard\\vpn-gui.log", "a");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, msg);
        fclose(f);
    }
}

std::wstring ExecWg(const wchar_t* args) {
    wchar_t cmd[512];
    wsprintfW(cmd, L"\"C:\\Program Files\\WireGuard\\wg.exe\" %s", args);

    HANDLE hR, hW;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    if (!CreatePipe(&hR, &hW, &sa, 0)) return L"";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hW;
    si.hStdError = hW;

    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessW(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(hW);

    if (!ok) {
        CloseHandle(hR);
        LogMsg("CreateProcess failed for wg.exe");
        return L"";
    }

    std::wstring result;
    char buf[4096];
    DWORD n = 0;
    while (ReadFile(hR, buf, sizeof(buf) - 1, &n, NULL) && n > 0) {
        buf[n] = 0;
        int wl = MultiByteToWideChar(CP_UTF8, 0, buf, -1, 0, 0);
        if (wl > 0) {
            wchar_t* wb = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, wl * sizeof(wchar_t));
            if (wb) {
                MultiByteToWideChar(CP_UTF8, 0, buf, -1, wb, wl);
                result += wb;
                HeapFree(GetProcessHeap(), 0, wb);
            }
        }
        n = 0;
    }
    CloseHandle(hR);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return result;
}

std::wstring ExecCmd(const wchar_t* cmdline) {
    HANDLE hR, hW;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    if (!CreatePipe(&hR, &hW, &sa, 0)) return L"";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hW;
    si.hStdError = hW;

    wchar_t buf[512];
    wcscpy_s(buf, cmdline);
    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessW(NULL, buf, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(hW);

    if (!ok) { CloseHandle(hR); return L""; }

    std::wstring result;
    char tmp[4096];
    DWORD n = 0;
    while (ReadFile(hR, tmp, sizeof(tmp) - 1, &n, NULL) && n > 0) {
        tmp[n] = 0;
        int wl = MultiByteToWideChar(CP_UTF8, 0, tmp, -1, 0, 0);
        if (wl > 0) {
            wchar_t* wb = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, wl * sizeof(wchar_t));
            if (wb) {
                MultiByteToWideChar(CP_UTF8, 0, tmp, -1, wb, wl);
                result += wb;
                HeapFree(GetProcessHeap(), 0, wb);
            }
        }
        n = 0;
    }
    CloseHandle(hR);
    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return result;
}

std::wstring GetLine(const std::wstring& s, const wchar_t* key) {
    size_t p = s.find(key);
    if (p == std::wstring::npos) return L"";
    p += wcslen(key);
    size_t e = s.find(L'\n', p);
    if (e == std::wstring::npos) e = s.size();
    std::wstring r = s.substr(p, e - p);
    while (!r.empty() && (r.back() == L'\r' || r.back() == L' ')) r.pop_back();
    return r;
}

std::wstring ReadLogFile(int maxLines) {
    FILE* f = NULL;
    _wfopen_s(&f, L"C:\\WireGuard\\vpn-monitor.log", L"rb");
    if (!f) {
        _wfopen_s(&f, L"C:\\WireGuard\\vpn-gui.log", L"rb");
        if (!f) return L"  (no log file found)";
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz <= 0) { fclose(f); return L"  (empty)"; }
    int rs = sz > 16384 ? 16384 : sz;
    fseek(f, -rs, SEEK_END);
    char* d = (char*)HeapAlloc(GetProcessHeap(), 0, rs + 1);
    if (!d) { fclose(f); return L"  (alloc error)"; }
    fread(d, 1, rs, f);
    d[rs] = 0;
    fclose(f);
    int wl = MultiByteToWideChar(CP_UTF8, 0, d, -1, 0, 0);
    wchar_t* wd = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, wl * sizeof(wchar_t));
    if (!wd) { HeapFree(GetProcessHeap(), 0, d); return L"  (alloc error)"; }
    MultiByteToWideChar(CP_UTF8, 0, d, -1, wd, wl);
    HeapFree(GetProcessHeap(), 0, d);
    std::wstring content(wd);
    HeapFree(GetProcessHeap(), 0, wd);

    std::vector<std::wstring> lines;
    size_t p = 0;
    while (p < content.size()) {
        size_t e = content.find(L'\n', p);
        if (e == std::wstring::npos) e = content.size();
        std::wstring l = content.substr(p, e - p);
        while (!l.empty() && l.back() == L'\r') l.pop_back();
        if (!l.empty()) lines.push_back(l);
        p = e + 1;
    }
    std::wstring out;
    int start = ((int)lines.size() > maxLines) ? ((int)lines.size() - maxLines) : 0;
    for (int i = start; i < (int)lines.size(); i++)
        out += lines[i] + L"\n";
    return out;
}

void WriteAppLog(const wchar_t* msg) {
    FILE* f = NULL;
    _wfopen_s(&f, L"C:\\WireGuard\\vpn-gui.log", L"a, ccs=UTF-8");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        char mb[256] = {};
        WideCharToMultiByte(CP_UTF8, 0, msg, -1, mb, sizeof(mb), NULL, NULL);
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, mb);
        fclose(f);
    }
}

void DoRefresh() {
    if (g_refreshing) return;
    g_refreshing = true;
    LogMsg("Refresh start");

    SendMessage(g_hWnd, WM_SETREDRAW, FALSE, 0);

    std::wstring wg = ExecWg(L"show wg0");
    bool on = (wg.find(L"listening port") != std::wstring::npos);
    LogMsg(on ? "Server ONLINE" : "Server OFFLINE");

    SetWindowTextW(g_lblStatus, on ? L"  ONLINE  " : L"  OFFLINE  ");

    if (on) {
        std::wstring port = GetLine(wg, L"listening port: ");
        std::wstring pk = GetLine(wg, L"public key: ");
        if (pk.size() > 36) pk = pk.substr(0, 36) + L"...";
        SetWindowTextW(g_lblPort, (L"  Port: " + port).c_str());
        SetWindowTextW(g_lblKey, (L"  Key: " + pk).c_str());
    } else {
        SetWindowTextW(g_lblPort, L"  Port: ---");
        SetWindowTextW(g_lblKey, L"  Key: ---");
    }

    std::wstring ipRaw = ExecCmd(L"cmd.exe /c ipconfig");
    size_t ipPos = ipRaw.find(L"192.168");
    if (ipPos == std::wstring::npos) ipPos = ipRaw.find(L"10.0");
    std::wstring lip = L"---";
    if (ipPos != std::wstring::npos) {
        size_t e = ipRaw.find_first_of(L"\r\n", ipPos);
        lip = ipRaw.substr(ipPos, e != std::wstring::npos ? e - ipPos : 50);
        size_t trim = lip.find_last_not_of(L" \t");
        if (trim != std::wstring::npos) lip = lip.substr(0, trim + 1);
    }
    SetWindowTextW(g_lblIp, (L"  LAN: " + lip).c_str());

    ListView_DeleteAllItems(g_listPeer);
    int idx = 0;
    size_t pp = 0;
    while ((pp = wg.find(L"peer:", pp)) != std::wstring::npos) {
        size_t pe = wg.find(L"peer:", pp + 1);
        if (pe == std::wstring::npos) pe = wg.size();
        std::wstring blk = wg.substr(pp, pe - pp);
        std::wstring key = GetLine(blk, L"public key: ");
        std::wstring ep = GetLine(blk, L"endpoint: ");
        std::wstring hs = GetLine(blk, L"latest handshake: ");
        std::wstring tr = GetLine(blk, L"transfer: ");
        if (!key.empty()) {
            if (key.size() > 20) key = key.substr(0, 20) + L"...";
            LVITEMW li = {};
            li.mask = LVIF_TEXT;
            li.iItem = idx;
            li.pszText = (LPWSTR)key.c_str();
            ListView_InsertItem(g_listPeer, &li);
            if (!ep.empty()) ListView_SetItemText(g_listPeer, idx, 1, (LPWSTR)ep.c_str());
            if (!hs.empty()) ListView_SetItemText(g_listPeer, idx, 2, (LPWSTR)hs.c_str());
            if (!tr.empty()) ListView_SetItemText(g_listPeer, idx, 3, (LPWSTR)tr.c_str());
            idx++;
        }
        pp = pe;
    }

    std::wstring logContent = ReadLogFile(30);
    SendMessage(g_listLog, LB_RESETCONTENT, 0, 0);
    size_t lp = 0;
    while (lp < logContent.size()) {
        size_t e = logContent.find(L'\n', lp);
        if (e == std::wstring::npos) e = logContent.size();
        std::wstring l = logContent.substr(lp, e - lp);
        while (!l.empty() && l.back() == L'\r') l.pop_back();
        if (!l.empty())
            SendMessageW(g_listLog, LB_ADDSTRING, 0, (LPARAM)l.c_str());
        lp = e + 1;
    }
    int cnt = (int)SendMessage(g_listLog, LB_GETCOUNT, 0, 0);
    if (cnt > 0) SendMessage(g_listLog, LB_SETTOPINDEX, cnt - 1, 0);

    SendMessage(g_hWnd, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hWnd, NULL, FALSE);
    UpdateWindow(g_hWnd);
    g_refreshing = false;
    LogMsg("Refresh done");
}

void DoStart() {
    WriteAppLog(L"Starting server...");
    SetWindowTextW(g_lblStatus, L"  STARTING...  ");
    InvalidateRect(g_lblStatus, NULL, TRUE);
    UpdateWindow(g_lblStatus);
    g_refreshing = true;
    ShellExecuteW(NULL, L"open", L"C:\\Program Files\\WireGuard\\wireguard.exe",
        L"/installtunnelservice C:\\WireGuard\\wg0.conf", NULL, SW_HIDE);
    Sleep(2000);
    g_refreshing = false;
    DoRefresh();
    WriteAppLog(L"Start command sent");
}

void DoStop() {
    WriteAppLog(L"Stopping server...");
    SetWindowTextW(g_lblStatus, L"  STOPPING...  ");
    InvalidateRect(g_lblStatus, NULL, TRUE);
    UpdateWindow(g_lblStatus);
    g_refreshing = true;
    ShellExecuteW(NULL, L"open", L"C:\\Program Files\\WireGuard\\wireguard.exe",
        L"/uninstalltunnelservice wg0", NULL, SW_HIDE);
    Sleep(2000);
    g_refreshing = false;
    DoRefresh();
    WriteAppLog(L"Stop command sent");
}

void DoAddClient() {
    WriteAppLog(L"Opening client creator...");
    ShellExecuteW(NULL, L"open", L"powershell.exe",
        L"-ExecutionPolicy Bypass -NoProfile -Command \"& 'D:\\SOOBSHESTVA\\specify\\eblya\\vpn-add-client.ps1'\"",
        NULL, SW_SHOW);
}

void DoOpenLog() {
    ShellExecuteW(NULL, L"open", L"notepad.exe",
        L"C:\\WireGuard\\vpn-gui.log", NULL, SW_SHOW);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        LogMsg("WM_CREATE");
        g_hBrushBg = CreateSolidBrush(RGB(22, 22, 28));
        g_hFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI");
        g_hFontBold = CreateFontW(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI");
        g_hFontMono = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, FIXED_PITCH, 0, 0, 0, 0, L"Consolas");

        HWND t = CreateWindowW(L"STATIC", L"WireGuard VPN Monitor",
            WS_CHILD | WS_VISIBLE, 20, 12, 500, 28, hWnd, 0, g_hInst, 0);
        SendMessage(t, WM_SETFONT, (WPARAM)g_hFontBold, 1);

        g_lblStatus = CreateWindowW(L"STATIC", L"  WAIT  ",
            WS_CHILD | WS_VISIBLE | SS_CENTER, 620, 10, 150, 32, hWnd, (HMENU)ID_LBL_STATUS, g_hInst, 0);
        SendMessage(g_lblStatus, WM_SETFONT, (WPARAM)g_hFontBold, 1);

        CreateWindowW(L"STATIC", 0, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
            20, 50, 750, 2, hWnd, 0, g_hInst, 0);

        g_lblPort = CreateWindowW(L"STATIC", L"  Port: ---",
            WS_CHILD | WS_VISIBLE, 20, 62, 360, 22, hWnd, (HMENU)ID_LBL_PORT, g_hInst, 0);
        g_lblKey = CreateWindowW(L"STATIC", L"  Key: ---",
            WS_CHILD | WS_VISIBLE, 20, 86, 500, 22, hWnd, (HMENU)ID_LBL_KEY, g_hInst, 0);
        g_lblIp = CreateWindowW(L"STATIC", L"  LAN: ---",
            WS_CHILD | WS_VISIBLE, 520, 62, 250, 22, hWnd, (HMENU)ID_LBL_IP, g_hInst, 0);
        SendMessage(g_lblPort, WM_SETFONT, (WPARAM)g_hFontMono, 1);
        SendMessage(g_lblKey, WM_SETFONT, (WPARAM)g_hFontMono, 1);
        SendMessage(g_lblIp, WM_SETFONT, (WPARAM)g_hFontMono, 1);

        HWND pl = CreateWindowW(L"STATIC", L"  Peers",
            WS_CHILD | WS_VISIBLE, 20, 118, 200, 22, hWnd, 0, g_hInst, 0);
        SendMessage(pl, WM_SETFONT, (WPARAM)g_hFontBold, 1);

        g_listPeer = CreateWindowExW(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
            WC_LISTVIEWW, 0,
            WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_NOCOLUMNHEADER,
            20, 142, 750, 120, hWnd, (HMENU)ID_LIST_PEER, g_hInst, 0);
        SendMessage(g_listPeer, WM_SETFONT, (WPARAM)g_hFontMono, 1);
        ListView_SetBkColor(g_listPeer, RGB(26, 26, 32));
        ListView_SetTextColor(g_listPeer, RGB(200, 200, 200));
        LVCOLUMNW c = {};
        c.mask = LVCF_TEXT | LVCF_WIDTH;
        c.pszText = (LPWSTR)L"Key"; c.cx = 230; ListView_InsertColumn(g_listPeer, 0, &c);
        c.pszText = (LPWSTR)L"Endpoint"; c.cx = 180; ListView_InsertColumn(g_listPeer, 1, &c);
        c.pszText = (LPWSTR)L"Handshake"; c.cx = 170; ListView_InsertColumn(g_listPeer, 2, &c);
        c.pszText = (LPWSTR)L"Transfer"; c.cx = 160; ListView_InsertColumn(g_listPeer, 3, &c);

        HWND ll = CreateWindowW(L"STATIC", L"  Log",
            WS_CHILD | WS_VISIBLE, 20, 272, 200, 22, hWnd, 0, g_hInst, 0);
        SendMessage(ll, WM_SETFONT, (WPARAM)g_hFontBold, 1);

        g_listLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", 0,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOSEL,
            20, 296, 750, 160, hWnd, (HMENU)ID_LIST_LOG, g_hInst, 0);
        SendMessage(g_listLog, WM_SETFONT, (WPARAM)g_hFontMono, 1);

        CreateWindowW(L"STATIC", 0, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
            20, 466, 750, 2, hWnd, 0, g_hInst, 0);

        g_btnRefresh = CreateWindowW(L"BUTTON", L"Refresh",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 20, 480, 100, 34, hWnd, (HMENU)ID_BTN_REFRESH, g_hInst, 0);
        g_btnStart = CreateWindowW(L"BUTTON", L"Start",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 130, 480, 100, 34, hWnd, (HMENU)ID_BTN_START, g_hInst, 0);
        g_btnStop = CreateWindowW(L"BUTTON", L"Stop",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 240, 480, 100, 34, hWnd, (HMENU)ID_BTN_STOP, g_hInst, 0);
        g_btnAdd = CreateWindowW(L"BUTTON", L"Add Client",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 350, 480, 110, 34, hWnd, (HMENU)ID_BTN_ADD, g_hInst, 0);
        g_btnLog = CreateWindowW(L"BUTTON", L"Open Log",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 470, 480, 110, 34, hWnd, (HMENU)ID_BTN_LOG, g_hInst, 0);
        SendMessage(g_btnRefresh, WM_SETFONT, (WPARAM)g_hFont, 1);
        SendMessage(g_btnStart, WM_SETFONT, (WPARAM)g_hFont, 1);
        SendMessage(g_btnStop, WM_SETFONT, (WPARAM)g_hFont, 1);
        SendMessage(g_btnAdd, WM_SETFONT, (WPARAM)g_hFont, 1);
        SendMessage(g_btnLog, WM_SETFONT, (WPARAM)g_hFont, 1);

        HWND foot = CreateWindowW(L"STATIC",
            L"  UDP 51820 | 10.0.0.0/24 | Auto-refresh 3s",
            WS_CHILD | WS_VISIBLE, 20, 524, 500, 20, hWnd, 0, g_hInst, 0);
        SendMessage(foot, WM_SETFONT, (WPARAM)g_hFont, 1);

        SetTimer(hWnd, ID_TIMER, 5000, 0);
        DoRefresh();
        return 0;
    }

    case WM_TIMER:
        if (wParam == ID_TIMER) DoRefresh();
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BTN_REFRESH: DoRefresh(); break;
        case ID_BTN_START:   DoStart(); break;
        case ID_BTN_STOP:    DoStop(); break;
        case ID_BTN_ADD:     DoAddClient(); break;
        case ID_BTN_LOG:     DoOpenLog(); break;
        }
        return 0;

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND c = (HWND)lParam;
        SetBkMode(hdc, TRANSPARENT);
        if (c == g_lblStatus) {
            wchar_t buf[64] = {};
            GetWindowTextW(c, buf, 63);
            SetTextColor(hdc, wcsstr(buf, L"ONLINE") ? RGB(0, 230, 0) :
                wcsstr(buf, L"OFFLINE") ? RGB(255, 50, 50) : RGB(200, 200, 0));
        } else {
            SetTextColor(hdc, RGB(160, 160, 160));
        }
        return (LRESULT)g_hBrushBg;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        HWND c = (HWND)lParam;
        if (c == g_listLog) {
            SetTextColor(hdc, RGB(0, 210, 90));
            SetBkColor(hdc, RGB(14, 14, 18));
            static HBRUSH br = CreateSolidBrush(RGB(14, 14, 18));
            return (LRESULT)br;
        }
        SetTextColor(hdc, RGB(190, 190, 190));
        SetBkColor(hdc, RGB(26, 26, 32));
        static HBRUSH br2 = CreateSolidBrush(RGB(26, 26, 32));
        return (LRESULT)br2;
    }

    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(220, 220, 220));
        static HBRUSH br = CreateSolidBrush(RGB(48, 48, 54));
        return (LRESULT)br;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, g_hBrushBg);
        return 1;
    }

    case WM_DESTROY:
        LogMsg("WM_DESTROY - exiting");
        KillTimer(hWnd, ID_TIMER);
        DeleteObject(g_hBrushBg);
        DeleteObject(g_hFont);
        DeleteObject(g_hFontBold);
        DeleteObject(g_hFontMono);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nShow) {
    g_hInst = hInst;
    LogMsg("=== Application start ===");

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(22, 22, 28));
    RegisterClassW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    g_hWnd = CreateWindowExW(0, CLASS_NAME, L"WireGuard VPN Monitor",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (sw - 800) / 2, (sh - 570) / 2, 800, 570,
        0, 0, hInst, 0);

    ShowWindow(g_hWnd, nShow);
    UpdateWindow(g_hWnd);

    MSG m;
    while (GetMessageW(&m, 0, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    LogMsg("=== Application exit ===");
    return 0;
}
