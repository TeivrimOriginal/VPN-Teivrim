#ifndef UNICODE
#define UNICODE
#endif
#include "killswitch.h"
#include "exec.h"
#include "config.h"

static const wchar_t* KS_MARKER = L"C:\\WireGuard\\.ks_enabled";

bool IsKillSwitchEnabled() {
    // Check marker file OR firewall rule existence
    FILE* f = NULL;
    _wfopen_s(&f, KS_MARKER, L"rb");
    if (f) { fclose(f); return true; }
    std::wstring out = ExecCmd(L"cmd.exe /c netsh advfirewall firewall show rule name=\"KS-Block-All\" 2>nul", 2000);
    return out.find(L"KS-Block-All") != std::wstring::npos;
}

static bool CreateScheduledTask() {
    // schtasks creates a task that reapplies KS on logon if marker exists
    // Use schtasks /create /tn VPN-TEIVRIM-KS /tr "netsh ..." /sc onlogon /ru SYSTEM /f
    // For v2.4 minimal, we just ensure marker + firewall rules are persistent via netsh (already persistent)
    // and also add Run key for fallback
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        const wchar_t* val = L"cmd.exe /c netsh advfirewall firewall show rule name=\"KS-Block-All\" >nul 2>&1 || netsh advfirewall firewall add rule name=\"KS-Block-All\" dir=out action=block >nul 2>&1";
        // Only create if KS enabled, otherwise remove
        if (IsKillSwitchEnabled()) {
            RegSetValueExW(hKey, L"VPN-TEIVRIM-KS", 0, REG_SZ, (BYTE*)val, (DWORD)((wcslen(val)+1)*sizeof(wchar_t)));
        } else {
            RegDeleteValueW(hKey, L"VPN-TEIVRIM-KS");
        }
        RegCloseKey(hKey);
    }
    return true;
}

bool EnableKillSwitch() {
    WriteLog("Kill Switch v2 enable");
    // Create marker first so Restore can work after reboot
    FILE* f = NULL;
    _wfopen_s(&f, KS_MARKER, L"wb");
    if (f) { fputs("1", f); fclose(f); }

    ExecCmd(L"cmd.exe /c netsh advfirewall firewall add rule name=\"KS-Allow-WG\" dir=out action=allow protocol=udp remoteport=51820", 3000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall add rule name=\"KS-Allow-WG-In\" dir=in action=allow protocol=udp localport=51820", 3000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall add rule name=\"KS-Allow-Loop\" dir=out action=allow remoteip=127.0.0.1", 3000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall add rule name=\"KS-Allow-Loop-In\" dir=in action=allow remoteip=127.0.0.1", 3000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall add rule name=\"KS-Allow-LAN\" dir=out action=allow remoteip=192.168.0.0/16,10.0.0.0/8,172.16.0.0/12", 3000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall add rule name=\"KS-Allow-LAN-In\" dir=in action=allow remoteip=192.168.0.0/16,10.0.0.0/8,172.16.0.0/12", 3000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall add rule name=\"KS-Allow-Tunnel\" dir=out action=allow remoteip=10.0.0.0/24", 3000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall add rule name=\"KS-Allow-Tunnel-In\" dir=in action=allow remoteip=10.0.0.0/24", 3000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall add rule name=\"KS-Block-All\" dir=out action=block", 3000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall add rule name=\"KS-Block-All-In\" dir=in action=block", 3000);

    // Persistent: netsh rules are already persistent; also add Run key
    CreateScheduledTask();
    WriteLog("Kill Switch v2 enabled");
    return true;
}

bool DisableKillSwitch() {
    WriteLog("Kill Switch v2 disable");
    // Remove marker
    DeleteFileW(KS_MARKER);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall delete rule name=\"KS-Allow-WG\"", 2000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall delete rule name=\"KS-Allow-WG-In\"", 2000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall delete rule name=\"KS-Allow-Loop\"", 2000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall delete rule name=\"KS-Allow-Loop-In\"", 2000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall delete rule name=\"KS-Allow-LAN\"", 2000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall delete rule name=\"KS-Allow-LAN-In\"", 2000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall delete rule name=\"KS-Allow-Tunnel\"", 2000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall delete rule name=\"KS-Allow-Tunnel-In\"", 2000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall delete rule name=\"KS-Block-All\"", 2000);
    ExecCmd(L"cmd.exe /c netsh advfirewall firewall delete rule name=\"KS-Block-All-In\"", 2000);

    CreateScheduledTask();
    WriteLog("Kill Switch v2 disabled");
    return true;
}

bool RestoreKillSwitchIfNeeded() {
    FILE* f = NULL;
    _wfopen_s(&f, KS_MARKER, L"rb");
    if (!f) return false;
    fclose(f);
    // Marker exists but rule might be missing after network reset -> re-enable
    std::wstring chk = ExecCmd(L"cmd.exe /c netsh advfirewall firewall show rule name=\"KS-Block-All\" 2>nul", 2000);
    if (chk.find(L"KS-Block-All") == std::wstring::npos) {
        WriteLog("Restoring Kill Switch after reboot");
        EnableKillSwitch();
        return true;
    }
    return false;
}
