#include <windows.h>
#include <stdio.h>
#include "main.h"
#include <string>
#include <sstream>
#include "Common.h"
#include <map>
#include <vector>
#include "LuaFunctions.h"


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
	int numargs = lua_gettop(state);
	if (numargs != 2)
		return 0;
	size_t size = 0;
	const char *key = lua_tolstring(state, 1, &size);

	const char *lua = lua_tolstring(state, 2, &size);
	char charkey = key[0];
	keyBinds[charkey] = std::string(lua);

	return 0;
}

static int repeatedCallback(lua_State *L)
{
	lua_State* state = GetL();
	int numargs = lua_gettop(state);
	if (numargs < 2 || numargs > 3)
		return 0;

	int delta = lua_tointeger(state, 1);
	size_t size = 0;
	const char *lua = lua_tolstring(state, 2, &size);
	repeatinfo test;
	test.delta = delta;
	test.lua = lua;
	if (numargs == 3){
		int numtimes = lua_tointeger(state, 3);
		test.numtimes = numtimes;
	}
	test.ID = repeatID++;
	repeats.push_back(test);
	lua_pushinteger(state, test.ID);
	return 1;
}

static int killCallback(lua_State *L)
{
	lua_State* state = GetL();
	int numargs = lua_gettop(state);
	if (numargs != 1)
		return 0;

	int ID = lua_tointeger(state, 1);
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
		LoadPointers();
		
		lua_register(state, "bindKey", bindKey);
		lua_register(state, "repeatedCallback", repeatedCallback);
		lua_register(state, "killCallback", killCallback);
	}

	luaL_loadbuffer(state, const_cast< char* >(lua.c_str()), lua.length(), "test");
	int lua_loadb_result = lua_pcall( state, 0, LUA_MULTRET, 0 );
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
			const char *error = lua_tolstring(state, -1, &size);
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

typedef bool             ( __cdecl *StartPlugin_t )();

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	if( ul_reason_for_call == DLL_PROCESS_ATTACH ) {
		log("loaded");
		WIN32_FIND_DATA data;
		HANDLE file = FindFirstFileEx("InjectorPlugins\\*.dll", FindExInfoStandard, &data, FindExSearchNameMatch, 0, 0);
		if (file != (HANDLE)0xffffffff){
			do {
				std::string path = "InjectorPlugins\\";
				path += data.cFileName;
				HMODULE lib = LoadLibrary(path.c_str());
				if (!lib){
					log("could not load " + path);
					continue;
				}
				StartPlugin_t pStartPlugin = (StartPlugin_t)GetProcAddress(lib, "StartPlugin");
				if (!pStartPlugin){
					log("could find StartPlugin in " + path);
					continue;
				}
				if (!pStartPlugin()){
					log("StartPlugin in " + path + " failed");
					continue;
				}

			} while (file && FindNextFile(file, &data));
		}

		
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