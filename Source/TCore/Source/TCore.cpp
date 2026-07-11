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
		dynalo::native::handle NativeHnd;
		TCPluginInfo Info;
		TCPluginFunctions Functions;
		const void* Api;
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
		auto entryPoint = dynalo::get_function<TCResult(const ITC*, TCPluginInfo*, TCPluginFunctions*)>(
			handle, "TCORE_PLUGIN_ENTRY_FUNC");
		if (!entryPoint)
		{
			printf(TCORE_ERROR_TEXT_ENTRY_NOT_FOUND);
			return TC_RESULT_FAILURE;
		}

		TCPluginInfo info{};
		TCPluginFunctions functions{};
		if (auto res = entryPoint(TC, &info, &functions); res != TC_RESULT_SUCCESS)
		{
			printf("Plugin entry point failed to initialize: %s\n", path);
			dynalo::close(handle);
			return res;
		}

		const void* api{};
		if (auto res = functions.Initialize(&api); res != TC_RESULT_SUCCESS)
		{
			printf("Plugin failed to initialize: %s\n", path);
			return res;
		}

		StrCopy((char**)&info.Name, info.Name);
		// StrCopy((char**)&info.RootFolderPath, info.RootFolderPath);
		info.Version = info.Version;

		Context->Plugins[path] = {handle, info, functions, api};
		if (outInfo)
			*outInfo = &Context->Plugins[path].Info;
		return TC_RESULT_SUCCESS;
	}
	catch (std::exception& e)
	{
		printf(TCORE_ERROR_TEXT_DLL_NOT_FOUND, path);
		return TC_RESULT_FAILURE;
	}
}

TCResult GetPlugin(const char* pluginName,
				   unsigned int version,
				   const TCPluginInfo** outPluginInfo,
				   const void** outPluginAPI)
{
	for (auto& [pluginPath, plugin] : Context->Plugins)
	{
		if (strcmp(plugin.Info.Name, pluginName) == 0)
		{
			if (outPluginInfo)
				*outPluginInfo = &plugin.Info;
			if (outPluginAPI)
				*outPluginAPI = plugin.Api;
			return TC_RESULT_SUCCESS;
		}
	}
	return TC_RESULT_NOT_FOUND;
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
	dynalo::close(plugin.NativeHnd);

	TCPluginInfo info = plugin.Info;
	Context->Plugins.erase(it);
	for (auto& [name, stored] : Context->Plugins)
		stored.Functions.OnPluginLoadStateChange(&plugin.Info, false);
	delete[] plugin.Info.Name;
	delete[] plugin.Info.RootFolderPath;
	return TC_RESULT_SUCCESS;
}

TCResult TC_Initialize(const void** outPluginAPI);
TCResult TC_OnPreShutdown();
TCResult TC_Shutdown();
void TC_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded);
void BindPluginFunctions(TCPluginFunctions* outPluginFunctions)
{
	if (!outPluginFunctions)
	{
		return;
	}
	outPluginFunctions->Initialize = TC_Initialize;
	outPluginFunctions->OnPreShutdown = TC_OnPreShutdown;
	outPluginFunctions->Shutdown = TC_Shutdown;
	outPluginFunctions->OnPluginLoadStateChange = TC_OnPluginLoadStateChange;
}
const char* TCORE_ACTIVE_PLUGIN_NAME = nullptr;
extern "C" __declspec(dllexport) TCResult TCORE_PLUGIN_ENTRY_FUNC(const ITC* core,
																  TCPluginInfo* outPluginInfo,
																  TCPluginFunctions* outPluginFunctions)
{
	if (!outPluginFunctions || !outPluginInfo)
	{
		return TC_RESULT_INVALID_ARGUMENT;
	}
	outPluginInfo->Name = TC_PLUGIN_NAME;
	outPluginInfo->Version = TC_PLUGIN_VERSION;
	outPluginInfo->RootFolderPath = nullptr;
	BindPluginFunctions(outPluginFunctions);
	TCORE_ACTIVE_PLUGIN_NAME = TC_PLUGIN_NAME;
	return TC_RESULT_SUCCESS;
}

TCResult TC_Initialize(const void** outPluginAPI)
{
	ITC* newTC = new ITC;
	Context = new TCContext();
	newTC->GetVersion = []() -> unsigned int {
		return TC_PLUGIN_VERSION;
	};
	newTC->LoadPlugin = LoadPlugin;
	newTC->UnloadPlugin = UnloadPlugin;
	newTC->GetPlugin = GetPlugin;
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

void TC_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded)
{
	// This plugin doesn't react to other plugins being loaded or unloaded, so this function is empty.
}