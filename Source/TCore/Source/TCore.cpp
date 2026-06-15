#include "TCore.h"

// External
#include <assert.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <dynalo/dynalo.hpp>  

TCORE_PLUGIN_INIT(TS, TC)

static const char* TCORE_ERROR_TEXT_DLL_NOT_FOUND   = "DLL file isn't found: %s\n";
static const char* TCORE_ERROR_TEXT_ENTRY_NOT_FOUND = "DLL file is found but plugin entry isn't\n";

struct TCContext{
  struct TCPluginStored{
    dynalo::native::handle NativeHandle;
    TCPluginInfo Info;
    TCPluginFunctions Functions;
  };
  std::unordered_map<std::string, TCPluginStored> Plugins;
};

static TCContext* TSContext = nullptr;

void StrCopy(char** dst, const char* src){
  size_t srclen = std::strlen(src);
  *dst = new char[srclen + 1];
  std::memcpy(*dst, src, srclen);
  (*dst)[srclen] = '\0';
}

TCResult LoadPlugin(const char* path, const TCPluginInfo** outInfo) {
  try{
    auto handle = dynalo::open(path);
    auto entryPoint = dynalo::get_function<void(const TC* core, TCPluginInfo* outPluginInfo, TCPluginFunctions* outPluginFunctions)>(handle, "TCORE_PLUGIN_ENTRY_FUNC");
    if (!entryPoint) {
      printf(TCORE_ERROR_TEXT_ENTRY_NOT_FOUND);
      return TC_RESULT_FAILURE;
    }

    TCPluginInfo info;
    TCPluginFunctions functions;
    entryPoint(TS, &info, &functions);

    StrCopy((char**)&info.Name, info.Name);
    StrCopy((char**)&info.RootFolderPath, info.RootFolderPath);
    info.Version = info.Version;

    TSContext->Plugins[path] = { handle, info, functions };
    *outInfo = &TSContext->Plugins[path].Info;
    return TC_RESULT_SUCCESS;
  }
  catch (std::exception& e) {
    printf(TCORE_ERROR_TEXT_DLL_NOT_FOUND, path);
    return TC_RESULT_FAILURE;
  }
}

TCResult UnloadPlugin(const char* pluginName) {
  auto it = TSContext->Plugins.find(pluginName);
  if (it == TSContext->Plugins.end()) {
    printf("Plugin isn't found: %s\n", pluginName);
    return TC_RESULT_FAILURE;
  }
  auto plugin = it->second;
  plugin.Functions.OnPreShutdown();
  plugin.Functions.Shutdown();
  dynalo::close(plugin.NativeHandle);

  TCPluginInfo info = plugin.Info;
  TSContext->Plugins.erase(it);
  for(auto& [name, stored] : TSContext->Plugins) 
    stored.Functions.OnPluginLoadStateChange(&plugin.Info, false);
  delete[] plugin.Info.Name;
  delete[] plugin.Info.RootFolderPath;
}

TCResult InitializeCore(const void** outPluginAPI){
    TC* newTC = new TC;
    TSContext = new TCContext();
    newTC->GetVersion = []() -> unsigned int {
        return TS_PLUGIN_VERSION;
    };
    newTC->LoadPlugin = LoadPlugin;
    newTC->UnloadPlugin = UnloadPlugin;
    TS = newTC;
    *outPluginAPI = TS;
    return TC_RESULT_SUCCESS;
}

TCResult OnPreShutdownCore() {
    for(auto& [name, stored] : TSContext->Plugins) 
      TS->UnloadPlugin(name.c_str());
    return TC_RESULT_SUCCESS;
}

TCResult ShutdownCore() {
    delete TSContext;
    delete TS;
    return TC_RESULT_SUCCESS;
}

void BindPluginFunctions(TCPluginFunctions* outPluginFunctions){
    outPluginFunctions->Initialize   = InitializeCore;
    outPluginFunctions->OnPluginLoadStateChange = [](const TCPluginInfo* pluginInfo, bool isLoaded){};
    outPluginFunctions->OnPreShutdown = OnPreShutdownCore;
    outPluginFunctions->Shutdown = ShutdownCore;
}

TCORE_PLUGIN_ENTRY_POINT_START(TS)
TCORE_PLUGIN_ENTRY_POINT_END()