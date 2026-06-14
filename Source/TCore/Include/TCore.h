#pragma once
#include "TCoreMacros.h"

TCORE_BEGIN_C_LINKAGE

#pragma region Plugin Macros

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
static constexpr unsigned int name##_PLUGIN_REQUESTED_VERSION = version; \
static const char* const name##_PLUGIN_NAME = #display_name; \
static const struct_name* name = nullptr;

#define TCORE_PLUGIN_DEPEND(name) \
{ \
    ecsSys->getSystem(name##_PLUGIN_NAME, &name); \
    if (!name) { \
        printf("Failed to load plugin %s because it is not found!\n", name##_PLUGIN_NAME); \
        return; \
    } \
}
#define TCORE_PLUGIN_START_DEPENDENCY_IMPORTS() \
  TCORE_FUN_EXPORT void load_dependencies(struct tcECS* ecsSys) { 
#define TCORE_PLUGIN_END_DEPENDENCY_IMPORTS() }

#pragma region TCore Plugin

TCORE_PLUGIN_DEFINE(TS, "TCore", TC, TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

typedef struct TCPluginInfo{
  const char* Name;
  unsigned int Version;
  // UTF-8 string with null terminator, should be absolute path to the plugin file. This is used for reloading and unloading the plugin.
  const char* RootFolderPath;
} TCPluginInfo;

TCORE_DEFINE_HANDLE(TCPlugin);

typedef struct TCPluginFunctions {
    
} TCPluginFunctions;

typedef struct TC {
    TCPluginHandle (*LoadPlugin)(const TCPluginInfo* pluginInfo);
    void (*UnloadPlugin)(TCPluginHandle plugin);
    TCPluginHandle (*GetPlugin)(const char* pluginName, unsigned int version, TCPluginInfo* outPluginInfo);
} TC;
TCORE_END_C_LINKAGE