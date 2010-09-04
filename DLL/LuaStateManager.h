#pragma once

#include "Export.h"
#include <Windows.h>

class LuaStateManager
{
public:
	LuaStateManager();
	~LuaStateManager();
	static DWORD WINAPI WatcherThread( LPVOID lpParam );
	void StartThread();
	lua_State *GetState();
	bool IsStateGood(lua_State *L);
protected:
	void StateChanged(lua_State *L);
	lua_State *m_pLuaState;
	bool m_bEnded;
};

