#ifndef UNICODE
#define UNICODE
#endif
#include "exec.h"
#include <cstdio>

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

std::wstring ExecCmd(const wchar_t* cmdline, DWORD timeoutMs) {
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

std::wstring TrimWS(const std::wstring& s) {
    size_t a = s.find_first_not_of(L" \t\r\n");
    if (a == std::wstring::npos) return L"";
    size_t b = s.find_last_not_of(L" \t\r\n");
    return s.substr(a, b - a + 1);
}

double ParseSize(const std::wstring& s) {
    if (s.empty()) return 0;
    size_t numEnd = s.find_first_not_of(L"0123456789.,");
    if (numEnd == std::wstring::npos) numEnd = s.size();
    std::wstring numStr = s.substr(0, numEnd);
    double val = 0;
    swscanf_s(numStr.c_str(), L"%lf", &val);
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

std::wstring FormatSize(double bytes) {
    wchar_t buf[64];
    if (bytes < 1024.0) wsprintfW(buf, L"%.0f B", bytes);
    else if (bytes < 1024.0 * 1024.0) wsprintfW(buf, L"%.1f KiB", bytes / 1024.0);
    else if (bytes < 1024.0 * 1024.0 * 1024.0) wsprintfW(buf, L"%.2f MiB", bytes / (1024.0 * 1024.0));
    else if (bytes < 1024.0 * 1024.0 * 1024.0 * 1024.0) wsprintfW(buf, L"%.2f GiB", bytes / (1024.0 * 1024.0 * 1024.0));
    else wsprintfW(buf, L"%.2f TiB", bytes / (1024.0 * 1024.0 * 1024.0 * 1024.0));
    return std::wstring(buf);
}

std::wstring FormatSpeed(double bytesPerSec) {
    if (bytesPerSec < 1.0) return L"0 B/s";
    return FormatSize(bytesPerSec) + L"/s";
}
