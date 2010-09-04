#include "Common.h"
#include <psapi.h>
#include <fstream>
#include <sstream>

#pragma comment(lib, "Psapi.lib")

DWORD GetHandleByProcessName(const std::string &name)
{
	DWORD result = 0;
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return 0;

	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.

	for ( i = 0; i < cProcesses; ++i ){
		if( aProcesses[i] == 0 )
			continue;
		DWORD processID = aProcesses[i];
		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
			PROCESS_VM_READ,
			FALSE, processID );
		TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

		// Get the process name.

		if (NULL != hProcess )
		{
			HMODULE hMod;
			DWORD cbNeeded;

			if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded) )
			{
				GetModuleBaseName( hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR) );
			}
			if (strcmp(ToLower(szProcessName).c_str(), ToLower(name).c_str()) == 0){
				result = processID;
				break;
			}

		}
		CloseHandle( hProcess );


	}

	return result;
}

std::string ToLower(const std::string &str)
{
	std::string output;
	int len = str.length();
	for(int i = 0; i < len; ++i){
		output += (char)tolower(str[i]);
	}
	return output;
}

void log(std::string message)
{
	std::fstream file("log.log", std::ios::out|std::ios::app);
	file << message;
	file << "\n";
	file.close();
}

HMODULE InjectDll(HANDLE hProcess, const char *DllName)
{
	size_t DllNameLength = strlen(DllName) + 1;
	PVOID mem = VirtualAllocEx(hProcess, NULL, DllNameLength, MEM_COMMIT, PAGE_READWRITE);

	if (mem == NULL)
	{
		log("can't allocate memory in that pid");
		CloseHandle(hProcess);
		return 0;
	}

	if (WriteProcessMemory(hProcess, mem, (void*)DllName, DllNameLength, NULL) == 0)
	{
		log("can't write to memory in that pid\n");
		VirtualFreeEx(hProcess, mem, DllNameLength, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandle("KERNEL32.DLL"),"LoadLibraryA"), mem, 0, NULL);
	if (hThread == INVALID_HANDLE_VALUE)
	{
		log("can't create a thread in that pid\n");
		VirtualFreeEx(hProcess, mem, DllNameLength, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}

	WaitForSingleObject(hThread, INFINITE);

	HMODULE hLibrary = NULL;
	if (!GetExitCodeThread(hThread, (LPDWORD)&hLibrary))
	{
		std::stringstream ss;
		ss << "can't get exit code for thread GetLastError() = ";
		ss << GetLastError();
		log (ss.str());
		CloseHandle(hThread);
		VirtualFreeEx(hProcess, mem, DllNameLength, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}

	CloseHandle(hThread);
	VirtualFreeEx(hProcess, mem, DllNameLength, MEM_RELEASE);

	if (hLibrary == NULL)
	{
		hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandle("KERNEL32.DLL"),"GetLastError"), 0, 0, NULL);
		if (hThread == INVALID_HANDLE_VALUE)
		{
			log ("LoadLibraryA returned NULL and can't get last error.");
			CloseHandle(hProcess);
			return 0;
		}

		WaitForSingleObject(hThread, INFINITE);
		DWORD error;
		GetExitCodeThread(hThread, &error);

		CloseHandle(hThread);

		std::stringstream ss;
		ss << "LoadLibrary return NULL, GetLastError() is ";
		ss << error;
		log (ss.str());
		CloseHandle(hProcess);
		return false;
	}


	return hLibrary;
}

bool LoadRemoteFunction(HANDLE hProcess, HMODULE hLibrary, const char *DllName, const char *functionName, void *mem)
{
	HINSTANCE hLocalLibrary = LoadLibraryA(DllName);
	if (!hLocalLibrary) 
		return false;

	FARPROC pFunction = GetProcAddress(hLocalLibrary, functionName);
	if (!pFunction) 
		return false;

	DWORD loc = ((DWORD)pFunction - (DWORD)hLocalLibrary);

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((DWORD)hLibrary + loc), mem, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return true;
}

bool bDataCompare( const BYTE* pData, const BYTE* bMask, const char* szMask )
{
    for( ; *szMask; ++szMask, ++pData, ++bMask )
	{
        if( *szMask == 'x' && *pData != *bMask ) 
		{
            return false;
		}
	}
    return ( *szMask ) == NULL;
}

DWORD FindPattern( DWORD dwAddress, DWORD dwLen, BYTE *bMask, char* szMask )
{
    for( DWORD i=0; i < dwLen; i++ )
	{
		if( bDataCompare( ( BYTE* )( dwAddress + i ), bMask, szMask) )
		{
			return ( DWORD )( dwAddress + i );
		}
	}
    return 0;
}

DWORD FindPattern(BYTE *bMask, char* szMask )
{
	return FindPattern((DWORD)GetModuleHandle( 0 ), 0xFFFFFFFF, bMask, szMask);
}
