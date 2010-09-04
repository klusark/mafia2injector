#pragma once

#include <vector>
#include "Export.h"

typedef bool             ( __cdecl *StartPlugin_t )(lua_State *L);
typedef bool             ( __cdecl *StopPlugin_t )();
struct Plugin
{
	StartPlugin_t pStartPlugin;
	StopPlugin_t pStopPlugin;
};


class PluginSystem
{
public:
	PluginSystem();
	void LoadPlugins();
	
	void StartPlugins();
	void StopPlugins();
protected:
	std::vector <Plugin> plugins;
};

