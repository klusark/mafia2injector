#include "Common.h"
#include <psapi.h>
#include <fstream>

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
		output += tolower(str[i]);
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
	PVOID mem = VirtualAllocEx(hProcess, NULL, strlen(DllName) + 1, MEM_COMMIT, PAGE_READWRITE);

	if (mem == NULL)
	{
		fprintf(stderr, "can't allocate memory in that pid\n");
		CloseHandle(hProcess);
		return 0;
	}
	log("section4.1");
	if (WriteProcessMemory(hProcess, mem, (void*)DllName, strlen(DllName) + 1, NULL) == 0)
	{
		fprintf(stderr, "can't write to memory in that pid\n");
		VirtualFreeEx(hProcess, mem, strlen(DllName) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}
	log("section4.2");
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) GetProcAddress(GetModuleHandle("KERNEL32.DLL"),"LoadLibraryA"), mem, 0, NULL);
	if (hThread == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "can't create a thread in that pid\n");
		VirtualFreeEx(hProcess, mem, strlen(DllName) + 1, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}
	log("section4.3");
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
	log("section4.4");
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
		log("section4.5");
		WaitForSingleObject(hThread, INFINITE);
		DWORD error;
		GetExitCodeThread(hThread, &error);

		CloseHandle(hThread);

		printf("LoadLibrary return NULL, GetLastError() is %i\n", error);
		CloseHandle(hProcess);
		return false;
	}
	log("section4.6");

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