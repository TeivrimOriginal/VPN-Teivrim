#pragma once
#include <windows.h>

// P1: перенос vpn-anonymity.ps1 / vpn-harden.ps1 в C++ (без ExecutionPolicy)
bool ApplyAnonymityLevel(int level); // 1,2,3
bool ApplyHarden();                  // полный харденинг 8/8 (level3 + extras)
bool SetupPrivacyServer();           // создание wg0.conf/client0.conf если отсутствует

// helpers exposed for wizard
bool IsWireGuardInstalled();
bool IsServerConfigured();
bool EnableIpForwarding();
