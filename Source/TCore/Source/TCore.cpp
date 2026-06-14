#include "TCore.h"

#include <assert.h>
#include <stdio.h>

TCPluginHandle   LoadPlugin(const TCPluginInfo* info) {
  try{
    dynalo::library dll(pluginPath);
    ECSTAPIPLUGIN_loadfnc* dll_loader = dll.get_function<ECSTAPIPLUGIN_loadfnc>("ECSTAPIPLUGIN_load");
    if (!dll_loader || !(*dll_loader)) {
      printf(ECS_ERROR_TEXT_ENTRY_NOT_FOUND);
      return nullptr;
    }
    (*dll_loader)(ecs_funcs, false);

    ecs_pluginInfo info;
    uint32_t       pathLen    = std::strlen(pluginPath);
    int32_t        pathDif    = int32_t(pathLen) - MAX_PATHCHAR;
    uint32_t       maxcharlen = std::min<uint32_t>(std::strlen(pluginPath), MAX_PATHCHAR - 1);
    if (maxcharlen > MAX_PATHCHAR - 1) {
      printf("Plugin isn't loaded because it exceeds max char length of path\n");
      return nullptr;
    }

    std::memcpy(info.PATH, pluginPath + std::max(0, pathDif), maxcharlen);
    info.pluginDataPtr = dll.get_native_handle();
    return priv->create_pluginInfo(info);
  }
  catch (std::exception& e) {
    printf(ECS_ERROR_TEXT_DLL_NOT_FOUND, pluginPath);
    return nullptr;
  }
}

void UnloadPlugin(TCPluginHandle plugin) {
  assert(0 && "Unloading a plugin is not supported yet!\n");
}

TCResult InitializeCore(const void** outPluginAPI){
    TC* newTC = new TC;
    newTC->GetVersion = []() -> unsigned int {
        return TS_PLUGIN_VERSION;
    };
    newTC->LoadPlugin = LoadPlugin;
    newTC->UnloadPlugin = UnloadPlugin;
    TS = newTC;
    *outPluginAPI = TS;
    return TC_RESULT_SUCCESS;
}

void FillPluginFunctions(TCPluginFunctions* outPluginFunctions){
    outPluginFunctions->Initialize   = InitializeCore;
    outPluginFunctions->OnPluginLoadStateChange = OnPluginLoadStateChange;
    outPluginFunctions->OnPreShutdown = OnPreShutdown;
    outPluginFunctions->Shutdown = Shutdown;
}

TCORE_PLUGIN_ENTRY_POINT_START(TS)
TCORE_PLUGIN_ENTRY_POINT_END()