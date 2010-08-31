#include <windows.h>
#include <stdio.h>
#include "main.h"
#include <string>
#include <sstream>
#include "Common.h"


typedef int             ( __cdecl *luaL_loadbuffer_t )( lua_State *L, char *buff, size_t size, char *name );
luaL_loadbuffer_t               pluaL_loadbuffer = (luaL_loadbuffer_t)0x5C54C0;

typedef int             ( __cdecl *lua_pcall_t )( lua_State *L, int nargs, int nresults, int errfunc );
lua_pcall_t                             plua_pcall = (lua_pcall_t)0x5C3870;

void ExecuteLua(const std::string &lua);

void LoadLuaFile(const std::string &name)
{
	std::string file = "function dofile (filename)local f = assert(loadfile(filename)) return f() end dofile(\"";
	file.append(name);
	file.append("\")");
	ExecuteLua(file);
}

void ExecuteLua(const std::string &lua)
{
	lua_State* state = GetL();
	if (!state){
		log("BadState");
		return;
	}

	pluaL_loadbuffer(state, const_cast< char* >(lua.c_str()), lua.length(), "test");
	int lua_loadb_result = plua_pcall( state, 0, LUA_MULTRET, 0 );
	if( lua_loadb_result != 0 )
	{
		if( LUA_ERRSYNTAX == lua_loadb_result )
		{
			log("Error loading Lua code into buffer with (Syntax Error)");
		}
		else if( LUA_ERRMEM == lua_loadb_result )
		{
			log("Error loading Lua code into buffer with (Memory Allocation Error)");
		}
		else
		{
			std::stringstream ss;
			ss << "Error loading Lua code into buffer. Error ";
			ss << lua_loadb_result;
			log(ss.str());
		}
	}
}


DWORD WINAPI mainThread( LPVOID lpParam ) {
	log("thread loaded");
	for (;;) {
		Sleep( 2 );
		for (int i = 0; i < 12; ++i)
			if( GetAsyncKeyState( VK_F1+i ) & 1 ) {
				std::stringstream ss;
				ss<<"userscript/f";
				ss<<i+1;
				ss<<".lua";
				LoadLuaFile(ss.str());
			}
		
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	if( ul_reason_for_call == DLL_PROCESS_ATTACH ) {
		log("loaded");
		

	}
	return TRUE;
}

extern "C" {
__declspec(dllexport) void __cdecl RunLua(const char *lua)
{
	log(lua);
	ExecuteLua(lua);
}

__declspec(dllexport) void __cdecl StartThread(void)
{
	CreateThread( 0, 0, mainThread, 0, 0, 0 );
}

__declspec(dllexport) void __cdecl InjectLua(const char *lua)
{
	log(lua);
	DWORD pid = GetHandleByProcessName("Mafia2.exe");
	if (!pid){
		log("Could not find process Mafia2.exe");
		return;
	}
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hProcess){
		std::stringstream ss;
		ss << "Could not open process ";
		ss << pid;
		log(ss.str());
		return;
	}
	char dir[256];
	GetCurrentDirectory(256, dir);
	std::string fulldir(dir);
	fulldir += "\\MafiaDll.dll";
	HMODULE hLibrary = InjectDll(hProcess, fulldir.c_str());
	if (!hLibrary){
		log("Could not inject " + fulldir);
		CloseHandle(hProcess);
		return;
	}
	PVOID mem = VirtualAllocEx(hProcess, NULL, strlen(lua) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (!mem){
		log("Could not allocate memory");
		CloseHandle(hProcess);
		return;
	}
	if (WriteProcessMemory(hProcess, mem, (void*)lua, strlen(lua) + 1, NULL) == 0){
		log("Could not write memory");
		CloseHandle(hProcess);
		return;
	}
	if (!LoadRemoteFunction(hProcess, hLibrary, "MafiaDll.dll", "RunLua", mem)){
		log("Could not run lua");
		CloseHandle(hProcess);
		return;
	}
	VirtualFreeEx(hProcess, mem, strlen(lua) + 1, MEM_RELEASE);
	CloseHandle(hProcess);
}
}



lua_State* GetL()
{
	C_GameScriptEngine* pEngine = ( C_GameScriptEngine* ) *( DWORD* )( 0x01AADBC8 );//C_GameScriptEngine::Singleton();
	if( pEngine == NULL ) return NULL;
	if( pEngine->Handler == NULL ) return NULL;
	if( pEngine->Handler->Pool == NULL ) return NULL;
	if( pEngine->Handler->Pool->List == NULL ) return NULL;
	if( pEngine->Handler->Pool->List[0] == NULL ) return NULL;
	return pEngine->Handler->Pool->List[0]->L;
}