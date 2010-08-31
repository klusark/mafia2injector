#pragma once

#include <Windows.h>
#include <string>

DWORD GetHandleByProcessName(const std::string &name);
HMODULE InjectDll(HANDLE hProcess, const char *DllName);

bool LoadRemoteFunction(HANDLE hProcess, HMODULE hLibrary, const char *DllName, const char *functionName, void *mem);