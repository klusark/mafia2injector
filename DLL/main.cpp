#include <windows.h>
#include <stdio.h>
#include "main.h"
#include <string>
#include <sstream>
#include "Common.h"
#include <map>

typedef int             ( __cdecl *luaL_loadbuffer_t )( lua_State *L, char *buff, size_t size, char *name );
luaL_loadbuffer_t		pluaL_loadbuffer = (luaL_loadbuffer_t)0x5C54C0;

typedef int             ( __cdecl *lua_pcall_t )( lua_State *L, int nargs, int nresults, int errfunc );
lua_pcall_t				plua_pcall = (lua_pcall_t)0x5C3870;

typedef const char *	(__cdecl *lua_tolstring_t) (lua_State *L, int idx, size_t *len);
lua_tolstring_t			plua_tolstring = (lua_tolstring_t)0x5C2950;

typedef void			(__cdecl *lua_pushcclosure_t) (lua_State *L, lua_CFunction fn, int n);
lua_pushcclosure_t		plua_pushcclosure = (lua_pushcclosure_t)0x5C2E40;

typedef void			(__cdecl *lua_setfield_t) (lua_State *L, int idx, const char *k);
lua_setfield_t			plua_setfield = (lua_setfield_t)0x5C3410;

typedef	int				(__cdecl *lua_gettop_t) (lua_State *L);
lua_gettop_t			plua_gettop = (lua_gettop_t)0x5C20F0;


#define plua_setglobal(L,s)	plua_setfield(L, LUA_GLOBALSINDEX, (s))
#define plua_pushcfunction(L,f)	plua_pushcclosure(L, (f), 0)
#define plua_register(L,n,f) (plua_pushcfunction(L, (f)), plua_setglobal(L, (n)))

typedef std::map<char, std::string> keyBinds_t;
keyBinds_t keyBinds;

bool luaRegistered = false;

void ExecuteLua(const std::string &lua);

void LoadLuaFile(const std::string &name)
{
	std::string file = "function dofile (filename)local f = assert(loadfile(filename)) return f() end dofile(\"";
	file.append(name);
	file.append("\")");
	ExecuteLua(file);
}

static int bindKey(lua_State *L)
{
	lua_State* state = GetL();
	int numargs = plua_gettop(state);
	if (numargs != 2)
		return 0;
	size_t size = 0;
	const char *key = plua_tolstring(state, 1, &size);

	const char *lua = plua_tolstring(state, 2, &size);
	char charkey = key[0];
	keyBinds[charkey] = std::string(lua);

	return 0;
}

void ExecuteLua(const std::string &lua)
{
	lua_State* state = GetL();
	if (!state){
		log("BadState");
		return;
	}

	if (!luaRegistered){
		luaRegistered = true;
		plua_register(state, "bindKey", bindKey);
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
			size_t size = 0;
			const char *error = plua_tolstring(state, -1, &size);
			log(error);
		}
	}
}



DWORD WINAPI mainThread( LPVOID lpParam ) {
	log("thread loaded");
	for (;;) {
		Sleep( 2 );
		for (int i = 0; i < 12; ++i){
			if( GetAsyncKeyState( VK_F1+i ) & 1 ) {
				std::stringstream ss;
				ss<<"userscript/f";
				ss<<i+1;
				ss<<".lua";
				LoadLuaFile(ss.str());
				
			}
		}
		for (auto it = keyBinds.begin(); it != keyBinds.end(); ++it){
			if (GetAsyncKeyState(it->first) & 1) {
				ExecuteLua(it->second);
			}
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

__declspec(dllexport) bool __cdecl InjectLua(const char *lua)
{
	log(lua);
	DWORD pid = GetHandleByProcessName("Mafia2.exe");
	if (!pid){
		log("Could not find process Mafia2.exe");
		return false;
	}
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hProcess){
		std::stringstream ss;
		ss << "Could not open process ";
		ss << pid;
		log(ss.str());
		return false;
	}
	char dir[256];
	GetCurrentDirectory(256, dir);
	std::string fulldir(dir);
	fulldir += "\\MafiaDll.dll";
	HMODULE hLibrary = InjectDll(hProcess, fulldir.c_str());
	if (!hLibrary){
		log("Could not inject " + fulldir);
		CloseHandle(hProcess);
		return false;
	}
	PVOID mem = VirtualAllocEx(hProcess, NULL, strlen(lua) + 1, MEM_COMMIT, PAGE_READWRITE);
	if (!mem){
		log("Could not allocate memory");
		CloseHandle(hProcess);
		return false;
	}
	if (WriteProcessMemory(hProcess, mem, (void*)lua, strlen(lua) + 1, NULL) == 0){
		log("Could not write memory");
		CloseHandle(hProcess);
		return false;
	}
	if (!LoadRemoteFunction(hProcess, hLibrary, "MafiaDll.dll", "RunLua", mem)){
		log("Could not run lua");
		CloseHandle(hProcess);
		return false;
	}
	VirtualFreeEx(hProcess, mem, strlen(lua) + 1, MEM_RELEASE);
	CloseHandle(hProcess);
	return true;
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