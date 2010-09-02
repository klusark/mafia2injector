#include "Export.h"
#include "Common.h"

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


void LoadPointers()
{
	plua_pcall = (lua_pcall_t)FindPattern(( BYTE* ) "\x55\x8b\xec\x83\xec\x14\x83\x7d\x14\x00\x75\x09\xc7\x45\xf0\x00", "xxxxxxxxxxxxxxxx" );
	plua_tolstring = (lua_tolstring_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x51\x8B\x45\x0C\x50\x8B\x4D\x08\x51\xE8\x8F\xF8\xFF", "xxxxxxxxxxxxxxxx" );
	plua_pushcclosure = (lua_pushcclosure_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x83\xEC\x10\x8B\x45\x08\x8B\x48\x10\x8B\x55\x08\x8B", "xxxxxxxxxxxxxxxx" );
	plua_setfield = (lua_setfield_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x83\xEC\x20\x8B\x45\x0C\x50\x8B\x4D\x08\x51\xE8\xCD", "xxxxxxxxxxxxxxxx" );
	plua_gettop = (lua_gettop_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x8B\x45\x08\x8B\x4D\x08\x8B\x40\x08\x2B\x41\x0C\xC1", "xxxxxxxxxxxxxxxx" );
	pluaL_loadbuffer = (luaL_loadbuffer_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x83\xEC\x08\x8B\x45\x0C\x89\x45\xF8\x8B\x4D\x10\x89", "xxxxxxxxxxxxxxxx" );
	plua_tointeger = (lua_tointeger_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x83\xEC\x14\x8B\x45\x0C\x50\x8B\x4D\x08\x51\xE8\x3D", "xxxxxxxxxxxxxxxx" );
	pluaL_loadbuffer = (luaL_loadbuffer_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x83\xEC\x08\x8B\x45\x0C\x89\x45\xF8\x8B\x4D\x10\x89", "xxxxxxxxxxxxxxxx" );
	plua_pushinteger = (lua_pushinteger_t)FindPattern(( BYTE* ) "\x55\x8B\xEC\x51\x8B\x45\x08\x8B\x48\x08\x89\x4D\xFC\xF3\x0F\x2A", "xxxxxxxxxxxxxxxx" );
}