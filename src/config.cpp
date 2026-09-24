#ifndef UNICODE
#define UNICODE
#endif
#include "config.h"
#include <cstdio>
#include <vector>
#include <wincrypt.h>

// g_appDir defined in vpn-gui.cpp

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
    if (n <= 1) return;
    std::string utf8(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, &utf8[0], n - 1, NULL, NULL);
    FILE* f = NULL;
    _wfopen_s(&f, path, L"wb");
    if (f) { fwrite(utf8.data(), 1, utf8.size(), f); fclose(f); }
}

std::wstring ReadServerConf() {
    return ReadFileText(L"C:\\WireGuard\\wg0.conf");
}

std::wstring ReadLog(int maxLines) {
    wchar_t path[MAX_PATH];
    wsprintfW(path, L"%s\\vpn-gui.log", g_appDir);
    FILE* f = NULL;
    _wfopen_s(&f, path, L"rb");
    if (!f) _wfopen_s(&f, L"C:\\WireGuard\\vpn-monitor.log", L"rb");
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
    for (int i = start; i < (int)lines.size(); i++) out += lines[i] + L"\n";
    return out;
}

// --- DPAPI: encrypt/decrypt file in-place (simple envelope, no header versioning needed for v2.4) ---
// For v2.4 we protect wg0.conf/client*.conf with CryptProtectData (CurrentUser scope, no entropy).
// The file is stored as binary DPAPI blob if protected; ReadFileText auto-detects via IsFileProtected.

bool ProtectFileDPAPI(const wchar_t* path) {
    if (IsFileProtected(path)) return true;
    std::wstring txt = ReadFileText(path);
    if (txt.empty()) return false;
    int n = WideCharToMultiByte(CP_UTF8, 0, txt.c_str(), -1, 0, 0, NULL, NULL);
    if (n <= 1) return false;
    std::string utf8(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, txt.c_str(), -1, &utf8[0], n - 1, NULL, NULL);

    DATA_BLOB in = { (DWORD)utf8.size(), (BYTE*)utf8.data() };
    DATA_BLOB out = {};
    if (!CryptProtectData(&in, L"VPN-TEIVRIM", NULL, NULL, NULL, 0, &out)) return false;
    FILE* f = NULL;
    _wfopen_s(&f, path, L"wb");
    if (!f) { LocalFree(out.pbData); return false; }
    fwrite(out.pbData, 1, out.cbData, f);
    fclose(f);
    LocalFree(out.pbData);
    WriteLog("DPAPI protected file");
    return true;
}

bool UnprotectFileDPAPI(const wchar_t* path) {
    if (!IsFileProtected(path)) return true;
    FILE* f = NULL;
    _wfopen_s(&f, path, L"rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz <= 0) { fclose(f); return false; }
    fseek(f, 0, SEEK_SET);
    BYTE* data = (BYTE*)HeapAlloc(GetProcessHeap(), 0, sz);
    if (!data) { fclose(f); return false; }
    fread(data, 1, sz, f);
    fclose(f);

    DATA_BLOB in = { (DWORD)sz, data };
    DATA_BLOB out = {};
    BOOL ok = CryptUnprotectData(&in, NULL, NULL, NULL, NULL, 0, &out);
    HeapFree(GetProcessHeap(), 0, data);
    if (!ok) return false;
    // out.pbData is utf8 bytes
    std::string utf8((char*)out.pbData, out.cbData);
    LocalFree(out.pbData);
    int wl = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), 0, 0);
    wchar_t* wd = (wchar_t*)HeapAlloc(GetProcessHeap(), 0, (wl + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), wd, wl);
    wd[wl] = 0;
    std::wstring wtxt(wd, wl);
    HeapFree(GetProcessHeap(), 0, wd);
    WriteFileText(path, wtxt);
    WriteLog("DPAPI unprotected file");
    return true;
}

bool IsFileProtected(const wchar_t* path) {
    FILE* f = NULL;
    _wfopen_s(&f, path, L"rb");
    if (!f) return false;
    unsigned char head[16] = {0};
    size_t r = fread(head, 1, 16, f);
    fclose(f);
    if (r < 4) return false;
    // DPAPI blob starts with version/ASN. Our protected files are binary, not UTF-8 text starting with '['
    // Heuristic: if file starts with '[' (wg conf) or contains readable text, it's not protected
    if (head[0] == '[') return false;
    // DPAPI blob is not valid UTF-8 text with '[Interface]' header
    std::wstring txt = ReadFileText(path);
    if (txt.find(L"[Interface]") != std::wstring::npos) return false;
    return true;
}
