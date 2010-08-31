#include "Common.h"
#include <psapi.h>

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
			if (strcmp(szProcessName, name.c_str()) == 0){
				result = processID;
				break;
			}

		}
		CloseHandle( hProcess );


	}

	return result;
}

HMODULE InjectDll(HANDLE hProcess, const char *DllName)
{
	PVOID mem = VirtualAllocEx(hProcess, NULL, strlen(DllName) + 1, MEM_COMMIT, PAGE_READWRITE);

	if (mem == NULL)
	{
		fprintf(stderr, "can't allocate memory in that pid\n");
		CloseHandle(hProcess);
		return 0;
	}

	if (WriteProcessMemory(hProcess, mem, (void*)DllName, strlen(DllName) + 1, NULL) == 0)
	{
		fprintf(stderr, "can't write to memory in that pid\n");
		VirtualFreeEx(hProcess, mem, strlen(DllName) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandle("KERNEL32.DLL"),"LoadLibraryA"), mem, 0, NULL);
	if (hThread == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "can't create a thread in that pid\n");
		VirtualFreeEx(hProcess, mem, strlen(DllName) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}

	WaitForSingleObject(hThread, INFINITE);

	HMODULE hLibrary = NULL;
	if (!GetExitCodeThread(hThread, (LPDWORD)&hLibrary))
	{
		printf("can't get exit code for thread GetLastError() = %i.\n", GetLastError());
		CloseHandle(hThread);
		VirtualFreeEx(hProcess, mem, strlen(DllName) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}

	CloseHandle(hThread);
	VirtualFreeEx(hProcess, mem, strlen(DllName) + 1, MEM_RELEASE);

	if (hLibrary == NULL)
	{
		hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandle("KERNEL32.DLL"),"GetLastError"), 0, 0, NULL);
		if (hThread == INVALID_HANDLE_VALUE)
		{
			fprintf(stderr, "LoadLibraryA returned NULL and can't get last error.\n");
			CloseHandle(hProcess);
			return 0;
		}

		WaitForSingleObject(hThread, INFINITE);
		DWORD error;
		GetExitCodeThread(hThread, &error);

		CloseHandle(hThread);

		printf("LoadLibrary return NULL, GetLastError() is %i\n", error);
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

	FARPROC loc =   (FARPROC)((DWORD)pFunction - (DWORD)hLocalLibrary);

	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((DWORD)hLibrary+(DWORD)loc), mem, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return true;
}