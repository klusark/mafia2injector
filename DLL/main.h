#pragma once
/*extern "C" {
#include <lua.h>
//#include <lauxlib.h>
}*/
#include "Export.h"

#define SCRIPTMACHINE_MAX 12

class C_ScriptEngine
{
public:
	unsigned char            Unknown[ 0x50 ];   //0000
	lua_State*               L;
};

class C_ScriptPool
{
public:
	C_ScriptEngine*            List[ SCRIPTMACHINE_MAX ];
};

class C_ScriptHandler
{
public:
	unsigned long            Unknown;         //0000
	C_ScriptPool*            Pool;            //0004
};

class C_GameScriptEngine
{
public:
	unsigned long            VTable;            //0000
	C_ScriptHandler*         Handler;         //0004
};


lua_State *GetL(C_GameScriptEngine *pEngine = 0);

typedef bool             ( __cdecl *StartPlugin_t )(lua_State *L);
typedef bool             ( __cdecl *StopPlugin_t )();
struct Plugin
{
	StartPlugin_t pStartPlugin;
	StopPlugin_t pStopPlugin;
};
