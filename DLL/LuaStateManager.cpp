#include "LuaStateManager.h"
#include <Windows.h>
#include "LuaFunctions.h"
#include "main.h"

LuaStateManager::LuaStateManager()
{
	m_pLuaState = 0;
	m_bEnded = false;
}

LuaStateManager::~LuaStateManager()
{
	m_bEnded = true;
}

void LuaStateManager::StartThread()
{
	CreateThread( 0, 0, LuaStateManager::WatcherThread, 0, 0, 0 );
}

void LuaStateManager::StateChanged(lua_State *L)
{
	m_pLuaState = L;
	gPluginSystem.StartPlugins();
}

lua_State * LuaStateManager::GetState()
{
	if (m_pLuaState)
		return lua_newthread(m_pLuaState);
	else
		return 0;
}

bool LuaStateManager::IsStateGood(lua_State *L)
{
	bool result = true;
	if (!L)
		result = false;
	return result;
}

DWORD WINAPI LuaStateManager::WatcherThread( LPVOID lpParam ) {
	do {
		Sleep(1000);
	} while (!LoadPointers());
		

	lua_State *lastState = 0;
	while(!gLuaStateManager->m_bEnded) {
		lua_State* nstate = GetL();
		if (nstate != lastState && nstate){
			gLuaStateManager->StateChanged(nstate);
			lastState = nstate;
		}

		Sleep(1000);
	}
	return 0;
}
