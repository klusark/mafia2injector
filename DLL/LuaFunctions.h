#pragma once

#include "Export.h"

bool LoadPointers();

__declspec(dllexport) int luaL_loadbuffer( lua_State *L, char *buff, size_t size, char *name );

