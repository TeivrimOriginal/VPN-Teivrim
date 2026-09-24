#pragma once
#include <windows.h>
#include <string>

std::wstring GetServerPubKey();
int CountPeers();
void AppendPeer(const std::wstring& pub, const std::wstring& ip);
void WriteClientConf(int idx, const std::wstring& priv, const std::wstring& serverPub,
                     const std::wstring& wan, const std::wstring& ip);
void RemovePeerByKey(const std::wstring& pub);
