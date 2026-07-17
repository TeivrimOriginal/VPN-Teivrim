#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <cstdio>
#include <process.h>
#include "qrcodegen.hpp"

#pragma comment(lib, "comctl32.lib")

// Control IDs
enum {
    ID_BTN_REFRESH=101, ID_BTN_START, ID_BTN_STOP, ID_BTN_ADD, ID_BTN_LOG,
    ID_LIST_PEER, ID_LIST_LOG, ID_LBL_STATUS, ID_LBL_PORT, ID_LBL_KEY,
    ID_LBL_IP, ID_TIMER, ID_BTN_LVL1, ID_BTN_LVL2, ID_BTN_LVL3,
    ID_BTN_KS_ON, ID_BTN_KS_OFF, ID_BTN_LEAK, ID_BTN_HARDEN,
    ID_LBL_LVL, ID_LBL_TRAFFIC, ID_LBL_UPTIME, ID_BTN_REBOOT,
    ID_BTN_EXPORT, ID_BTN_ABOUT, ID_BTN_QR, ID_BTN_COPY, ID_SPARKLINE, ID_BTN_REMOVE,
    ID_QR_COPY = 9001, ID_QR_CLOSE = 9002
};

const wchar_t CLASS_NAME[] = L"WGMon4";
HINSTANCE g_hInst;
HWND g_hWnd;
HWND g_lblStatus, g_lblPort, g_lblKey, g_lblIp, g_lblLevel;
HWND g_lblTraffic, g_lblUptime;
HWND g_listPeer, g_listLog;
HFONT g_hFont, g_hFontBold, g_hFontMono, g_hFontSmall;
HBRUSH g_hBrushBg;
HWND g_btnRefresh, g_btnStart, g_btnStop, g_btnAdd, g_btnLog;
HWND g_btnLvl1, g_btnLvl2, g_btnLvl3;
HWND g_btnKSOn, g_btnKSOff, g_btnLeak, g_btnHarden, g_btnReboot, g_btnExport;
HWND g_btnQr, g_btnCopy, g_btnRemove;
bool g_refreshing = false;
int g_currentLevel = 0;
wchar_t g_appDir[MAX_PATH];
DWORD g_startTick = 0;

// Speed tracking
double g_lastRx = 0, g_lastTx = 0;
DWORD g_lastTick = 0;

// Tray icon
#define WM_TRAYICON (WM_USER + 1)
NOTIFYICONDATAW g_nid = {};

// QR code
using qrcodegen::QrCode;
QrCode* g_qr = nullptr;
std::wstring g_qrText;

// Traffic sparkline history (rx, tx speed samples)
std::vector<double> g_histRx;
std::vector<double> g_histTx;
const int MAX_HIST = 120;
std::wstring g_publicIp;
std::wstring g_lanIp;
bool g_publicIpTried = false;
bool g_publicIpFetching = false;
std::vector<std::wstring> g_peerKeys;
std::wstring g_removeKey;

// --- Crash handler ---
LONG WINAPI CrashFilter(EXCEPTION_POINTERS* ep) {
    FILE* f = NULL;
    _wfopen_s(&f, L"C:\\VPN-TEIVRIM\\crash.log", L"a");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] CRASH code=0x%08X addr=%p\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
            ep ? ep->ExceptionRecord->ExceptionCode : 0,
            ep ? ep->ExceptionRecord->ExceptionAddress : 0);
        fclose(f);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

// --- Logging ---
void WriteLog(const char* msg) {
    wchar_t path[MAX_PATH];
    wsprintfW(path, L"%s\\vpn-gui.log", g_appDir);
    FILE* f = NULL;
    _wfopen_s(&f, path, L"a");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, msg);
        fclose(f);
    }
}

void WriteLogW(const wchar_t* msg) {
    char mb[512] = {};
    WideCharToMultiByte(CP_UTF8, 0, msg, -1, mb, sizeof(mb), NULL, NULL);
    WriteLog(mb);
}

std::wstring GetLine(const std::wstring& s, const wchar_t* key);

// --- Process execution ---
struct ExecReadCtx {
    HANDLE hRead;
    std::wstring* result;
};

static DWORD WINAPI ExecReadThread(LPVOID p) {
    ExecReadCtx* ctx = (ExecReadCtx*)p;
    char tmp[4096];
    DWORD n = 0;
    while (ReadFile(ctx->hRead, tmp, sizeof(tmp) - 1, &n, NULL) && n > 0) {
        tmp[n] = 0;
        int wl = MultiByteToWideChar(CP_UTF8, 0, tmp, -1, 0, 0);
        if (wl > 0) {
            wchar_t* wb = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, wl * sizeof(wchar_t));
            if (wb) {
                MultiByteToWideChar(CP_UTF8, 0, tmp, -1, wb, wl);
                *(ctx->result) += wb;
                HeapFree(GetProcessHeap(), 0, wb);
            }
        }
    }
    return 0;
}

// Timeout-safe: reads child output in a worker thread, waits for the process
// with a bounded timeout, kills it if it hangs, and closes the pipe so a
// grandchild inheriting the handle can never block us forever.
std::wstring ExecCmd(const wchar_t* cmdline, DWORD timeoutMs = 5000) {
    HANDLE hR, hW;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    if (!CreatePipe(&hR, &hW, &sa, 0)) return L"";
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hW;
    si.hStdError = hW;
    wchar_t buf[1024];
    wcscpy_s(buf, cmdline);
    PROCESS_INFORMATION pi = {};
    BOOL ok = CreateProcessW(NULL, buf, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(hW);
    if (!ok) { CloseHandle(hR); return L""; }

    std::wstring result;
    ExecReadCtx ctx = { hR, &result };
    HANDLE hThread = CreateThread(NULL, 0, ExecReadThread, &ctx, 0, NULL);

    DWORD wr = WaitForSingleObject(pi.hProcess, timeoutMs);
    if (wr == WAIT_TIMEOUT) TerminateProcess(pi.hProcess, 1);

    // Wait for the reader to drain the pipe. It ends on EOF once the child
    // closes stdout. Only force-close the read end if the reader is stuck
    // (e.g. a service inherited the pipe write end and keeps it open).
    bool readClosed = false;
    if (hThread) {
        if (WaitForSingleObject(hThread, timeoutMs + 2000) == WAIT_TIMEOUT) {
            CloseHandle(hR);
            readClosed = true;
            WaitForSingleObject(hThread, 1000);
        }
        CloseHandle(hThread);
    }
    if (!readClosed) CloseHandle(hR);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return result;
}

static void UpdateIpLabel() {
    SetWindowTextW(g_lblIp, (L"  LAN: " + g_lanIp + (g_publicIp.empty() ? L"" : (L"  |  WAN: " + g_publicIp))).c_str());
}

// Fetch public IP once, in the background, so the UI never blocks on the network.
static DWORD WINAPI FetchPublicIpThread(LPVOID) {
    g_publicIpFetching = true;
    std::wstring pip = ExecCmd(L"curl.exe -s --max-time 4 https://api.ipify.org", 6000);
    pip = GetLine(pip, L"");
    if (!pip.empty() && pip.find(L".") != std::wstring::npos) {
        size_t trim = pip.find_last_not_of(L"\r\n ");
        if (trim != std::wstring::npos) pip = pip.substr(0, trim + 1);
        g_publicIp = pip;
    }
    g_publicIpFetching = false;
    g_publicIpTried = true;
    if (g_hWnd) PostMessageW(g_hWnd, WM_USER + 101, 0, 0);
    return 0;
}

std::wstring ExecWG(const wchar_t* args) {
    wchar_t cmd[512];
    wsprintfW(cmd, L"\"C:\\Program Files\\WireGuard\\wg.exe\" %s", args);
    return ExecCmd(cmd, 3000);
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

// Parse size like "1.23 MiB" -> bytes as double
double ParseSize(const std::wstring& s) {
    if (s.empty()) return 0;
    // Extract number
    size_t numEnd = s.find_first_not_of(L"0123456789.,");
    if (numEnd == std::wstring::npos) numEnd = s.size();
    std::wstring numStr = s.substr(0, numEnd);
    double val = 0;
    swscanf_s(numStr.c_str(), L"%lf", &val);

    // Find unit
    size_t unitStart = s.find_first_not_of(L" \t", numEnd);
    std::wstring unit = (unitStart != std::wstring::npos) ? s.substr(unitStart) : L"";
    if (unit.find(L"KiB") != std::wstring::npos) val *= 1024.0;
    else if (unit.find(L"MiB") != std::wstring::npos) val *= 1024.0 * 1024.0;
    else if (unit.find(L"GiB") != std::wstring::npos) val *= 1024.0 * 1024.0 * 1024.0;
    else if (unit.find(L"TiB") != std::wstring::npos) val *= 1024.0 * 1024.0 * 1024.0 * 1024.0;
    else if (unit.find(L"kB") != std::wstring::npos) val *= 1000.0;
    else if (unit.find(L"MB") != std::wstring::npos) val *= 1000.0 * 1000.0;
    else if (unit.find(L"GB") != std::wstring::npos) val *= 1000.0 * 1000.0 * 1000.0;
    return val;
}

// Format bytes as human-readable string
std::wstring FormatSize(double bytes) {
    wchar_t buf[64];
    if (bytes < 1024.0) {
        wsprintfW(buf, L"%.0f B", bytes);
    } else if (bytes < 1024.0 * 1024.0) {
        wsprintfW(buf, L"%.1f KiB", bytes / 1024.0);
    } else if (bytes < 1024.0 * 1024.0 * 1024.0) {
        wsprintfW(buf, L"%.2f MiB", bytes / (1024.0 * 1024.0));
    } else if (bytes < 1024.0 * 1024.0 * 1024.0 * 1024.0) {
        wsprintfW(buf, L"%.2f GiB", bytes / (1024.0 * 1024.0 * 1024.0));
    } else {
        wsprintfW(buf, L"%.2f TiB", bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0));
    }
    return std::wstring(buf);
}

std::wstring FormatSpeed(double bytesPerSec) {
    if (bytesPerSec < 1.0) return L"0 B/s";
    return FormatSize(bytesPerSec) + L"/s";
}

// --- File operations ---
std::wstring ReadLog(int maxLines) {
    wchar_t path[MAX_PATH];
    wsprintfW(path, L"%s\\vpn-gui.log", g_appDir);
    FILE* f = NULL;
    _wfopen_s(&f, path, L"rb");
    if (!f) {
        _wfopen_s(&f, L"C:\\WireGuard\\vpn-monitor.log", L"rb");
    }
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

// Forward declarations
std::wstring TrimWS(const std::wstring&);
int CountPeers();
std::wstring GetServerPubKey();
void AppendPeer(const std::wstring&, const std::wstring&);
void WriteClientConf(int, const std::wstring&, const std::wstring&, const std::wstring&, const std::wstring&);
void RemovePeerByKey(const std::wstring&);
void ShowQRDialog(HWND, const std::wstring&);

// --- Background thread for long operations ---
struct ThreadData {
    int action;
    HWND hWnd;
};

unsigned __stdcall BackgroundThread(void* param) {
    ThreadData* td = (ThreadData*)param;
    switch (td->action) {
    case 1: WriteLog("Starting server...");
        ExecCmd(L"\"C:\\Program Files\\WireGuard\\wireguard.exe\" /installtunnelservice C:\\WireGuard\\wg0.conf", 3000);
        Sleep(2000); break;
    case 2: WriteLog("Stopping server...");
        ExecCmd(L"\"C:\\Program Files\\WireGuard\\wireguard.exe\" /uninstalltunnelservice wg0", 3000);
        Sleep(2000); break;
    case 3: WriteLog("Level 1...");
        ExecCmd(L"powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \"& 'D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-anonymity.ps1' -Level 1\"", 15000); break;
    case 4: WriteLog("Level 2...");
        ExecCmd(L"powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \"& 'D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-anonymity.ps1' -Level 2\"", 15000); break;
    case 5: WriteLog("Level 3...");
        ExecCmd(L"powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \"& 'D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-anonymity.ps1' -Level 3\"", 15000); break;
    case 6: WriteLog("Kill switch ON...");
        ExecCmd(L"powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \"& 'D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-killswitch.ps1' -Enable\"", 10000); break;
    case 7: WriteLog("Kill switch OFF...");
        ExecCmd(L"powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \"& 'D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-killswitch.ps1' -Disable\"", 10000); break;
    case 8: WriteLog("Full hardening...");
        ExecCmd(L"powershell.exe -ExecutionPolicy Bypass -NoProfile -Command \"& 'D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-harden.ps1'\"", 20000); break;
    case 9: {
        WriteLog("Adding client...");
        std::wstring priv = TrimWS(ExecCmd(L"\"C:\\Program Files\\WireGuard\\wg.exe\" genkey", 3000));
        std::wstring pub = TrimWS(ExecCmd((std::wstring(L"cmd.exe /c echo ") + priv + L" | \"C:\\Program Files\\WireGuard\\wg.exe\" pubkey").c_str(), 3000));
        if (priv.empty() || pub.empty()) { WriteLog("Key generation failed"); break; }
        int peers = CountPeers();
        int idx = peers;
        std::wstring ip = L"10.0.0." + std::to_wstring(peers + 2);
        std::wstring wan = g_publicIp.empty() ? L"91.212.68.231" : g_publicIp;
        std::wstring serverPub = GetServerPubKey();
        AppendPeer(pub, ip);
        WriteClientConf(idx, priv, serverPub, wan, ip);
        ExecCmd(L"\"C:\\Program Files\\WireGuard\\wireguard.exe\" /uninstalltunnelservice wg0", 3000);
        Sleep(1500);
        ExecCmd(L"\"C:\\Program Files\\WireGuard\\wireguard.exe\" /installtunnelservice C:\\WireGuard\\wg0.conf", 3000);
        Sleep(1500);
        std::wstring added = L"Client " + std::to_wstring(idx) + L" added, IP " + ip;
        WriteLogW(added.c_str());
        wchar_t cpath[MAX_PATH];
        wsprintfW(cpath, L"C:\\WireGuard\\client%d.conf", idx);
        ShowQRDialog(g_hWnd, cpath);
        PostMessageW(g_hWnd, WM_USER + 100, 0, 0);
        break;
    }
    case 10: {
        if (g_removeKey.empty()) break;
        WriteLog("Removing client...");
        RemovePeerByKey(g_removeKey);
        ExecCmd(L"\"C:\\Program Files\\WireGuard\\wireguard.exe\" /uninstalltunnelservice wg0", 3000);
        Sleep(1500);
        ExecCmd(L"\"C:\\Program Files\\WireGuard\\wireguard.exe\" /installtunnelservice C:\\WireGuard\\wg0.conf", 3000);
        Sleep(1500);
        g_removeKey.clear();
        WriteLog("Client removed");
        PostMessageW(g_hWnd, WM_USER + 100, 0, 0);
        break;
    }
    }
    g_refreshing = false;
    delete td;
    return 0;
}

void RunAsync(int action) {
    if (g_refreshing) return;
    g_refreshing = true;
    ThreadData* td = new ThreadData{ action, g_hWnd };
    _beginthreadex(NULL, 0, BackgroundThread, td, 0, NULL);
}

// --- Refresh UI ---
void DoRefresh() {
    if (g_refreshing) return;
    g_refreshing = true;
    SendMessage(g_hWnd, WM_SETREDRAW, FALSE, 0);

    // Server status
    std::wstring wg = ExecWG(L"show wg0");
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

    // IP
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
    g_lanIp = lip;
    UpdateIpLabel();

    // Public IP fetched once in the background (never blocks the UI thread)
    if (g_publicIp.empty() && !g_publicIpTried && !g_publicIpFetching) {
        g_publicIpFetching = true;
        HANDLE h = CreateThread(NULL, 0, FetchPublicIpThread, NULL, 0, NULL);
        if (h) CloseHandle(h);
    }

    // Anonymity level
    bool hasIPv6 = (ipRaw.find(L"IPv6") != std::wstring::npos && ipRaw.find(L"disabled") == std::wstring::npos);
    std::wstring ksCheck = ExecCmd(L"cmd.exe /c netsh advfirewall firewall show rule name=all dir=out | findstr /i \"L3-KS\"", 3000);
    bool hasKS = (ksCheck.find(L"L3-KS") != std::wstring::npos);
    int level = 1;
    if (!hasIPv6) level = 2;
    if (hasKS) level = 3;
    g_currentLevel = level;
    wchar_t lvlBuf[128];
    wsprintfW(lvlBuf, L"  Level %d", level);
    SetWindowTextW(g_lblLevel, lvlBuf);

    // Traffic stats - sum across all peers
    std::wstring rxTotal = L"0 B";
    std::wstring txTotal = L"0 B";
    double rxSum = 0, txSum = 0;
    {
        size_t pp2 = 0;
        while ((pp2 = wg.find(L"transfer:", pp2)) != std::wstring::npos) {
            size_t e2 = wg.find(L'\n', pp2);
            if (e2 == std::wstring::npos) e2 = wg.size();
            std::wstring line = wg.substr(pp2, e2 - pp2);
            // Format: "transfer: 1.23 MiB received, 4.56 MiB sent"
            size_t rxPos = line.find(L"received");
            size_t sentPos = line.find(L"sent");
            if (rxPos != std::wstring::npos) {
                size_t valStart = line.find(L":") + 1;
                std::wstring rxStr = line.substr(valStart, rxPos - valStart);
                rxSum += ParseSize(rxStr);
            }
            if (sentPos != std::wstring::npos) {
                size_t valStart = line.find(L",") + 1;
                std::wstring txStr = line.substr(valStart, sentPos - valStart);
                txSum += ParseSize(txStr);
            }
            pp2 = e2 + 1;
        }
        rxTotal = FormatSize(rxSum);
        txTotal = FormatSize(txSum);
    }
    wchar_t trafficBuf[256];
    if (on) {
        // Real-time speed from delta between refreshes
        DWORD now = GetTickCount();
        double rxSpeed = 0, txSpeed = 0;
        double dt = (g_lastTick != 0) ? ((now - g_lastTick) / 1000.0) : 0;
        if (dt > 0.5) {
            double dr = rxSum - g_lastRx;
            double dt2 = txSum - g_lastTx;
            if (dr < 0) dr = 0;
            if (dt2 < 0) dt2 = 0;
            rxSpeed = dr / dt;
            txSpeed = dt2 / dt;
        }
        g_lastRx = rxSum; g_lastTx = txSum; g_lastTick = now;
        if (g_histRx.size() >= MAX_HIST) { g_histRx.erase(g_histRx.begin()); g_histTx.erase(g_histTx.begin()); }
        g_histRx.push_back(rxSpeed);
        g_histTx.push_back(txSpeed);
        wsprintfW(trafficBuf, L"  ↓ %s (%s)  ↑ %s (%s)",
            rxTotal.c_str(), FormatSpeed(rxSpeed).c_str(),
            txTotal.c_str(), FormatSpeed(txSpeed).c_str());
        SetWindowTextW(g_lblTraffic, trafficBuf);
    } else {
        g_lastRx = 0; g_lastTx = 0; g_lastTick = 0;
        if (!g_histRx.empty()) { g_histRx.push_back(0); g_histTx.push_back(0); if (g_histRx.size() > MAX_HIST) { g_histRx.erase(g_histRx.begin()); g_histTx.erase(g_histTx.begin()); } }
        SetWindowTextW(g_lblTraffic, L"  Traffic: сервер выключен");
    }

    // Uptime
    DWORD uptime = (GetTickCount() - g_startTick) / 1000;
    int hours = uptime / 3600;
    int mins = (uptime % 3600) / 60;
    int secs = uptime % 60;
    wchar_t upBuf[64];
    wsprintfW(upBuf, L"  Uptime: %02d:%02d:%02d", hours, mins, secs);
    SetWindowTextW(g_lblUptime, upBuf);

    // Peers
    ListView_DeleteAllItems(g_listPeer);
    g_peerKeys.clear();
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
            g_peerKeys.push_back(key);
            if (key.size() > 22) key = key.substr(0, 22) + L"...";
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

    // Log
    std::wstring logContent = ReadLog(20);
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
    HWND spark = GetDlgItem(g_hWnd, ID_SPARKLINE);
    if (spark) InvalidateRect(spark, NULL, FALSE);
    UpdateWindow(g_hWnd);
    g_refreshing = false;
}

HWND MakeBtn(HWND parent, const wchar_t* text, int x, int y, int w, int h, int id) {
    HWND btn = CreateWindowW(L"BUTTON", text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, g_hInst, 0);
    SendMessage(btn, WM_SETFONT, (WPARAM)g_hFont, 1);
    return btn;
}

HWND MakeLabel(HWND parent, const wchar_t* text, int x, int y, int w, int h, HFONT font) {
    HWND lbl = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent, 0, g_hInst, 0);
    SendMessage(lbl, WM_SETFONT, (WPARAM)font, 1);
    return lbl;
}

// Build a simple 16x16 tray icon (green rounded square)
HICON MakeTrayIcon() {
    HDC hdc = GetDC(NULL);
    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP hbmColor = CreateCompatibleBitmap(hdc, 16, 16);
    HBITMAP hOld = (HBITMAP)SelectObject(mem, hbmColor);
    RECT r = { 0, 0, 16, 16 };
    FillRect(mem, &r, (HBRUSH)GetStockObject(BLACK_BRUSH));
    HBRUSH br = CreateSolidBrush(RGB(0, 200, 120));
    RECT rc = { 2, 2, 14, 14 };
    FillRect(mem, &rc, br);
    DeleteObject(br);
    SelectObject(mem, hOld);
    DeleteDC(mem);

    HBITMAP hbmMask = CreateBitmap(16, 16, 1, 1, NULL);
    HDC mem2 = CreateCompatibleDC(hdc);
    HBITMAP hOld2 = (HBITMAP)SelectObject(mem2, hbmMask);
    RECT rm = { 0, 0, 16, 16 };
    FillRect(mem2, &rm, (HBRUSH)GetStockObject(WHITE_BRUSH));
    SelectObject(mem2, hOld2);
    DeleteDC(mem2);
    ReleaseDC(NULL, hdc);

    ICONINFO ii = { TRUE, 0, 0, hbmMask, hbmColor };
    HICON icon = CreateIconIndirect(&ii);
    DeleteObject(hbmMask);
    DeleteObject(hbmColor);
    return icon;
}

// Read a UTF-8 text file fully into a wstring
std::wstring ReadFileText(const wchar_t* path) {
    FILE* f = NULL;
    _wfopen_s(&f, path, L"rb");
    if (!f) return L"";
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz <= 0) { fclose(f); return L""; }
    fseek(f, 0, SEEK_SET);
    char* d = (char*)HeapAlloc(GetProcessHeap(), 0, sz + 1);
    if (!d) { fclose(f); return L""; }
    fread(d, 1, sz, f);
    d[sz] = 0;
    fclose(f);
    int wl = MultiByteToWideChar(CP_UTF8, 0, d, -1, 0, 0);
    wchar_t* wd = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, wl * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, d, -1, wd, wl);
    std::wstring out(wd);
    HeapFree(GetProcessHeap(), 0, d);
    HeapFree(GetProcessHeap(), 0, wd);
    return out;
}

void WriteFileText(const wchar_t* path, const std::wstring& content) {
    int n = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, 0, 0, NULL, NULL);
    if (n <= 0) return;
    std::string utf8(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, &utf8[0], n, NULL, NULL);
    FILE* f = NULL;
    _wfopen_s(&f, path, L"wb");
    if (f) { fwrite(utf8.data(), 1, utf8.size(), f); fclose(f); }
}

// --- Client management ---
std::wstring TrimWS(const std::wstring& s) {
    size_t a = s.find_first_not_of(L" \t\r\n");
    if (a == std::wstring::npos) return L"";
    size_t b = s.find_last_not_of(L" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::wstring ReadServerConf() {
    return ReadFileText(L"C:\\WireGuard\\wg0.conf");
}

std::wstring GetServerPubKey() {
    return TrimWS(GetLine(ReadServerConf(), L"PublicKey = "));
}

int CountPeers() {
    std::wstring cfg = ReadServerConf();
    int c = 0; size_t p = 0;
    while ((p = cfg.find(L"[Peer]", p)) != std::wstring::npos) { c++; p++; }
    return c;
}

void AppendPeer(const std::wstring& pub, const std::wstring& ip) {
    std::wstring cfg = ReadServerConf();
    if (!cfg.empty() && cfg.back() != L'\n') cfg += L"\n";
    cfg += L"[Peer]\nPublicKey = " + pub + L"\nAllowedIPs = " + ip + L"/32\n";
    WriteFileText(L"C:\\WireGuard\\wg0.conf", cfg);
}

void WriteClientConf(int idx, const std::wstring& priv, const std::wstring& serverPub,
                     const std::wstring& wan, const std::wstring& ip) {
    wchar_t path[MAX_PATH];
    wsprintfW(path, L"C:\\WireGuard\\client%d.conf", idx);
    std::wstring cfg = L"[Interface]\nPrivateKey = " + priv + L"\nAddress = " + ip + L"/24\nDNS = 1.1.1.1\n\n[Peer]\nPublicKey = " + serverPub + L"\nEndpoint = " + wan + L":51820\nAllowedIPs = 0.0.0.0/0\nPersistentKeepalive = 25\n";
    WriteFileText(path, cfg);
}

void RemovePeerByKey(const std::wstring& pub) {
    std::wstring cfg = ReadServerConf();
    std::wstring out;
    size_t pos = 0;
    bool first = true;
    while (pos < cfg.size()) {
        size_t nb = cfg.find(L"[", pos);
        if (nb == std::wstring::npos) break;
        size_t ne = cfg.find(L"]", nb);
        if (ne == std::wstring::npos) break;
        std::wstring blockName = cfg.substr(nb, ne - nb + 1);
        size_t next = cfg.find(L"[", ne + 1);
        std::wstring block = cfg.substr(nb, (next == std::wstring::npos ? cfg.size() : next) - nb);
        if (blockName == L"[Peer]") {
            std::wstring bp = TrimWS(GetLine(block, L"PublicKey = "));
            if (bp == pub) { pos = (next == std::wstring::npos ? cfg.size() : next); continue; }
        }
        if (!first) out += L"\n";
        out += block;
        first = false;
        if (next == std::wstring::npos) break;
        pos = next;
    }
    WriteFileText(L"C:\\WireGuard\\wg0.conf", out);
}

void CopyTextToClipboard(HWND h, const std::wstring& text) {
    if (text.empty()) return;
    if (!OpenClipboard(h)) return;
    EmptyClipboard();
    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
    if (hg) {
        wchar_t* p = (wchar_t*)GlobalLock(hg);
        wcscpy_s(p, text.size() + 1, text.c_str());
        GlobalUnlock(hg);
        SetClipboardData(CF_UNICODETEXT, hg);
    }
    CloseClipboard();
}

void DrawQRToDC(HDC hdc, const RECT& rc, const QrCode& qr, int border) {
    int size = qr.getSize();
    int total = size + border * 2;
    int cell = (rc.right - rc.left) / total;
    if (cell < 1) cell = 1;
    int offx = rc.left + ((rc.right - rc.left) - cell * total) / 2;
    int offy = rc.top + ((rc.bottom - rc.top) - cell * total) / 2;
    HBRUSH white = (HBRUSH)GetStockObject(WHITE_BRUSH);
    HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
    RECT bg = { offx, offy, offx + cell * total, offy + cell * total };
    FillRect(hdc, &bg, white);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qr.getModule(x, y)) {
                RECT m = { offx + (x + border) * cell, offy + (y + border) * cell,
                           offx + (x + border + 1) * cell, offy + (y + border + 1) * cell };
                FillRect(hdc, &m, black);
            }
        }
    }
    DeleteObject(black);
}

void DrawSparkline(HWND hwnd, HDC hdc, const RECT& rc) {
    HBRUSH bg = CreateSolidBrush(RGB(14, 16, 22));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);
    // border
    HBRUSH brd = CreateSolidBrush(RGB(50, 54, 64));
    FrameRect(hdc, &rc, brd);
    DeleteObject(brd);

    int n = (int)g_histRx.size();
    if (n < 2) {
        SetTextColor(hdc, RGB(120, 120, 120));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, L"нет данных", -1, (RECT*)&rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return;
    }
    double mx = 1.0;
    for (int i = 0; i < n; i++) {
        if (g_histRx[i] > mx) mx = g_histRx[i];
        if (g_histTx[i] > mx) mx = g_histTx[i];
    }
    int pad = 6;
    int w = rc.right - rc.left - pad * 2;
    int h = rc.bottom - rc.top - pad * 2;
    int x0 = rc.left + pad, y0 = rc.top + pad;

    // grid baseline
    HPEN grid = CreatePen(PS_SOLID, 1, RGB(40, 44, 54));
    SelectObject(hdc, grid);
    MoveToEx(hdc, x0, y0 + h, NULL);
    LineTo(hdc, x0 + w, y0 + h);
    DeleteObject(grid);

    auto drawLine = [&](const std::vector<double>& v, COLORREF col) {
        HPEN pen = CreatePen(PS_SOLID, 2, col);
        SelectObject(hdc, pen);
        for (int i = 0; i < n; i++) {
            int x = x0 + (int)((double)i / (n - 1) * w);
            int y = y0 + h - (int)(v[i] / mx * h);
            if (i == 0) MoveToEx(hdc, x, y, NULL);
            else LineTo(hdc, x, y);
        }
        DeleteObject(pen);
    };
    drawLine(g_histRx, RGB(0, 220, 120));
    drawLine(g_histTx, RGB(80, 160, 255));
}

LRESULT CALLBACK QRWndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(h, &ps);
        RECT rc;
        GetClientRect(h, &rc);
        RECT qrRc = { rc.left + 12, rc.top + 12, rc.right - 12, rc.bottom - 56 };
        if (g_qr) DrawQRToDC(hdc, qrRc, *g_qr, 4);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(w) == ID_QR_COPY) {
            CopyTextToClipboard(h, g_qrText);
            MessageBoxW(h, L"Конфиг скопирован в буфер", L"QR", MB_ICONINFORMATION);
        } else if (LOWORD(w) == ID_QR_CLOSE) {
            DestroyWindow(h);
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(h);
        return 0;
    case WM_DESTROY:
        if (g_qr) { delete g_qr; g_qr = nullptr; }
        return 0;
    }
    return DefWindowProcW(h, msg, w, l);
}

void ShowQRDialog(HWND parent, const std::wstring& configPath) {
    std::wstring text = ReadFileText(configPath.c_str());
    if (text.empty()) {
        MessageBoxW(parent, L"Конфиг клиента не найден", L"QR", MB_ICONERROR);
        return;
    }
    g_qrText = text;
    int n = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, 0, 0, NULL, NULL);
    std::string utf8(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, &utf8[0], n, NULL, NULL);
    if (g_qr) delete g_qr;
    g_qr = new QrCode(QrCode::encodeText(utf8.c_str(), QrCode::Ecc::MEDIUM));

    static bool reg = false;
    if (!reg) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = QRWndProc;
        wc.hInstance = g_hInst;
        wc.lpszClassName = L"QRWin";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(RGB(18, 18, 24));
        RegisterClassW(&wc);
        reg = true;
    }
    HWND h = CreateWindowExW(WS_EX_DLGMODALFRAME, L"QRWin", L"QR — отсканируй в WireGuard",
        WS_OVERLAPPEDWINDOW, 0, 0, 380, 440, parent, 0, g_hInst, 0);
    int sw = GetSystemMetrics(SM_CXSCREEN), sh = GetSystemMetrics(SM_CYSCREEN);
    SetWindowPos(h, NULL, (sw - 380) / 2, (sh - 440) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    CreateWindowW(L"BUTTON", L"Копировать", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        40, 372, 140, 30, h, (HMENU)ID_QR_COPY, g_hInst, 0);
    CreateWindowW(L"BUTTON", L"Закрыть", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        200, 372, 140, 30, h, (HMENU)ID_QR_CLOSE, g_hInst, 0);
    ShowWindow(h, SW_SHOW);
}

void AddControls(HWND hWnd) {
    g_hBrushBg = CreateSolidBrush(RGB(18, 18, 24));
    g_hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI");
    g_hFontBold = CreateFontW(18, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI");
    g_hFontMono = CreateFontW(13, 0, 0, 0, FW_NORMAL, 0, 0, 0, FIXED_PITCH, 0, 0, 0, 0, L"Consolas");
    g_hFontSmall = CreateFontW(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI");

    // Header
    MakeLabel(hWnd, L"VPN-TEIVRIM", 16, 6, 200, 28, g_hFontBold);
    g_lblStatus = CreateWindowW(L"STATIC", L"  WAIT  ",
        WS_CHILD | WS_VISIBLE | SS_CENTER, 580, 4, 100, 28, hWnd, (HMENU)ID_LBL_STATUS, g_hInst, 0);
    SendMessage(g_lblStatus, WM_SETFONT, (WPARAM)g_hFontBold, 1);

    g_lblLevel = CreateWindowW(L"STATIC", L"  Level ?",
        WS_CHILD | WS_VISIBLE, 690, 8, 100, 20, hWnd, (HMENU)ID_LBL_LVL, g_hInst, 0);
    SendMessage(g_lblLevel, WM_SETFONT, (WPARAM)g_hFontSmall, 1);

    g_lblUptime = CreateWindowW(L"STATIC", L"  Uptime: 00:00:00",
        WS_CHILD | WS_VISIBLE, 800, 8, 140, 20, hWnd, (HMENU)ID_LBL_UPTIME, g_hInst, 0);
    SendMessage(g_lblUptime, WM_SETFONT, (WPARAM)g_hFontSmall, 1);

    CreateWindowW(L"STATIC", 0, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        16, 36, 920, 2, hWnd, 0, g_hInst, 0);

    // Info row
    g_lblPort = MakeLabel(hWnd, L"  Port: ---", 16, 44, 300, 18, g_hFontMono);
    g_lblIp   = MakeLabel(hWnd, L"  LAN: ---", 320, 44, 360, 18, g_hFontMono);
    g_lblTraffic = CreateWindowW(L"STATIC", L"  Traffic: ---",
        WS_CHILD | WS_VISIBLE, 690, 44, 230, 18, hWnd, (HMENU)ID_LBL_TRAFFIC, g_hInst, 0);
    SendMessage(g_lblTraffic, WM_SETFONT, (WPARAM)g_hFontMono, 1);
    g_lblKey  = MakeLabel(hWnd, L"  Key: ---", 16, 64, 900, 18, g_hFontMono);

    // Peers
    MakeLabel(hWnd, L"  Connected Peers", 16, 88, 200, 20, g_hFontBold);
    g_listPeer = CreateWindowExW(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER,
        WC_LISTVIEWW, 0,
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_NOCOLUMNHEADER,
        16, 110, 920, 90, hWnd, (HMENU)ID_LIST_PEER, g_hInst, 0);
    SendMessage(g_listPeer, WM_SETFONT, (WPARAM)g_hFontMono, 1);
    ListView_SetBkColor(g_listPeer, RGB(22, 22, 28));
    ListView_SetTextColor(g_listPeer, RGB(200, 200, 200));
    LVCOLUMNW col = {};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.pszText = (LPWSTR)L"Key"; col.cx = 260; ListView_InsertColumn(g_listPeer, 0, &col);
    col.pszText = (LPWSTR)L"Endpoint"; col.cx = 210; ListView_InsertColumn(g_listPeer, 1, &col);
    col.pszText = (LPWSTR)L"Handshake"; col.cx = 220; ListView_InsertColumn(g_listPeer, 2, &col);
    col.pszText = (LPWSTR)L"Transfer"; col.cx = 220; ListView_InsertColumn(g_listPeer, 3, &col);

    // Log
    MakeLabel(hWnd, L"  Log", 16, 208, 100, 20, g_hFontBold);
    g_listLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", 0,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOSEL,
        16, 230, 920, 90, hWnd, (HMENU)ID_LIST_LOG, g_hInst, 0);
    SendMessage(g_listLog, WM_SETFONT, (WPARAM)g_hFontMono, 1);

    CreateWindowW(L"STATIC", 0, WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
        16, 328, 920, 2, hWnd, 0, g_hInst, 0);

    // Server buttons
    MakeLabel(hWnd, L"  Server", 16, 336, 100, 18, g_hFontBold);
    g_btnRefresh = MakeBtn(hWnd, L"Refresh",     16,  356, 85, 28, ID_BTN_REFRESH);
    g_btnStart   = MakeBtn(hWnd, L"Start",       110, 356, 85, 28, ID_BTN_START);
    g_btnStop    = MakeBtn(hWnd, L"Stop",        204, 356, 85, 28, ID_BTN_STOP);
    g_btnAdd     = MakeBtn(hWnd, L"Add Client",  300, 356, 95, 28, ID_BTN_ADD);
    g_btnLog     = MakeBtn(hWnd, L"Open Log",    404, 356, 95, 28, ID_BTN_LOG);
    g_btnExport  = MakeBtn(hWnd, L"Export",      508, 356, 85, 28, ID_BTN_EXPORT);
    g_btnQr      = MakeBtn(hWnd, L"QR Code",     600, 356, 85, 28, ID_BTN_QR);
    g_btnCopy    = MakeBtn(hWnd, L"Copy",        692, 356, 85, 28, ID_BTN_COPY);
    g_btnRemove  = MakeBtn(hWnd, L"Remove",      784, 356, 85, 28, ID_BTN_REMOVE);

    // Anonymity
    MakeLabel(hWnd, L"  Anonymity", 16, 392, 150, 18, g_hFontBold);
    g_btnLvl1 = MakeBtn(hWnd, L"1: Basic VPN",      16,  412, 130, 28, ID_BTN_LVL1);
    g_btnLvl2 = MakeBtn(hWnd, L"2: + DNS Shield",   154, 412, 140, 28, ID_BTN_LVL2);
    g_btnLvl3 = MakeBtn(hWnd, L"3: MAXIMUM",        302, 412, 130, 28, ID_BTN_LVL3);

    // Privacy tools
    MakeLabel(hWnd, L"  Privacy Tools", 480, 392, 150, 18, g_hFontBold);
    g_btnKSOn   = MakeBtn(hWnd, L"Kill Switch ON",   480, 412, 130, 28, ID_BTN_KS_ON);
    g_btnKSOff  = MakeBtn(hWnd, L"Kill Switch OFF",  618, 412, 140, 28, ID_BTN_KS_OFF);
    g_btnLeak   = MakeBtn(hWnd, L"DNS Leak Test",    480, 446, 130, 28, ID_BTN_LEAK);
    g_btnHarden = MakeBtn(hWnd, L"Full Harden",      618, 446, 140, 28, ID_BTN_HARDEN);
    g_btnReboot = MakeBtn(hWnd, L"Restart PC",       766, 412, 110, 28, ID_BTN_REBOOT);

    // Traffic graph (sparkline)
    MakeLabel(hWnd, L"  Traffic speed (last ~10 min)", 16, 478, 320, 18, g_hFontBold);
    CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        16, 498, 920, 86, hWnd, (HMENU)ID_SPARKLINE, g_hInst, 0);

    // Footer
    wchar_t footer[256];
    wsprintfW(footer, L"  UDP 51820 | 10.0.0.0/24 | v2.3.0 | PID %d", GetCurrentProcessId());
    MakeLabel(hWnd, footer, 16, 588, 600, 18, g_hFontSmall);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hWnd = hWnd;
        GetModuleFileNameW(NULL, g_appDir, MAX_PATH);
        wchar_t* slash = wcsrchr(g_appDir, L'\\');
        if (slash) *slash = 0;
        AddControls(hWnd);
        g_startTick = GetTickCount();
        SetTimer(hWnd, ID_TIMER, 5000, 0);

        // Tray icon
        g_nid.cbSize = sizeof(g_nid);
        g_nid.hWnd = hWnd;
        g_nid.uID = 1;
        g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        g_nid.uCallbackMessage = WM_TRAYICON;
        g_nid.hIcon = MakeTrayIcon();
        wcscpy_s(g_nid.szTip, L"VPN-TEIVRIM");
        Shell_NotifyIconW(NIM_ADD, &g_nid);

        WriteLog("=== GUI v2.2.0 started ===");
        DoRefresh();
        return 0;
    }

    case WM_TIMER:
        if (wParam == ID_TIMER) DoRefresh();
        return 0;

    case WM_TRAYICON:
        if (lParam == WM_LBUTTONDBLCLK || lParam == WM_LBUTTONUP) {
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
        } else if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            HMENU hMenu = CreatePopupMenu();
            AppendMenuW(hMenu, MF_STRING, 1, L"Показать");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, 2, L"Выход");
            SetForegroundWindow(hWnd);
            int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
            if (cmd == 1) { ShowWindow(hWnd, SW_RESTORE); SetForegroundWindow(hWnd); }
            else if (cmd == 2) { PostQuitMessage(0); }
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            ShowWindow(hWnd, SW_HIDE);
            return 0;
        }
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_USER + 100:
        DoRefresh();
        return 0;

    case WM_USER + 101:
        UpdateIpLabel();
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BTN_REFRESH: DoRefresh(); break;
        case ID_BTN_START:   RunAsync(1); break;
        case ID_BTN_STOP:    RunAsync(2); break;
        case ID_BTN_LVL1:    RunAsync(3); break;
        case ID_BTN_LVL2:    RunAsync(4); break;
        case ID_BTN_LVL3:    RunAsync(5); break;
        case ID_BTN_KS_ON:   RunAsync(6); break;
        case ID_BTN_KS_OFF:  RunAsync(7); break;
        case ID_BTN_HARDEN:  RunAsync(8); break;
        case ID_BTN_LEAK:
            ShellExecuteW(NULL, L"open", L"powershell.exe",
                L"-ExecutionPolicy Bypass -NoProfile -Command \"& 'D:\\SOOBSHESTVA\\VPN\\VPN-TEIVRIM\\vpn-leaktest.ps1'\"",
                NULL, SW_SHOW);
            break;
        case ID_BTN_ADD:
            RunAsync(9);
            break;
        case ID_BTN_REMOVE: {
            int sel = (int)SendMessage(g_listPeer, LVM_GETNEXTITEM, (WPARAM)-1, (LPARAM)LVNI_SELECTED);
            if (sel >= 0 && sel < (int)g_peerKeys.size()) {
                g_removeKey = g_peerKeys[sel];
                RunAsync(10);
            } else {
                MessageBoxW(hWnd, L"Выбери клиента в списке для удаления", L"Remove", MB_ICONINFORMATION);
            }
            break;
        }
        case ID_BTN_LOG:
            ShellExecuteW(NULL, L"open", L"notepad.exe",
                (L"\"" + std::wstring(g_appDir) + L"\\vpn-gui.log\"").c_str(), NULL, SW_SHOW);
            break;
        case ID_BTN_EXPORT: {
            wchar_t dst[MAX_PATH];
            wsprintfW(dst, L"%s\\client-export.conf", g_appDir);
            CopyFileW(L"C:\\WireGuard\\client0.conf", dst, FALSE);
            ShellExecuteW(NULL, L"open", L"explorer.exe", g_appDir, NULL, SW_SHOW);
            break;
        }
        case ID_BTN_QR:
            ShowQRDialog(hWnd, L"C:\\WireGuard\\client0.conf");
            break;
        case ID_BTN_COPY:
            CopyTextToClipboard(hWnd, ReadFileText(L"C:\\WireGuard\\client0.conf").c_str());
            MessageBoxW(hWnd, L"Конфиг client0 скопирован в буфер", L"Copy", MB_ICONINFORMATION);
            break;
        case ID_BTN_REBOOT:
            if (MessageBoxW(hWnd, L"Restart PC?", L"Confirm", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                WriteLog("Rebooting...");
                ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_APPLICATION);
            }
            break;
        }
        return 0;

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT di = (LPDRAWITEMSTRUCT)lParam;
        if (di->CtlID == ID_SPARKLINE) {
            DrawSparkline(di->hwndItem, di->hDC, di->rcItem);
        }
        return 0;
    }

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
            else SetTextColor(hdc, RGB(255, 200, 0));
        } else {
            SetTextColor(hdc, RGB(150, 150, 150));
        }
        return (LRESULT)g_hBrushBg;
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        HWND c = (HWND)lParam;
        if (c == g_listLog) {
            SetTextColor(hdc, RGB(0, 210, 90));
            SetBkColor(hdc, RGB(12, 12, 16));
            static HBRUSH br = CreateSolidBrush(RGB(12, 12, 16));
            return (LRESULT)br;
        }
        SetTextColor(hdc, RGB(190, 190, 190));
        SetBkColor(hdc, RGB(22, 22, 28));
        static HBRUSH br2 = CreateSolidBrush(RGB(22, 22, 28));
        return (LRESULT)br2;
    }

    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(220, 220, 220));
        static HBRUSH br = CreateSolidBrush(RGB(40, 40, 48));
        return (LRESULT)br;
    }

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, g_hBrushBg);
        return 1;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 920;
        mmi->ptMinTrackSize.y = 604;
        mmi->ptMaxTrackSize.x = 920;
        mmi->ptMaxTrackSize.y = 604;
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hWnd, ID_TIMER);
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        if (g_nid.hIcon) DestroyIcon(g_nid.hIcon);
        DeleteObject(g_hBrushBg);
        DeleteObject(g_hFont);
        DeleteObject(g_hFontBold);
        DeleteObject(g_hFontMono);
        DeleteObject(g_hFontSmall);
        WriteLog("=== GUI closed ===");
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nShow) {
    g_hInst = hInst;
    SetUnhandledExceptionFilter(CrashFilter);

    // Single instance check - silently focus existing window instead of flashing a box.
    // Only exit if a real window is alive; a stale/zombie mutex must never cause an instant-exit loop.
    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"VPN-TEIVRIM-Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND hExisting = FindWindowW(CLASS_NAME, NULL);
        if (hExisting) {
            ShowWindow(hExisting, SW_SHOW);
            SetForegroundWindow(hExisting);
            return 0;
        }
        CloseHandle(hMutex);
        hMutex = CreateMutexW(NULL, TRUE, L"VPN-TEIVRIM-Mutex");
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(18, 18, 24));
    RegisterClassW(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    g_hWnd = CreateWindowExW(0, CLASS_NAME, L"VPN-TEIVRIM v2.3.0",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (sw - 952) / 2, (sh - 604) / 2, 952, 604,
        0, 0, hInst, 0);

    ShowWindow(g_hWnd, nShow);
    UpdateWindow(g_hWnd);

    MSG m;
    while (GetMessageW(&m, 0, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}
