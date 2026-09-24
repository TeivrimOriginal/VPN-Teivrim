#ifndef UNICODE
#define UNICODE
#endif
#define _WIN32_WINNT 0x0600
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "privacy.h"
#include "exec.h"
#include "config.h"
#include "peers.h"
#include "killswitch.h"

static bool SetRegDword(HKEY root, const wchar_t* sub, const wchar_t* name, DWORD val) {
    HKEY h = 0;
    LONG r = RegCreateKeyExW(root, sub, 0, NULL, 0, KEY_WRITE, NULL, &h, NULL);
    if (r != ERROR_SUCCESS) return false;
    r = RegSetValueExW(h, name, 0, REG_DWORD, (BYTE*)&val, sizeof(val));
    RegCloseKey(h);
    return r == ERROR_SUCCESS;
}

static bool DisableIPv6() {
    WriteLog("Privacy: disable IPv6");
    // Binding via netsh is more reliable than Get-NetAdapterBinding; use registry global
    SetRegDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip6\\Parameters", L"DisabledComponents", 0xFF);
    // Also try netsh for current adapters (best effort)
    ExecCmd(L"cmd.exe /c for /f \"tokens=3\" %i in ('netsh interface ipv6 show interfaces ^| findstr /r \"[0-9][0-9]*\"') do netsh interface ipv6 set interface %i admin=disable >nul 2>&1", 3000);
    return true;
}

static bool SetDnsCloudflare() {
    WriteLog("Privacy: set DNS 1.1.1.1");
    // Enumerate adapters via IP helper and set DNS via netsh
    PIP_ADAPTER_ADDRESSES addrs = NULL;
    ULONG sz = 0;
    GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &sz);
    if (sz == 0) sz = 16384;
    addrs = (PIP_ADAPTER_ADDRESSES)HeapAlloc(GetProcessHeap(), 0, sz);
    if (!addrs) return false;
    if (GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addrs, &sz) != NO_ERROR) {
        HeapFree(GetProcessHeap(), 0, addrs);
        // fallback: try common names
        ExecCmd(L"cmd.exe /c netsh interface ip set dns \"Ethernet\" static 1.1.1.1 >nul 2>&1", 2000);
        ExecCmd(L"cmd.exe /c netsh interface ip add dns \"Ethernet\" 1.0.0.1 index=2 >nul 2>&1", 2000);
        ExecCmd(L"cmd.exe /c netsh interface ip set dns \"Wi-Fi\" static 1.1.1.1 >nul 2>&1", 2000);
        ExecCmd(L"cmd.exe /c netsh interface ip add dns \"Wi-Fi\" 1.0.0.1 index=2 >nul 2>&1", 2000);
        return true;
    }
    for (PIP_ADAPTER_ADDRESSES a = addrs; a; a = a->Next) {
        if (a->OperStatus != IfOperStatusUp) continue;
        if (wcsstr(a->Description, L"WireGuard") || wcsstr(a->FriendlyName, L"WireGuard")) continue;
        if (a->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
        wchar_t cmd1[256], cmd2[256];
        wsprintfW(cmd1, L"cmd.exe /c netsh interface ip set dns \"%s\" static 1.1.1.1 >nul 2>&1", a->FriendlyName);
        wsprintfW(cmd2, L"cmd.exe /c netsh interface ip add dns \"%s\" 1.0.0.1 index=2 >nul 2>&1", a->FriendlyName);
        ExecCmd(cmd1, 2000);
        ExecCmd(cmd2, 2000);
    }
    HeapFree(GetProcessHeap(), 0, addrs);
    return true;
}

static bool DisableNetBios() {
    WriteLog("Privacy: disable NetBIOS");
    HKEY hRoot;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces", 0, KEY_READ, &hRoot) != ERROR_SUCCESS) return false;
    wchar_t sub[512];
    DWORD idx = 0;
    while (true) {
        DWORD len = 512;
        LONG r = RegEnumKeyExW(hRoot, idx, sub, &len, NULL, NULL, NULL, NULL);
        if (r != ERROR_SUCCESS) break;
        wchar_t full[1024];
        wsprintfW(full, L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters\\Interfaces\\%s", sub);
        SetRegDword(HKEY_LOCAL_MACHINE, full, L"NetbiosOptions", 2);
        idx++;
    }
    RegCloseKey(hRoot);
    return true;
}

static bool DisableLLMNR() {
    WriteLog("Privacy: disable LLMNR");
    return SetRegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows NT\\DNSClient", L"EnableMulticast", 0);
}

static bool DisableMDNS() {
    WriteLog("Privacy: disable mDNS");
    return SetRegDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Dnscache\\Parameters", L"EnableMDNS", 0);
}

static bool DisableWPAD() {
    WriteLog("Privacy: disable WPAD");
    return SetRegDword(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\Wpad", L"WpadOverride", 1);
}

static bool DisableTelemetry() {
    WriteLog("Privacy: disable telemetry");
    SetRegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"AllowTelemetry", 0);
    SetRegDword(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Policies\\Microsoft\\Windows\\DataCollection", L"MaxTelemetryAllowed", 0);
    ExecCmd(L"cmd.exe /c sc stop DiagTrack >nul 2>&1", 2000);
    ExecCmd(L"cmd.exe /c sc config DiagTrack start= disabled >nul 2>&1", 2000);
    return true;
}

bool EnableIpForwarding() {
    WriteLog("Privacy: enable IP forwarding");
    SetRegDword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", L"IPEnableRouter", 1);
    ExecCmd(L"cmd.exe /c netsh interface ipv4 set global forwarding=enabled >nul 2>&1", 2000);
    return true;
}

bool IsWireGuardInstalled() {
    return GetFileAttributesW(L"C:\\Program Files\\WireGuard\\wg.exe") != INVALID_FILE_ATTRIBUTES;
}

bool IsServerConfigured() {
    return GetFileAttributesW(L"C:\\WireGuard\\wg0.conf") != INVALID_FILE_ATTRIBUTES;
}

bool ApplyAnonymityLevel(int level) {
    if (level < 1) level = 1;
    if (level > 3) level = 3;
    WriteLogW((L"ApplyAnonymityLevel " + std::to_wstring(level)).c_str());
    if (level >= 1) {
        // Ensure server running
        std::wstring show = ExecWG(L"show wg0");
        if (show.find(L"listening port") == std::wstring::npos) {
            WriteLog("Level1: starting WG");
            ExecCmd(L"\"C:\\Program Files\\WireGuard\\wireguard.exe\" /installtunnelservice C:\\WireGuard\\wg0.conf", 3000);
            Sleep(2000);
        }
        WriteLog("Level1 done");
    }
    if (level >= 2) {
        DisableIPv6();
        SetDnsCloudflare();
        DisableNetBios();
        DisableLLMNR();
        DisableMDNS();
        WriteLog("Level2 done");
    }
    if (level >= 3) {
        EnableKillSwitch();
        DisableWPAD();
        DisableTelemetry();
        EnableIpForwarding();
        WriteLog("Level3 done");
    }
    WriteLogW((L"Anonymity Level " + std::to_wstring(level) + L" applied").c_str());
    return true;
}

bool ApplyHarden() {
    WriteLog("ApplyHarden 8/8");
    // Full hardening = level 3 + explicit 8 steps (same as vpn-harden.ps1)
    DisableIPv6();
    SetDnsCloudflare();
    DisableNetBios();
    DisableLLMNR();
    DisableMDNS();
    DisableWPAD();
    EnableKillSwitch();
    DisableTelemetry();
    EnableIpForwarding();
    WriteLog("Harden complete");
    return true;
}

bool SetupPrivacyServer() {
    if (!IsWireGuardInstalled()) {
        WriteLog("SetupPrivacyServer: WireGuard not found");
        return false;
    }
    if (IsServerConfigured()) {
        WriteLog("SetupPrivacyServer: already configured");
        return true;
    }
    WriteLog("SetupPrivacyServer: creating wg0.conf");
    // Ensure dir
    CreateDirectoryW(L"C:\\WireGuard", NULL);

    // Generate keys via wg.exe
    std::wstring serverPriv = TrimWS(ExecCmd(L"\"C:\\Program Files\\WireGuard\\wg.exe\" genkey", 3000));
    if (serverPriv.empty()) { WriteLog("genkey failed"); return false; }
    std::wstring serverPub = TrimWS(ExecCmd((std::wstring(L"cmd.exe /c echo ") + serverPriv + L" | \"C:\\Program Files\\WireGuard\\wg.exe\" pubkey").c_str(), 3000));
    std::wstring clientPriv = TrimWS(ExecCmd(L"\"C:\\Program Files\\WireGuard\\wg.exe\" genkey", 3000));
    std::wstring clientPub = TrimWS(ExecCmd((std::wstring(L"cmd.exe /c echo ") + clientPriv + L" | \"C:\\Program Files\\WireGuard\\wg.exe\" pubkey").c_str(), 3000));
    std::wstring psk = TrimWS(ExecCmd(L"\"C:\\Program Files\\WireGuard\\wg.exe\" genpsk", 3000));
    if (serverPub.empty() || clientPriv.empty() || clientPub.empty()) { WriteLog("keygen failed"); return false; }

    // Detect WAN IP for client config (fallback)
    std::wstring wan = TrimWS(ExecCmd(L"curl.exe -s --max-time 4 https://api.ipify.org", 4000));
    if (wan.empty() || wan.find(L".") == std::wstring::npos) wan = L"YOUR_SERVER_IP";

    std::wstring serverConf = L"[Interface]\nPrivateKey = " + serverPriv + L"\nAddress = 10.0.0.1/24\nListenPort = 51820\nMTU = 1420\n\n"
        L"# NAT\nPostUp = netsh advfirewall firewall add rule name=\"VPN-Tunnel-In\" dir=in action=allow protocol=udp localport=51820\n"
        L"PostUp = netsh advfirewall firewall add rule name=\"VPN-Tunnel-Allow\" dir=in action=allow remoteip=10.0.0.0/24\n"
        L"PostDown = netsh advfirewall firewall delete rule name=\"VPN-Tunnel-In\"\n"
        L"PostDown = netsh advfirewall firewall delete rule name=\"VPN-Tunnel-Allow\"\n\n"
        L"[Peer]\nPublicKey = " + clientPub + L"\nPresharedKey = " + psk + L"\nAllowedIPs = 10.0.0.2/32\n";
    WriteFileText(L"C:\\WireGuard\\wg0.conf", serverConf);

    std::wstring clientConf = L"[Interface]\nPrivateKey = " + clientPriv + L"\nAddress = 10.0.0.2/24\nDNS = 1.1.1.1, 1.0.0.1\nMTU = 1420\n\n"
        L"[Peer]\nPublicKey = " + serverPub + L"\nPresharedKey = " + psk + L"\nEndpoint = " + wan + L":51820\nAllowedIPs = 0.0.0.0/0, ::/0\nPersistentKeepalive = 25\n";
    WriteFileText(L"C:\\WireGuard\\client0.conf", clientConf);

    EnableIpForwarding();
    ExecCmd(L"\"C:\\Program Files\\WireGuard\\wireguard.exe\" /installtunnelservice C:\\WireGuard\\wg0.conf", 3000);
    Sleep(2000);
    std::wstring show = ExecWG(L"show wg0");
    bool ok = show.find(L"listening port") != std::wstring::npos;
    WriteLog(ok ? "SetupPrivacyServer: running" : "SetupPrivacyServer: failed");
    return ok;
}
