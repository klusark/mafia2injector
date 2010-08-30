#include <windows.h>
#include <stdio.h>
#include "main.h"
#include <fstream>
#include <string>
#include <sstream>

void log(std::string message)
{
	std::fstream file("log.log", std::ios::out|std::ios::app);
	file << message;
	file << "\n";
	file.close();
}


typedef int             ( __cdecl *luaL_loadbuffer_t )( lua_State *L, char *buff, size_t size, char *name );
luaL_loadbuffer_t               pluaL_loadbuffer = (luaL_loadbuffer_t)0x5C54C0;

typedef int             ( __cdecl *lua_pcall_t )( lua_State *L, int nargs, int nresults, int errfunc );
lua_pcall_t                             plua_pcall = (lua_pcall_t)0x5C3870;

void LoadLuaFile(const std::string &name)
{
	std::string file = "function dofile (filename)local f = assert(loadfile(filename)) return f() end dofile(\"";
	file.append(name);
	file.append("\")");
	lua_State* state = GetL();
	if (!state){
		log("BadState");
		return;
	}

	pluaL_loadbuffer(state, const_cast< char* >(file.c_str()), file.length(), "test");
	int lua_loadb_result = plua_pcall( state, 0, LUA_MULTRET, 0 );
	if( lua_loadb_result != 0 )
	{
		if( LUA_ERRSYNTAX == lua_loadb_result )
		{
			log("Error loading Lua code into buffer with  (Syntax Error)");
		}
		else if( LUA_ERRMEM == lua_loadb_result )
		{
			log("Error loading Lua code into buffer with name (Memory Allocation Error)");
		}
		else
		{
			log("Error loading Lua code into buffer with name" +lua_loadb_result);
		}
	}
}

DWORD WINAPI mainThread( LPVOID lpParam ) {
	log("thread loaded");
	bool exec = true;
	//std::string string = userscript/autoexec.lua\")";
	while( true ) {
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
		CreateThread( 0, 0, mainThread, 0, 0, 0 );

	}
	return TRUE;
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