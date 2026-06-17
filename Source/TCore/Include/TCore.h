#pragma once
#include "TCoreMacros.h"

TCORE_BEGIN_C_LINKAGE

#pragma region TCore Plugin

#define TCORE_PLUGIN_DEFINE(name, display_name, version)                    \
  typedef struct I##name #I##name;                                          \
  static constexpr unsigned int      name##_PLUGIN_VERSION = version;       \
  static constexpr const char* const name##_PLUGIN_NAME    = #display_name; \
  extern const I##name*              name;

// Use this once for every system variable you access in your code.
#define TCORE_PLUGIN_INIT(name) const I##name* name = nullptr;

TCORE_PLUGIN_DEFINE(TC, "TCore", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

typedef struct TCPluginInfo {
  const char*  Name;
  unsigned int Version;
  // UTF-8 string with null terminator, should be absolute path to the plugin file. This is used for
  // reloading and unloading the plugin.
  const char* RootFolderPath;
} TCPluginInfo;

typedef struct TCPluginFunctions {
  // This function is called when the plugin is loaded. The plugin should initialize its API struct
  // and fill outPluginAPI with the pointer to the API struct
  TCResult (*Initialize)(const void** outPluginAPI);
  // This function is called when the plugin is beginning to unload, but before the plugin is
  // actually unloaded. The plugin should perform any necessary cleanup in this function, and return
  // true if it is safe to unload the plugin. If the plugin returns false, it will not be unloaded
  // and will remain in a limbo state until it can be safely unloaded.
  TCResult (*OnPreShutdown)();
  // This function is called when the plugin is unloaded. The plugin should perform any necessary
  // cleanup.
  TCResult (*Shutdown)();
  // This function is called when another plugin is loaded or unloaded. This allows plugins to react
  // to changes in the plugin ecosystem. For example, a feature of the plugin can be disabled when a
  // plugin it depends on is unloaded, and re-enabled when the plugin is loaded again.
  void (*OnPluginLoadStateChange)(const TCPluginInfo* pluginInfo, bool isLoaded);
} TCPluginFunctions;

typedef struct ITC {
  // Returns the version of the TCore plugin system. This can be used to check for compatibility
  // with plugins. This allows plugins to adjust their behavior based on the version of TCore they
  // are running on.
  unsigned int (*GetVersion)();
  TCResult (*LoadPlugin)(const char* path, const TCPluginInfo** outPluginInfo);
  TCResult (*UnloadPlugin)(const char* pluginName);
  TCResult (*GetPlugin)(const char* pluginName, unsigned int version, TCPluginInfo* outPluginInfo,
                        void** outPluginAPI);
  TCResult (*GetCallerPluginInfo)(TCPluginInfo* outPluginInfo);
} ITC;

#pragma region Plugin Macros

/* Instructions for creating a plugin:
1. In header; define plugin's API struct and its functions. Then, use TCORE_PLUGIN_DEFINE to define
plugin's name, API struct and requested version.
2. In source; implement plugin's functions and add them to the API struct.
    2-1. You have to init TCore and your own plugin with TCORE_PLUGIN_INIT.
        For example, TCORE_PLUGIN_INIT(TS, TC) will init TCore plugin and
TCORE_PLUGIN_INIT(TSMyPlugin, TCMyPlugin) will init your plugin. 2-2. Define entry point of your
plugin and fill plugin functions. It's recommended to use TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START and
TCORE_PLUGIN_ENTRY_POINT_END for this, but you can also define your own entry point if you want to.
*/

// Define void BindPluginFunctions(TCPluginFunctions*) before calling this
#define TCORE_PLUGIN_ENTRY_POINT_START(name)                                                      \
  TCORE_FUN_EXPORT void TCORE_PLUGIN_ENTRY_FUNC(                                                  \
    const TCServices* core, TCPluginInfo* outPluginInfo, TCPluginFunctions* outPluginFunctions) { \
    TC = core;                                                                                    \
    if (!core || !outPluginFunctions || !outPluginInfo) {                                         \
      return;                                                                                     \
    }                                                                                             \
    outPluginInfo->Name           = name##_PLUGIN_NAME;                                           \
    outPluginInfo->Version        = name##_PLUGIN_VERSION;                                        \
    outPluginInfo->RootFolderPath = nullptr;                                                      \
    BindPluginFunctions(outPluginFunctions);

// Hard dependencies are plugins that must be loaded for this plugin to work. If any of the hard
// dependencies are not loaded, this plugin will not be loaded. You don't have to use this macro if
// your plugin doesn't have any hard dependencies, or if you want to load the dependencies
// dynamically at runtime. But using this macro is recommended if you have hard dependencies,
// because it allows TCore to manage the dependencies for you and provide better error handling and
// user experience.
#define TCORE_PLUGIN_HARD_DEPENDENCY(name, version)                                      \
  {                                                                                      \
    TCPluginInfo      info{};                                                            \
    TCPluginFunctions functions{};                                                       \
    TS->GetPlugin(name##_PLUGIN_NAME, version, &info, &name);                            \
    if (!info.Name) {                                                                    \
      printf("Failed to load plugin %s because it is not found!\n", name##_PLUGIN_NAME); \
      return TC_RESULT_FAILURE;                                                          \
    }                                                                                    \
  }

// If you don't want to write BindPluginFunctions, use this.
// You have to define functions in the form of name_FunctionName, for example, if your plugin is
// TSMyPlugin, you have to define functions like TSMyPlugin_Initialize, TSMyPlugin_Shutdown etc.
#define TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(name)                                      \
  TCResult name##_Initialize(const void** outPluginAPI);                                  \
  TCResult name##_OnPreShutdown();                                                        \
  TCResult name##_Shutdown();                                                             \
  void     name##_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, bool isLoaded); \
  void     BindPluginFunctions(TCPluginFunctions* outPluginFunctions) {                   \
    if (!outPluginFunctions) {                                                        \
      return;                                                                         \
    }                                                                                 \
    outPluginFunctions->Initialize              = name##_Initialize;                  \
    outPluginFunctions->OnPreShutdown           = name##_OnPreShutdown;               \
    outPluginFunctions->Shutdown                = name##_Shutdown;                    \
    outPluginFunctions->OnPluginLoadStateChange = name##_OnPluginLoadStateChange;     \
  }                                                                                       \
  TCORE_PLUGIN_ENTRY_POINT_START(name)

#define TCORE_PLUGIN_ENTRY_POINT_END() }

TCORE_END_C_LINKAGE