#pragma once
#include "TCoreMacros.h"

TCORE_BEGIN_C_LINKAGE

#pragma region TCore Plugin

/* Instructions for creating a plugin:
1. In header; define plugin's API struct and its functions. Then, use TCORE_PLUGIN_DEFINE to define plugin's name, API struct and requested version.
2. In source; if there are any dependencies, import dependencies as:
    TCORE_PLUGIN_START_DEPENDENCY_IMPORTS()
    TCORE_PLUGIN_DEPEND(dependency1)
    TCORE_PLUGIN_DEPEND(dependency2)
    TCORE_PLUGIN_END_DEPENDENCY_IMPORTS()
3. In source; implement plugin's functions and add them to the API struct.
*/

#define TCORE_PLUGIN_DEFINE(name, display_name, struct_name, version) \
typedef struct struct_name struct_name; \
static constexpr unsigned int name##_PLUGIN_VERSION = version; \
static constexpr const char* const name##_PLUGIN_NAME = #display_name; \
extern const struct_name* name;

TCORE_PLUGIN_DEFINE(TS, "TCore", TC, TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

typedef struct TCPluginInfo{
  const char* Name;
  unsigned int Version;
  // UTF-8 string with null terminator, should be absolute path to the plugin file. This is used for reloading and unloading the plugin.
  const char* RootFolderPath;
} TCPluginInfo;

TCORE_DEFINE_HANDLE(TCPlugin);

typedef struct TCPluginFunctions {
    // This function is called when the plugin is loaded. The plugin should initialize its API struct and fill outPluginAPI with the pointer to the API struct
    TCResult (*Initialize)(const void** outPluginAPI);
    // This function is called when the plugin is beginning to unload, but before the plugin is actually unloaded.
    // The plugin should perform any necessary cleanup in this function, and return true if it is safe to unload the plugin.
    // If the plugin returns false, it will not be unloaded and will remain in a limbo state until it can be safely unloaded.
    TCResult (*OnPreShutdown)();
    // This function is called when the plugin is unloaded. The plugin should perform any necessary cleanup.
    void (*Shutdown)(TCPluginHandle plugin);
    // This function is called when another plugin is loaded or unloaded. This allows plugins to react to changes in the plugin ecosystem.
    // For example, a feature of the plugin can be disabled when a plugin it depends on is unloaded, and re-enabled when the plugin is loaded again.
    void (*OnPluginLoadStateChange)(const TCPluginInfo* pluginInfo, bool isLoaded);
} TCPluginFunctions;

typedef struct TC {
    // Returns the version of the TCore plugin system. This can be used to check for compatibility with plugins.
    // This allows plugins to adjust their behavior based on the version of TCore they are running on.
    unsigned int (*GetVersion)();
    TCPluginHandle (*LoadPlugin)(const TCPluginInfo* pluginInfo);
    void (*UnloadPlugin)(TCPluginHandle plugin);
    TCPluginHandle (*GetPlugin)(const char* pluginName, unsigned int version, TCPluginInfo* outPluginInfo, void** outPluginAPI);
} TC;

#pragma region Plugin Macros

// Define void FillPluginFunctions(TCPluginFunctions*) before calling this
#define TCORE_PLUGIN_ENTRY_POINT_START(name) \
  TCORE_FUN_EXPORT void TCORE_PLUGIN_ENTRY_FUNC(const TC* core, TCPluginInfo* outPluginInfo, TCPluginFunctions* outPluginFunctions) { \
    TS = core; \
    if (!core || !outPluginFunctions || !outPluginInfo) { \
        return; \
    } \
    outPluginInfo->Name = name##_PLUGIN_NAME; \
    outPluginInfo->Version = name##_PLUGIN_VERSION; \
    outPluginInfo->RootFolderPath = nullptr; \
    FillPluginFunctions(outPluginFunctions); \

// Hard dependencies are plugins that must be loaded for this plugin to work. If any of the hard dependencies are not loaded, this plugin will not be loaded.
// You don't have to use this macro if your plugin doesn't have any hard dependencies, or if you want to load the dependencies dynamically at runtime. But using this macro is recommended if you have hard dependencies, because it allows TCore to manage the dependencies for you and provide better error handling and user experience.
#define TCORE_PLUGIN_HARD_DEPENDENCY(name, version) \
{ \
    TCPluginInfo info{}; \
    TCPluginFunctions functions{}; \
    TS->GetPlugin(name##_PLUGIN_NAME, version, &info, &name); \
    if (!info.Name) { \
        printf("Failed to load plugin %s because it is not found!\n", name##_PLUGIN_NAME); \
        return TC_RESULT_FAILURE; \
    } \
}

#define TCORE_PLUGIN_ENTRY_POINT_END() }

TCORE_END_C_LINKAGE