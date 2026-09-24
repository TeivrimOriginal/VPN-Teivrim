#pragma once
#include <windows.h>
#include <string>

extern wchar_t g_appDir[MAX_PATH];

std::wstring ReadFileText(const wchar_t* path);
void WriteFileText(const wchar_t* path, const std::wstring& content);
std::wstring ReadServerConf();
void WriteLog(const char* msg);
void WriteLogW(const wchar_t* msg);
std::wstring ReadLog(int maxLines);

// DPAPI helpers (P0-4)
bool ProtectFileDPAPI(const wchar_t* path);
bool UnprotectFileDPAPI(const wchar_t* path);
bool IsFileProtected(const wchar_t* path);
