#include <windows.h>
#include <stdio.h>
#include "main.h"
#include <string>
#include <sstream>
#include "Common.h"
#include <vector>
#include "LuaFunctions.h"

std::vector <Plugin> plugins;

lua_State *state = 0;

void LoadLuaFile(const std::string &name)
{
	std::string file = "function dofile (filename)local f = assert(loadfile(filename)) return f() end dofile(\"";
	file.append(name);
	file.append("\")");
	ExecuteLua(state, file);
}

void StartPlugins();



bool ExecuteLua(lua_State *L, const std::string &lua)
{

	if (!L){
		log("BadState");
		return false;
	}


	luaL_loadbuffer(L, const_cast< char* >(lua.c_str()), lua.length(), "test");
	int lua_loadb_result = lua_pcall( L, 0, LUA_MULTRET, 0 );
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
			const char *error = lua_tolstring(L, -1, &size);
			log(error);
			return false;
		}
	}
	return true;
}

void StateChanged(lua_State* nstate)
{
	state = lua_newthread(nstate);
	StartPlugins();
}

void StartPlugins()
{
	for (unsigned int i = 0; i < plugins.size(); ++i){
		plugins[i].pStartPlugin(lua_newthread(state));
	}
}

DWORD WINAPI WatcherThread( LPVOID lpParam ) {
	while (!LoadPointers())
		Sleep(1000);

	lua_State *lastState = 0;
	for (;;){
		lua_State* nstate = GetL();
		if (nstate != lastState && nstate){
			StateChanged(nstate);
			lastState = nstate;
		}

		Sleep(1000);
	}
	return TRUE;
}

DWORD WINAPI mainThread( LPVOID lpParam ) {
	log("thread loaded");

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
	}
}



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
				Plugin plugin;

				StartPlugin_t pStartPlugin = (StartPlugin_t)GetProcAddress(lib, "StartPlugin");
				if (!pStartPlugin){
					log("could find StartPlugin in " + path);
					continue;
				}
				StopPlugin_t pStopPlugin = (StopPlugin_t)GetProcAddress(lib, "StopPlugin");
				if (!pStopPlugin){
					log("could find StopPlugin in " + path);
					continue;
				}
				plugin.pStartPlugin = pStartPlugin;
				plugin.pStopPlugin = pStopPlugin;
				/*if (!pStartPlugin()){
					log("StartPlugin in " + path + " failed");
					continue;
				}*/
				plugins.push_back(plugin);
				log("loaded " + path);

			} while (file && FindNextFile(file, &data));
		}


	}
	return TRUE;
}

extern "C" {
	__declspec(dllexport) void __cdecl RunLua(const char *lua)
	{
		log(lua);
		ExecuteLua(state, lua);
	}

	__declspec(dllexport) void __cdecl StartThread(void)
	{
		CreateThread( 0, 0, mainThread, 0, 0, 0 );
		CreateThread( 0, 0, WatcherThread, 0, 0, 0 );
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



