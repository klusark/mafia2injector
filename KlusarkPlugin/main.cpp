#include "../Common/Export.h"
#include <Windows.h>
#include <map>
#include <vector>

std::map<char, std::string> keyBinds;
lua_State* state = 0;

struct repeatinfo
{
	repeatinfo()
	{
		lastExec = 0;
		numtimes = -1;
	}
	int delta, numtimes;
	unsigned int lastExec, ID;
	std::string lua;
};
std::vector<repeatinfo> repeats;
unsigned int repeatID = 0;

bool KeyBindsThreadStarted = false;
bool RepeatThreadStarted = false;

DWORD WINAPI KeyBindsThread( LPVOID lpParam ) {
	KeyBindsThreadStarted = true;
	unsigned int time = 0;
	for (;;){
		for (auto it = keyBinds.begin(); it != keyBinds.end(); ++it){
			if (GetAsyncKeyState(it->first) & 1) {
				ExecuteLua(state, it->second);
			}
		}
		for (unsigned int i = 0; i < repeats.size(); ++i){
			repeatinfo &info = repeats[i];
			if (info.delta + info.lastExec < time){
				if (!ExecuteLua(state, info.lua)){
					repeats.erase(repeats.begin()+i);
					--i;
				}else{
					info.lastExec += info.delta;
					if (info.numtimes != -1)
						--info.numtimes;
					if (info.numtimes == 0){
						repeats.erase(repeats.begin()+i);
						--i;
					}
				}
			}
		}
		++time;
		Sleep(1);
	}
	KeyBindsThreadStarted = false;
}



static int bindKey(lua_State *L)
{
	int numargs = lua_gettop(L);
	if (numargs != 2)
		return 0;
	size_t size = 0;
	const char *key = lua_tolstring(L, 1, &size);

	const char *lua = lua_tolstring(L, 2, &size);
	char charkey = key[0];
	keyBinds[charkey] = std::string(lua);

	return 0;
}

static int repeatedCallback(lua_State *L)
{
	int numargs = lua_gettop(L);
	if (numargs < 2 || numargs > 3)
		return 0;

	int delta = lua_tointeger(L, 1);
	size_t size = 0;
	const char *lua = lua_tolstring(L, 2, &size);
	repeatinfo test;
	test.delta = delta;
	test.lua = lua;
	if (numargs == 3){
		int numtimes = lua_tointeger(L, 3);
		test.numtimes = numtimes;
	}
	test.ID = repeatID++;
	repeats.push_back(test);
	lua_pushinteger(L, test.ID);
	return 1;
}

static int killCallback(lua_State *L)
{
	int numargs = lua_gettop(L);
	if (numargs != 1)
		return 0;

	int ID = lua_tointeger(L, 1);
	for (unsigned int i = 0; i < repeats.size(); ++i){
		if (repeats[i].ID == ID){
			repeats.erase(repeats.begin()+i);
			break;
		}
	}

	return 0;
}

extern "C"{
	__declspec(dllexport) bool StartPlugin(lua_State* newState)
	{
		state = newState;
		lua_register(state, "repeatedCallback", repeatedCallback);
		lua_register(state, "killCallback", killCallback);
		lua_register(state, "bindKey", bindKey);
		if (!KeyBindsThreadStarted)
			CreateThread( 0, 0, KeyBindsThread, 0, 0, 0 );

		return true;
	}
	__declspec(dllexport) bool StopPlugin()
	{
		return true;
	}
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}