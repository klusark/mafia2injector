#include "PluginSystem.h"
#include <Windows.h>
#include <string>
#include "Common.h"
#include "main.h"

PluginSystem::PluginSystem()
{
}

void PluginSystem::LoadPlugins()
{
	WIN32_FIND_DATA data;
	HANDLE file = FindFirstFileEx("InjectorPlugins\\*.dll", FindExInfoStandard, &data, FindExSearchNameMatch, 0, 0);
	if (file == (HANDLE)0xffffffff)
		return;

	do {
		std::string path = "InjectorPlugins\\";
		path += data.cFileName;
		HMODULE lib = LoadLibrary(path.c_str());
		if (!lib){
			log("could not load " + path);
			continue;
		}
		Plugin plugin;

		StartPlugin_t pStartPlugin = (StartPlugin_t)GetProcAddress(lib, "StartPlugin");
		if (!pStartPlugin){
			log("could find StartPlugin in " + path);
			continue;
		}
		StopPlugin_t pStopPlugin = (StopPlugin_t)GetProcAddress(lib, "StopPlugin");
		if (!pStopPlugin){
			log("could find StopPlugin in " + path);
			continue;
		}
		plugin.pStartPlugin = pStartPlugin;
		plugin.pStopPlugin = pStopPlugin;

		plugins.push_back(plugin);
		log("loaded " + path);

	} while (file && FindNextFile(file, &data));
	
}

void PluginSystem::StartPlugins()
{
	for (unsigned int i = 0; i < plugins.size(); ++i){
		plugins[i].pStartPlugin(gLuaStateManager.GetState());
	}
}


void PluginSystem::StopPlugins()
{
	for (unsigned int i = 0; i < plugins.size(); ++i){
		plugins[i].pStopPlugin();
	}
}
