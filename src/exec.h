#pragma once
#include <windows.h>
#include <string>

std::wstring ExecCmd(const wchar_t* cmdline, DWORD timeoutMs = 5000);
std::wstring ExecWG(const wchar_t* args);
std::wstring GetLine(const std::wstring& s, const wchar_t* key);
std::wstring TrimWS(const std::wstring& s);
double ParseSize(const std::wstring& s);
std::wstring FormatSize(double bytes);
std::wstring FormatSpeed(double bytesPerSec);
