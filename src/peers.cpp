#ifndef UNICODE
#define UNICODE
#endif
#include "peers.h"
#include "config.h"
#include "exec.h"

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
    if (cfg.empty()) { WriteLog("wg0.conf пустой — добавление отменено"); return; }
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
    if (cfg.empty()) { WriteLog("wg0.conf пустой — удаление отменено"); return; }
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
