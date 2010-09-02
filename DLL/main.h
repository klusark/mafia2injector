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

	/*static C_GameScriptEngine* Singleton()
	{
		return ( C_GameScriptEngine* )
			*reinterpret_cast< unsigned long* >( 0x1AADBC8 );
	}*/
};

lua_State* GetL();