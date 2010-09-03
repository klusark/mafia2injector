#pragma once

extern "C" {
#include <lua.h>
}
#include <string>

LUA_API bool ExecuteLua(lua_State *L, const std::string &lua);