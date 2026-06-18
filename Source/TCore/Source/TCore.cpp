#include "TCore.h"

// External
#include <assert.h>
#include <stdio.h>

#include <cstring>
#include <dynalo/dynalo.hpp>
#include <string>
#include <unordered_map>
#include <vector>

TCORE_PLUGIN_INIT(TC)

static const char* TCORE_ERROR_TEXT_DLL_NOT_FOUND = "DLL file isn't found: %s\n";
static const char* TCORE_ERROR_TEXT_ENTRY_NOT_FOUND = "DLL file is found but plugin entry isn't\n";

struct TCContext
{
	struct TCPluginStored
	{
		dynalo::native::handle NativeHandle;
		TCPluginInfo Info;
		TCPluginFunctions Functions;
	};
	std::unordered_map<std::string, TCPluginStored> Plugins;
};

static TCContext* Context = nullptr;

void StrCopy(char** dst, const char* src)
{
	size_t srclen = std::strlen(src);
	*dst = new char[srclen + 1];
	std::memcpy(*dst, src, srclen);
	(*dst)[srclen] = '\0';
}

TCResult LoadPlugin(const char* path, const TCPluginInfo** outInfo)
{
	try
	{
		auto handle = dynalo::open(path);
		auto entryPoint = dynalo::get_function<void(const ITC*, TCPluginInfo*, TCPluginFunctions*)>(
			handle, "TCORE_PLUGIN_ENTRY_FUNC");
		if (!entryPoint)
		{
			printf(TCORE_ERROR_TEXT_ENTRY_NOT_FOUND);
			return TC_RESULT_FAILURE;
		}

		TCPluginInfo info;
		TCPluginFunctions functions;
		entryPoint(TC, &info, &functions);

		StrCopy((char**)&info.Name, info.Name);
		StrCopy((char**)&info.RootFolderPath, info.RootFolderPath);
		info.Version = info.Version;

		Context->Plugins[path] = {handle, info, functions};
		*outInfo = &Context->Plugins[path].Info;
		return TC_RESULT_SUCCESS;
	}
	catch (std::exception& e)
	{
		printf(TCORE_ERROR_TEXT_DLL_NOT_FOUND, path);
		return TC_RESULT_FAILURE;
	}
}

TCResult UnloadPlugin(const char* pluginName)
{
	auto it = Context->Plugins.find(pluginName);
	if (it == Context->Plugins.end())
	{
		printf("Plugin isn't found: %s\n", pluginName);
		return TC_RESULT_FAILURE;
	}
	auto plugin = it->second;
	plugin.Functions.OnPreShutdown();
	plugin.Functions.Shutdown();
	dynalo::close(plugin.NativeHandle);

	TCPluginInfo info = plugin.Info;
	Context->Plugins.erase(it);
	for (auto& [name, stored] : Context->Plugins)
		stored.Functions.OnPluginLoadStateChange(&plugin.Info, false);
	delete[] plugin.Info.Name;
	delete[] plugin.Info.RootFolderPath;
}

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TC)
TCORE_PLUGIN_ENTRY_POINT_END()

TCResult TC_Initialize(const void** outPluginAPI)
{
	ITC* newTC = new ITC;
	Context = new TCContext();
	newTC->GetVersion = []() -> unsigned int {
		return TC_PLUGIN_VERSION;
	};
	newTC->LoadPlugin = LoadPlugin;
	newTC->UnloadPlugin = UnloadPlugin;
	TC = newTC;
	*outPluginAPI = TC;
	return TC_RESULT_SUCCESS;
}

TCResult TC_OnPreShutdown()
{
	for (auto& [name, stored] : Context->Plugins)
		TC->UnloadPlugin(name.c_str());
	return TC_RESULT_SUCCESS;
}

TCResult TC_Shutdown()
{
	delete Context;
	delete TC;
	return TC_RESULT_SUCCESS;
}

void TC_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, bool isLoaded)
{
	// This plugin doesn't react to other plugins being loaded or unloaded, so this function is empty.
}