#include "Export.h"
#include "Common.h"
#include <sstream>
#include "LuaFunctions.h"
#include "main.h"


bool bDataCompare( const BYTE* pData, const BYTE* bMask, const char* szMask )
{
    for( ; *szMask; ++szMask, ++pData, ++bMask )
	{
        if( *szMask == 'x' && *pData != *bMask ) 
		{
            return false;
		}
	}
    return ( *szMask ) == NULL;
}

DWORD FindPattern( DWORD dwAddress, DWORD dwLen, BYTE *bMask, char* szMask )
{
    for( DWORD i=0; i < dwLen; i++ )
	{
		if( bDataCompare( ( BYTE* )( dwAddress + i ), bMask, szMask) )
		{
			return ( DWORD )( dwAddress + i );
		}
	}
    return 0;
}

DWORD FindPattern(BYTE *bMask, char* szMask )
{
	return FindPattern((DWORD)GetModuleHandle( 0 ), 0xFFFFFFFF, bMask, szMask);
}


typedef int             ( __cdecl *luaL_loadbuffer_t )( lua_State *L, char *buff, size_t size, char *name );
luaL_loadbuffer_t		pluaL_loadbuffer;

__declspec(dllexport) int luaL_loadbuffer( lua_State *L, char *buff, size_t size, char *name )
{
	return pluaL_loadbuffer(L, buff, size, name);
}


typedef int             ( __cdecl *lua_pcall_t )( lua_State *L, int nargs, int nresults, int errfunc );
lua_pcall_t				plua_pcall;

__declspec(dllexport) int lua_pcall( lua_State *L, int nargs, int nresults, int errfunc )
{
	return plua_pcall(L, nargs, nresults, errfunc);
}


typedef const char *	(__cdecl *lua_tolstring_t) (lua_State *L, int idx, size_t *len);
lua_tolstring_t			plua_tolstring;

__declspec(dllexport) const char *lua_tolstring(lua_State *L, int idx, size_t *len)
{
	return plua_tolstring(L, idx, len);
}


typedef void			(__cdecl *lua_pushcclosure_t) (lua_State *L, lua_CFunction fn, int n);
lua_pushcclosure_t		plua_pushcclosure;

__declspec(dllexport) void lua_pushcclosure(lua_State *L, lua_CFunction fn, int n)
{
	plua_pushcclosure(L, fn, n);
}


typedef void			(__cdecl *lua_setfield_t) (lua_State *L, int idx, const char *k);
lua_setfield_t			plua_setfield;

__declspec(dllexport) void lua_setfield(lua_State *L, int idx, const char *k)
{
	plua_setfield(L, idx, k);
}


typedef	int				(__cdecl *lua_gettop_t) (lua_State *L);
lua_gettop_t			plua_gettop;

__declspec(dllexport) int lua_gettop(lua_State *L)
{
	return plua_gettop(L);
}


typedef	lua_Integer		(__cdecl *lua_tointeger_t) (lua_State *L, int idx);
lua_tointeger_t			plua_tointeger;

__declspec(dllexport) lua_Integer lua_tointeger(lua_State *L, int idx)
{
	return plua_tointeger(L, idx);
}


typedef	void		(__cdecl *lua_pushinteger_t) (lua_State *L, lua_Integer n);
lua_pushinteger_t		plua_pushinteger;

__declspec(dllexport) void lua_pushinteger(lua_State *L, lua_Integer n)
{
	plua_pushinteger(L, n);
}


typedef	lua_State *		(__cdecl *lua_newthread_t) (lua_State *L);
lua_newthread_t		plua_newthread;

__declspec(dllexport) lua_State *lua_newthread(lua_State *L)
{
	return plua_newthread(L);
}


C_GameScriptEngine *gpEngine;
lua_State* GetL(C_GameScriptEngine *pEngine)
{
	if (pEngine == 0)
		pEngine = gpEngine;

	if( pEngine == NULL ) return NULL;
	if( pEngine->Handler == NULL || pEngine->Handler == (C_ScriptHandler* )0xF) return NULL;
	if( pEngine->Handler->Pool == NULL ) return NULL;
	if( pEngine->Handler->Pool->List == NULL ) return NULL;
	if( pEngine->Handler->Pool->List[0] == NULL ) return NULL;
	return pEngine->Handler->Pool->List[0]->L;
}

void logPointer(std::string name, DWORD pointer)
{
	std::stringstream ss;
	ss << name << " (" << std::hex << pointer << ")";
	log(ss.str());
}

#define GetPointerFromPattern(name, pattern) p##name = (name##_t)FindPattern(( BYTE* ) pattern, "xxxxxxxxxxxxxxxx" );\
	logPointer(#name, (DWORD)p##name);\
	if(!p##name)return false

bool LoadPointers()
{
	unsigned long engine = FindPattern(( BYTE* ) "\xA1\x00\x00\x00\x00\x85\xC0\x75\x22\x6A\x20\xE8\x00\x00\x00\x00\x83\xC4\x04\x85\xC0", "x????xxxxxxx????xxxxx" )+1;
	gpEngine		= (C_GameScriptEngine *)*( DWORD* )(*( DWORD* ) engine);
	logPointer("gpEngine", (DWORD)gpEngine);
	if (!gpEngine)
		return false;

	GetPointerFromPattern(lua_pcall,		"\x55\x8B\xEC\x83\xEC\x14\x83\x7D\x14\x00\x75\x09\xC7\x45\xF0\x00");

	GetPointerFromPattern(lua_tolstring,	"\x55\x8B\xEC\x51\x8B\x45\x0C\x50\x8B\x4D\x08\x51\xE8\x8F\xF8\xFF");

	GetPointerFromPattern(lua_pushcclosure,	"\x55\x8B\xEC\x83\xEC\x10\x8B\x45\x08\x8B\x48\x10\x8B\x55\x08\x8B");

	GetPointerFromPattern(lua_setfield,		"\x55\x8B\xEC\x83\xEC\x20\x8B\x45\x0C\x50\x8B\x4D\x08\x51\xE8\xCD");

	GetPointerFromPattern(lua_gettop,		"\x55\x8B\xEC\x8B\x45\x08\x8B\x4D\x08\x8B\x40\x08\x2B\x41\x0C\xC1");

	GetPointerFromPattern(luaL_loadbuffer,	"\x55\x8B\xEC\x83\xEC\x08\x8B\x45\x0C\x89\x45\xF8\x8B\x4D\x10\x89");

	GetPointerFromPattern(lua_tointeger,	"\x55\x8B\xEC\x83\xEC\x14\x8B\x45\x0C\x50\x8B\x4D\x08\x51\xE8\x3D");

	GetPointerFromPattern(lua_pushinteger,	"\x55\x8B\xEC\x51\x8B\x45\x08\x8B\x48\x08\x89\x4D\xFC\xF3\x0F\x2A");

	GetPointerFromPattern(lua_newthread,	"\x55\x8B\xEC\x83\xEC\x08\x8B\x45\x08\x8B\x48\x10\x8B\x55\x08\x8B");


	return true;
}
