#pragma once

#include <Windows.h>
#include <string>

DWORD GetHandleByProcessName(const std::string &name);
HMODULE InjectDll(HANDLE hProcess, const char *DllName);
std::string ToLower(const std::string &str);
bool LoadRemoteFunction(HANDLE hProcess, HMODULE hLibrary, const char *DllName, const char *functionName, void *mem);
void log(std::string message);
DWORD FindPattern( DWORD dwAddress, DWORD dwLen, BYTE *bMask, char* szMask );
DWORD FindPattern(BYTE *bMask, char* szMask );