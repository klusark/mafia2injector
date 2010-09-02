#include "../Common/Export.h"
#include <Windows.h>

extern "C"{
__declspec(dllexport) bool StartPlugin()
{
	return true;
}
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}