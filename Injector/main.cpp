#include <windows.h>
#include <stdio.h>
#include "Common.h"


int main()
{
	DWORD pid = GetHandleByProcessName("Mafia2.exe");
	HANDLE hProcess = INVALID_HANDLE_VALUE;
	if (pid){
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	}else {
	
		STARTUPINFO si;
		PROCESS_INFORMATION pi;

		ZeroMemory( &si, sizeof(si) );
		si.cb = sizeof(si);
		ZeroMemory( &pi, sizeof(pi) );

		CreateProcess(0, "Mafia2.exe", 0, 0, false, NORMAL_PRIORITY_CLASS, 0, 0, &si, &pi);

		hProcess = pi.hProcess;
	}
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "cannot open that pid\n");
		return 1;
	}
	HMODULE hLibrary = InjectDll(hProcess, "MafiaDll.dll");
	if (hLibrary){

		printf("injected %08x\n", (DWORD)hLibrary);
	} else {
		log("Could not load library");
		return 1;
	}
	if (!LoadRemoteFunction(hProcess, hLibrary, "MafiaDll.dll", "StartThread", 0)){
		log("Could not start thread");
		return 1;
	}
	
	CloseHandle(hProcess);
	return 0;
}

