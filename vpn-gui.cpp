#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")

// Controls
#define ID_BTN_REFRESH   101
#define ID_BTN_START     102
#define ID_BTN_STOP      103
#define ID_BTN_ADD       104
#define ID_BTN_LOG       105
#define ID_LIST_PEER     106
#define ID_LIST_LOG      107
#define ID_LBL_STATUS    108
#define ID_LBL_PORT      109
#define ID_LBL_KEY       110
#define ID_LBL_IP        111
#define ID_TIMER         112
#define ID_BTN_LVL1      113
#define ID_BTN_LVL2      114
#define ID_BTN_LVL3      115
#define ID_BTN_KS_ON     116
#define ID_BTN_KS_OFF    117
#define ID_BTN_LEAK      118
#define ID_BTN_HARDEN    119
#define ID_LBL_LVL       120

const wchar_t CLASS_NAME[] = L"WGMon3";
HINSTANCE g_hInst;
HWND g_hWnd;
HWND g_lblStatus, g_lblPort, g_lblKey, g_lblIp, g_lblLevel;
HWND g_listPeer, g_listLog;
HWND g_btnRefresh, g_btnStart, g_btnStop, g_btnAdd, g_btnLog;
HWND g_btnLvl1, g_btnLvl2, g_btnLvl3;
HWND g_btnKSOn, g_btnKSOff, g_btnLeak, g_btnHarden;
HFONT g_hFont, g_hFontBold, g_hFontMono, g_hFontSmall;
HBRUSH g_hBrushBg;
bool g_refreshing = false;
int g_currentLevel = 0;

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
    if (!ok) { CloseHandle(hR); return L""; }
    std::wstring result;
    char buf[4096];
    DWORD n = 0;
    while (ReadFile(hR, buf, sizeof(buf) - 1, &n, NULL) && n > 0) {
        buf[n] = 0;
        int wl = MultiByteToWideChar(CP_UTF8, 0, buf, -1, 0, 0);
        if (wl > 0) {
            wchar_t* wb = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, wl * sizeof(wchar_t));
            if (wb) { MultiByteToWideChar(CP_UTF8, 0, buf, -1, wb, wl); result += wb; HeapFree(GetProcessHeap(), 0, wb); }
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
            if (wb) { MultiByteToWideChar(CP_UTF8, 0, tmp, -1, wb, wl); result += wb; HeapFree(GetProcessHeap(), 0, wb); }
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
    _wfopen_s(&f, L"C:\\WireGuard\\vpn-gui.log", L"rb");
    if (!f) { _wfopen_s(&f, L"C:\\WireGuard\\vpn-monitor.log", L"rb"); }
    if (!f) return L"  (no log)";
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz <= 0) { fclose(f); return L"  (empty)"; }
    int rs = sz > 16384 ? 16384 : sz;
    fseek(f, -rs, SEEK_END);
    char* d = (char*)HeapAlloc(GetProcessHeap(), 0, rs + 1);
    if (!d) { fclose(f); return L""; }
    fread(d, 1, rs, f);
    d[rs] = 0;
    fclose(f);
    int wl = MultiByteToWideChar(CP_UTF8, 0, d, -1, 0, 0);
    wchar_t* wd = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, wl * sizeof(wchar_t));
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
    _wfopen_s(&f, L"C:\\WireGuard\\vpn-gui.log", L"a");
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

    SendMessage(g_hWnd, WM_SETREDRAW, FALSE, 0);

    std::wstring wg = ExecWg(L"show wg0");
    bool on = (wg.find(L"listening port") != std::wstring::npos);
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

    // Detect anonymity level
    bool hasIPv6 = (ipRaw.find(L"IPv6") != std::wstring::npos && ipRaw.find(L"disabled") == std::wstring::npos);
    std::wstring dnsCheck = ExecCmd(L"cmd.exe /c netsh advfirewall firewall show rule name=all dir=out | findstr /i \"L3-KS\"");
    bool hasKS = (dnsCheck.find(L"L3-KS") != std::wstring::npos);

    int level = 1;
    if (!hasIPv6) level = 2;
    if (hasKS) level = 3;
    g_currentLevel = level;

    wchar_t lvlBuf[128];
    wsprintfW(lvlBuf, L"  Anonymity: Level %d", level);
    SetWindowTextW(g_lblLevel, lvlBuf);

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

    std::wstring logContent = ReadLogFile(20);
    SendMessage(g_listLog, LB_RESETCONTENT, 0, 0);
    size_t lp = 0;
    while (lp < logContent.size()) {
        size_t e = logContent.find(L'\n', lp);
        if (e == std::wstring::npos) e = logContent.size();
        std::wstring l = logContent.substr(lp, e - lp);
        while (!l.empty() && l.back() == L'\r') l.pop_back();
        if (!l.empty()) SendMessageW(g_listLog, LB_ADDSTRING, 0, (LPARAM)l.c_str());
        lp = e + 1;
    }
    int cnt = (int)SendMessage(g_listLog, LB_GETCOUNT, 0, 0);
    if (cnt > 0) SendMessage(g_listLog, LB_SETTOPINDEX, cnt - 1, 0);

    SendMessage(g_hWnd, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hWnd, NULL, FALSE);
    UpdateWindow(g_hWnd);
    g_refreshing = false;
}

void RunPS(const wchar_t* script) {
    wchar_t cmd[512];
    wsprintfW(cmd, L"powershell.exe -ExecutionPolicy Bypass -NoProfile -File \"%s\"", script);
    ShellExecuteW(NULL, L"open", L"powershell.exe",
        cmd + 17, NULL, SW_SHOW);
}

HWND CreateBtn(HWND parent, const wchar_t* text, int x, int y, int w, int h, int id) {
    HWND btn = CreateWindowW(L"BUTTON", text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, g_hInst, 0);
    SendMessage(btn, WM_SETFONT, (WPARAM)g_hFont, 1);
    return btn;
}

void AddControls(HWND hWnd) {
    g_hBrushBg = CreateSolidBrush(RGB(22, 22, 28));
    g_hFont = CreateFontW(15, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI");
    g_hFontBold = CreateFontW(20, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI");
    g_hFontMono = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, FIXED_PITCH, 0, 0, 0, 0, L"Consolas");
    g_hFontSmall = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI");

    // Title
    HWND t = CreateWindowW(L"STATIC", L"WireGuard VPN Monitor",
        WS_CHILD | WS_VISIBLE, 20, 8, 400, 28, hWnd, 0, g_hInst, 0);
    SendMessage(t, WM_SETFONT, (WPARAM)g_hFontBold, 1);

    g_lblStatus = CreateWindowW(L"STATIC", L"  WAIT  ",
        WS_CHILD | WS_VISIBLE | SS_CENTER, 550, 6, 120, 30, hWnd, (HMENU)ID_LBL_STATUS, g_hInst, 0);
    SendMessage(g_lblStatus, WM_SETFONT, (WPARAM)g_hFontBold, 1);

    g_lblLevel = CreateWindowW(L"STATIC", L"  Anonymity: Level ?",
        WS_CHILD | WS_VISIBLE, 680, 10, 200, 22, hWnd, (HMENU)ID_LBL_LVL, g_hInst, 0);
    SendMessage(g_lblLevel, WM_SETFONT, (WPARAM)g_hFontSmall, 1);

    // Separator
    CreateWindowW(L"STATIC", 0, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        20, 42, 860, 2, hWnd, 0, g_hInst, 0);

    // Info
    g_lblPort = CreateWindowW(L"STATIC", L"  Port: ---",
        WS_CHILD | WS_VISIBLE, 20, 52, 340, 20, hWnd, (HMENU)ID_LBL_PORT, g_hInst, 0);
    g_lblKey = CreateWindowW(L"STATIC", L"  Key: ---",
        WS_CHILD | WS_VISIBLE, 20, 74, 500, 20, hWnd, (HMENU)ID_LBL_KEY, g_hInst, 0);
    g_lblIp = CreateWindowW(L"STATIC", L"  LAN: ---",
        WS_CHILD | WS_VISIBLE, 520, 52, 250, 20, hWnd, (HMENU)ID_LBL_IP, g_hInst, 0);
    SendMessage(g_lblPort, WM_SETFONT, (WPARAM)g_hFontMono, 1);
    SendMessage(g_lblKey, WM_SETFONT, (WPARAM)g_hFontMono, 1);
    SendMessage(g_lblIp, WM_SETFONT, (WPARAM)g_hFontMono, 1);

    // Peers
    HWND pl = CreateWindowW(L"STATIC", L"  Peers",
        WS_CHILD | WS_VISIBLE, 20, 100, 200, 20, hWnd, 0, g_hInst, 0);
    SendMessage(pl, WM_SETFONT, (WPARAM)g_hFontBold, 1);

    g_listPeer = CreateWindowExW(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
        WC_LISTVIEWW, 0,
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_NOCOLUMNHEADER,
        20, 122, 860, 100, hWnd, (HMENU)ID_LIST_PEER, g_hInst, 0);
    SendMessage(g_listPeer, WM_SETFONT, (WPARAM)g_hFontMono, 1);
    ListView_SetBkColor(g_listPeer, RGB(26, 26, 32));
    ListView_SetTextColor(g_listPeer, RGB(200, 200, 200));
    LVCOLUMNW c = {};
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = (LPWSTR)L"Key"; c.cx = 250; ListView_InsertColumn(g_listPeer, 0, &c);
    c.pszText = (LPWSTR)L"Endpoint"; c.cx = 200; ListView_InsertColumn(g_listPeer, 1, &c);
    c.pszText = (LPWSTR)L"Handshake"; c.cx = 200; ListView_InsertColumn(g_listPeer, 2, &c);
    c.pszText = (LPWSTR)L"Transfer"; c.cx = 200; ListView_InsertColumn(g_listPeer, 3, &c);

    // Log
    HWND ll = CreateWindowW(L"STATIC", L"  Log",
        WS_CHILD | WS_VISIBLE, 20, 230, 200, 20, hWnd, 0, g_hInst, 0);
    SendMessage(ll, WM_SETFONT, (WPARAM)g_hFontBold, 1);

    g_listLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", 0,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOSEL,
        20, 252, 860, 110, hWnd, (HMENU)ID_LIST_LOG, g_hInst, 0);
    SendMessage(g_listLog, WM_SETFONT, (WPARAM)g_hFontMono, 1);

    // Separator
    CreateWindowW(L"STATIC", 0, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        20, 372, 860, 2, hWnd, 0, g_hInst, 0);

    // Server controls
    HWND sl = CreateWindowW(L"STATIC", L"  Server",
        WS_CHILD | WS_VISIBLE, 20, 380, 100, 20, hWnd, 0, g_hInst, 0);
    SendMessage(sl, WM_SETFONT, (WPARAM)g_hFontBold, 1);

    g_btnRefresh = CreateBtn(hWnd, L"Refresh",     20,  402, 90, 30, ID_BTN_REFRESH);
    g_btnStart   = CreateBtn(hWnd, L"Start",       120, 402, 90, 30, ID_BTN_START);
    g_btnStop    = CreateBtn(hWnd, L"Stop",        220, 402, 90, 30, ID_BTN_STOP);
    g_btnAdd     = CreateBtn(hWnd, L"Add Client",  320, 402, 100, 30, ID_BTN_ADD);
    g_btnLog     = CreateBtn(hWnd, L"Open Log",    430, 402, 100, 30, ID_BTN_LOG);

    // Anonymity levels
    HWND al = CreateWindowW(L"STATIC", L"  Anonymity Level",
        WS_CHILD | WS_VISIBLE, 20, 442, 200, 20, hWnd, 0, g_hInst, 0);
    SendMessage(al, WM_SETFONT, (WPARAM)g_hFontBold, 1);

    g_btnLvl1 = CreateBtn(hWnd, L"1: Basic VPN",      20,  464, 140, 30, ID_BTN_LVL1);
    g_btnLvl2 = CreateBtn(hWnd, L"2: + DNS Protect",  170, 464, 150, 30, ID_BTN_LVL2);
    g_btnLvl3 = CreateBtn(hWnd, L"3: Maximum",        330, 464, 130, 30, ID_BTN_LVL3);

    // Privacy tools
    HWND pl2 = CreateWindowW(L"STATIC", L"  Privacy Tools",
        WS_CHILD | WS_VISIBLE, 520, 442, 200, 20, hWnd, 0, g_hInst, 0);
    SendMessage(pl2, WM_SETFONT, (WPARAM)g_hFontBold, 1);

    g_btnKSOn   = CreateBtn(hWnd, L"Kill Switch ON",   520, 464, 130, 30, ID_BTN_KS_ON);
    g_btnKSOff  = CreateBtn(hWnd, L"Kill Switch OFF",  660, 464, 140, 30, ID_BTN_KS_OFF);
    g_btnLeak   = CreateBtn(hWnd, L"DNS Leak Test",    520, 502, 130, 30, ID_BTN_LEAK);
    g_btnHarden = CreateBtn(hWnd, L"Full Harden",      660, 502, 140, 30, ID_BTN_HARDEN);

    // Footer
    HWND foot = CreateWindowW(L"STATIC",
        L"  UDP 51820 | 10.0.0.0/24 | Auto-refresh 5s",
        WS_CHILD | WS_VISIBLE, 20, 544, 500, 20, hWnd, 0, g_hInst, 0);
    SendMessage(foot, WM_SETFONT, (WPARAM)g_hFontSmall, 1);
}

void DoStart() {
    WriteAppLog(L"Starting server...");
    SetWindowTextW(g_lblStatus, L"  STARTING  ");
    g_refreshing = true;
    ShellExecuteW(NULL, L"open", L"C:\\Program Files\\WireGuard\\wireguard.exe",
        L"/installtunnelservice C:\\WireGuard\\wg0.conf", NULL, SW_HIDE);
    Sleep(2000);
    g_refreshing = false;
    DoRefresh();
}

void DoStop() {
    WriteAppLog(L"Stopping server...");
    SetWindowTextW(g_lblStatus, L"  STOPPING  ");
    g_refreshing = true;
    ShellExecuteW(NULL, L"open", L"C:\\Program Files\\WireGuard\\wireguard.exe",
        L"/uninstalltunnelservice wg0", NULL, SW_HIDE);
    Sleep(2000);
    g_refreshing = false;
    DoRefresh();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        AddControls(hWnd);
        SetTimer(hWnd, ID_TIMER, 5000, 0);
        DoRefresh();
        return 0;

    case WM_TIMER:
        if (wParam == ID_TIMER) DoRefresh();
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BTN_REFRESH: DoRefresh(); break;
        case ID_BTN_START:   DoStart(); break;
        case ID_BTN_STOP:    DoStop(); break;
        case ID_BTN_ADD:     ShellExecuteW(NULL, L"open", L"powershell.exe",
            L"-ExecutionPolicy Bypass -NoProfile -File \"D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-privacy-addclient.ps1\"",
            NULL, SW_SHOW); break;
        case ID_BTN_LOG:     ShellExecuteW(NULL, L"open", L"notepad.exe",
            L"C:\\WireGuard\\vpn-gui.log", NULL, SW_SHOW); break;
        case ID_BTN_LVL1:    ShellExecuteW(NULL, L"open", L"powershell.exe",
            L"-ExecutionPolicy Bypass -NoProfile -File \"D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-anonymity.ps1\" -Level 1",
            NULL, SW_SHOW); Sleep(3000); DoRefresh(); break;
        case ID_BTN_LVL2:    ShellExecuteW(NULL, L"open", L"powershell.exe",
            L"-ExecutionPolicy Bypass -NoProfile -File \"D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-anonymity.ps1\" -Level 2",
            NULL, SW_SHOW); Sleep(3000); DoRefresh(); break;
        case ID_BTN_LVL3:    ShellExecuteW(NULL, L"open", L"powershell.exe",
            L"-ExecutionPolicy Bypass -NoProfile -File \"D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-anonymity.ps1\" -Level 3",
            NULL, SW_SHOW); Sleep(3000); DoRefresh(); break;
        case ID_BTN_KS_ON:   ShellExecuteW(NULL, L"open", L"powershell.exe",
            L"-ExecutionPolicy Bypass -NoProfile -File \"D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-killswitch.ps1\" -Enable",
            NULL, SW_SHOW); Sleep(2000); DoRefresh(); break;
        case ID_BTN_KS_OFF:  ShellExecuteW(NULL, L"open", L"powershell.exe",
            L"-ExecutionPolicy Bypass -NoProfile -File \"D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-killswitch.ps1\" -Disable",
            NULL, SW_SHOW); Sleep(2000); DoRefresh(); break;
        case ID_BTN_LEAK:    ShellExecuteW(NULL, L"open", L"powershell.exe",
            L"-ExecutionPolicy Bypass -NoProfile -File \"D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-leaktest.ps1\"",
            NULL, SW_SHOW); break;
        case ID_BTN_HARDEN:  ShellExecuteW(NULL, L"open", L"powershell.exe",
            L"-ExecutionPolicy Bypass -NoProfile -File \"D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-harden.ps1\"",
            NULL, SW_SHOW); Sleep(3000); DoRefresh(); break;
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
        } else if (c == g_lblLevel) {
            wchar_t buf[64] = {};
            GetWindowTextW(c, buf, 63);
            if (wcsstr(buf, L"3")) SetTextColor(hdc, RGB(0, 230, 0));
            else if (wcsstr(buf, L"2")) SetTextColor(hdc, RGB(0, 180, 230));
            else SetTextColor(hdc, RGB(200, 200, 0));
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
        KillTimer(hWnd, ID_TIMER);
        DeleteObject(g_hBrushBg);
        DeleteObject(g_hFont);
        DeleteObject(g_hFontBold);
        DeleteObject(g_hFontMono);
        DeleteObject(g_hFontSmall);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nShow) {
    g_hInst = hInst;
    WriteAppLog(L"GUI started");

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

    g_hWnd = CreateWindowExW(0, CLASS_NAME, L"VPN-TEIVRIM Monitor",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (sw - 920) / 2, (sh - 600) / 2, 920, 600,
        0, 0, hInst, 0);

    ShowWindow(g_hWnd, nShow);
    UpdateWindow(g_hWnd);

    MSG m;
    while (GetMessageW(&m, 0, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
    return 0;
}
