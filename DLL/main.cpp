#include <windows.h>
#include <stdio.h>
#include "main.h"
#include <string>
#include <sstream>
#include "Common.h"
#include <map>
#include <vector>

typedef int             ( __cdecl *luaL_loadbuffer_t )( lua_State *L, char *buff, size_t size, char *name );
luaL_loadbuffer_t		pluaL_loadbuffer;

typedef int             ( __cdecl *lua_pcall_t )( lua_State *L, int nargs, int nresults, int errfunc );
lua_pcall_t				plua_pcall;

typedef const char *	(__cdecl *lua_tolstring_t) (lua_State *L, int idx, size_t *len);
lua_tolstring_t			plua_tolstring;

typedef void			(__cdecl *lua_pushcclosure_t) (lua_State *L, lua_CFunction fn, int n);
lua_pushcclosure_t		plua_pushcclosure;

typedef void			(__cdecl *lua_setfield_t) (lua_State *L, int idx, const char *k);
lua_setfield_t			plua_setfield;

typedef	int				(__cdecl *lua_gettop_t) (lua_State *L);
lua_gettop_t			plua_gettop;

typedef	lua_Integer		(__cdecl *lua_tointeger_t) (lua_State *L, int idx);
lua_tointeger_t			plua_tointeger;

typedef	lua_Integer		(__cdecl *lua_pushinteger_t) (lua_State *L, lua_Integer n);
lua_pushinteger_t		plua_pushinteger;


#define plua_setglobal(L,s)	plua_setfield(L, LUA_GLOBALSINDEX, (s))
#define plua_pushcfunction(L,f)	plua_pushcclosure(L, (f), 0)
#define plua_register(L,n,f) (plua_pushcfunction(L, (f)), plua_setglobal(L, (n)))

std::map<char, std::string> keyBinds;
struct repeatinfo
{
	repeatinfo()
	{
		lastExec = 0;
		numtimes = -1;
	}
	int delta, numtimes;
	unsigned int lastExec, ID;
	std::string lua;
};
std::vector<repeatinfo> repeats;
unsigned int repeatID = 0;

bool luaRegistered = false;

bool ExecuteLua(const std::string &lua);

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

static int repeatedCallback(lua_State *L)
{
	lua_State* state = GetL();
	int numargs = plua_gettop(state);
	if (numargs < 2 || numargs > 3)
		return 0;

	int delta = plua_tointeger(state, 1);
	size_t size = 0;
	const char *lua = plua_tolstring(state, 2, &size);
	repeatinfo test;
	test.delta = delta;
	test.lua = lua;
	if (numargs == 3){
		int numtimes = plua_tointeger(state, 3);
		test.numtimes = numtimes;
	}
	test.ID = repeatID++;
	repeats.push_back(test);
	plua_pushinteger(state, test.ID);
	return 1;
}

static int killCallback(lua_State *L)
{
	lua_State* state = GetL();
	int numargs = plua_gettop(state);
	if (numargs != 1)
		return 0;

	int ID = plua_tointeger(state, 1);
	for (unsigned int i = 0; i < repeats.size(); ++i){
		if (repeats[i].ID == ID){
			repeats.erase(repeats.begin()+i);
			break;
		}
	}

	return 0;
}

bool ExecuteLua(const std::string &lua)
{
	lua_State* state = GetL();
	if (!state){
		log("BadState");
		return false;
	}

	if (!luaRegistered){
		luaRegistered = true;
		plua_pcall = (lua_pcall_t)FindPattern(( BYTE* ) "\x55\x8b\xec\x83\xec\x14\x83\x7d\x14\x00\x75\x09\xc7\x45\xf0\x00", "xxxxxxxxxxxxxxxx" );
		plua_tolstring = (lua_tolstring_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x51\x8B\x45\x0C\x50\x8B\x4D\x08\x51\xE8\x8F\xF8\xFF", "xxxxxxxxxxxxxxxx" );
		plua_pushcclosure = (lua_pushcclosure_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x83\xEC\x10\x8B\x45\x08\x8B\x48\x10\x8B\x55\x08\x8B", "xxxxxxxxxxxxxxxx" );
		plua_setfield = (lua_setfield_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x83\xEC\x20\x8B\x45\x0C\x50\x8B\x4D\x08\x51\xE8\xCD", "xxxxxxxxxxxxxxxx" );
		plua_gettop = (lua_gettop_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x8B\x45\x08\x8B\x4D\x08\x8B\x40\x08\x2B\x41\x0C\xC1", "xxxxxxxxxxxxxxxx" );
		pluaL_loadbuffer = (luaL_loadbuffer_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x83\xEC\x08\x8B\x45\x0C\x89\x45\xF8\x8B\x4D\x10\x89", "xxxxxxxxxxxxxxxx" );
		plua_tointeger = (lua_tointeger_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x83\xEC\x14\x8B\x45\x0C\x50\x8B\x4D\x08\x51\xE8\x3D", "xxxxxxxxxxxxxxxx" );
		pluaL_loadbuffer = (luaL_loadbuffer_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x83\xEC\x08\x8B\x45\x0C\x89\x45\xF8\x8B\x4D\x10\x89", "xxxxxxxxxxxxxxxx" );
		plua_pushinteger = (lua_tointeger_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x51\x8B\x45\x08\x8B\x48\x08\x89\x4D\xFC\xF3\x0F\x2A", "xxxxxxxxxxxxxxxx" );
		
		plua_register(state, "bindKey", bindKey);
		plua_register(state, "repeatedCallback", repeatedCallback);
		plua_register(state, "killCallback", killCallback);
	}

	pluaL_loadbuffer(state, const_cast< char* >(lua.c_str()), lua.length(), "test");
	int lua_loadb_result = plua_pcall( state, 0, LUA_MULTRET, 0 );
	if( lua_loadb_result != 0 )
	{
		if( LUA_ERRSYNTAX == lua_loadb_result )
		{
			log("Error loading Lua code into buffer with (Syntax Error)");
			return false;
		}
		else if( LUA_ERRMEM == lua_loadb_result )
		{
			log("Error loading Lua code into buffer with (Memory Allocation Error)");
			return false;
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
			return false;
		}
	}
	return true;
}



DWORD WINAPI mainThread( LPVOID lpParam ) {
	log("thread loaded");
	unsigned int time = 0;
	for (;;) {
		Sleep( 1 );
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
		for (unsigned int i = 0; i < repeats.size(); ++i){
			repeatinfo &info = repeats[i];
			if (info.delta + info.lastExec < time){
				if (!ExecuteLua(info.lua)){
					repeats.erase(repeats.begin()+i);
					--i;
				}else{
					info.lastExec += info.delta;
					if (info.numtimes != -1)
						--info.numtimes;
					if (info.numtimes == 0){
						repeats.erase(repeats.begin()+i);
						--i;
					}
				}
			}
		}
		++time;
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