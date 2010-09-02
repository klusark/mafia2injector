#pragma once

#include "Export.h"

void LoadPointers();

int luaL_loadbuffer( lua_State *L, char *buff, size_t size, char *name );