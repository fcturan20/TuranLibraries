// ECS

// Component Inheritance:
// A component is just a C struct. A component handle is just points to a component data.
// A component manager creates and destroys components of the same C struct.
// A component type is a pointer to component manager if you don't use component inheritance.
// Component Inheritance is a little bit more complex than C++ inheritance.
// Rule 1: A component can be derived from only a single component, not more.
// Rule 2: ComponentName should store whole inheritance list
//  Ex: comp C is derived from comp B, comp B is derived from comp A. comp C's name should be "C/B/A".
// Rule 3: Derived components doesn't have to store variables of the base component.
//  But because of this, component type handles should be used as a system for accessing variables (get-setters)
//  And derived comp should implement new comp types (systems) for the base comp type
//  @example
//    Base comp B stores Vec3 pos variable and reads it with BType->readPos()
//    Derived comp C doesn't store Vec3 pos because all comp Cs will be at pos(0,0,0)
//    So derived comp C will create new BType and will implement readPos as returning (0,0,0)
//    When user wants to access pos of comp C, it will get BType of comp C
//    Returned BType will store readPos func pointer as the func that returns (0,0,0)

#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include "predefinitions_tapi.h"
#define ECS_TAPI_NAME "ecs_tapi"
#define ECS_TAPI_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
//To help users to minimize accessing issues, each type name has its pointer
//With this way, users can't use an entityTypeID mistakenly as entityID etc.
typedef struct ecstapi_idOnlyPointer* entityTypeHnd_ecstapi;
typedef struct ecstapi_compType* compType_ecstapi;      // This should point to a component type defined by user
typedef struct ecstapi_compTypeID* compTypeID_ecstapi;  // This is an ID to find component type internally
typedef struct ecstapi_component* componentHnd_ecstapi;
typedef struct ecstapi_entity* entityHnd_ecstapi;
typedef struct ecstapi_plugin* pluginHnd_ecstapi;

// Use this to match base of a component type with a overriden one
typedef struct ecs_compTypePair {
  compTypeID_ecstapi base;
  compType_ecstapi overriden;
} ecs_compTypePair;

// Each component should handle its allocations in its own manager
// So there is no general componentManager for all components
// If a system will use a component; it should include the header that has component's type
typedef struct ecs_componentManager{
    componentHnd_ecstapi (*createComponent)();
    void  (*destroyComponent)(componentHnd_ecstapi hnd);
} componentManager_ecs;

typedef struct tapi_ecs_d ecs_d_tapi;
typedef struct tapi_ecs{
  // PLUGIN
  ////////////////////////////
  
  // Load a dll from the path and calls plugin initialization function to integrate
  pluginHnd_ecstapi(*loadPlugin)(const char* pluginPath);
  // Reload a dll from the previous path by its ID
  void (*reloadPlugin)(pluginHnd_ecstapi plugin);
  // Unload a dll (it's better not to use for reloading)
  unsigned char (*unloadPlugin)(pluginHnd_ecstapi plugin);

  // SYSTEM
  ////////////////////////////
  
  // Get the system registered by a plugin
  void* (*getSystem)(const char* name);
  // Make a system accessible from other systems
  void (*addSystem)(const char* name, unsigned int version, void* system_ptr);
  void (*destroySystem)(void* systemPTR);



  // COMPONENT
  ////////////////////////////

  // @param mainType: If main type can't be inherited, set NULL.
  // @return NULL if there is any component inheritance conflicts
  compTypeID_ecstapi (*addComponentType)(const char* name, compType_ecstapi mainType, componentManager_ecs manager, const ecs_compTypePair* pairList, unsigned int pairListSize);
  componentManager_ecs(*getCompManager)(compTypeID_ecstapi compType);

  // ENTITY
  ////////////////////////////

  entityTypeHnd_ecstapi(*addEntityType)(const compTypeID_ecstapi* compTypeList, unsigned int listSize);
  // Create an entity
  entityHnd_ecstapi(*createEntity)(entityTypeHnd_ecstapi typeHandle);
  // Find entity type handle
  entityTypeHnd_ecstapi(*findEntityType_byEntityHnd)(entityHnd_ecstapi entityHnd);
  //@return 1 if entity type contains the component type; otherwise 0
  unsigned char(*doesContains_entityType)(entityTypeHnd_ecstapi entityType, compTypeID_ecstapi compType);
  // Get a specific type of component of an entity
  // @param compTypeID: ID of the component type user wants to access
  // @param returnedCompType: Pointer to overriden component type, you should cast and use this to access data of the component
  // @return nullptr if there is no such component; otherwise valid pointer to use with new compTypeID
  // @example
  // compTypeID_ecstapi baseXXXCompTypeID = XXXCompManager->GetComponentTypeID();
  // compType_ecstapi overridenCompType;
  // compHnd_ecstapi compData = get_comp_byEntityHnd(firstEntity, baseXXXComponentType, &overridenCompType);
  // int ABCvarValue = ((XXX*)overridenCompType)->get_ABCvar(compData);
  // NOTE: Don't do ->    int ABCvarValue = ((XXX*)XXXCompManager->GetComponentType())->get_ABCvar(compData);
  //  because it will break inheritance
  componentHnd_ecstapi(*get_component_byEntityHnd)(entityHnd_ecstapi entityID, compTypeID_ecstapi compTypeID, compType_ecstapi* returnedCompType);

  ecs_d_tapi* data;
} ecs_tapi;

//Program that owns the ECS should use these types to convert loaded function pointer
typedef tapi_ecs* (*load_ecstapi_func)();
typedef unsigned char (*unload_ecstapi_func)();

//ECS TAPI MACROS

//Plugin should define this function to get initialized
#define ECSPLUGIN_ENTRY(ecsHndName, reloadFlagName) FUNC_DLIB_EXPORT void ECSTAPIPLUGIN_load(ecs_tapi* ecsHndName, unsigned int reloadFlagName)
#define ECSPLUGIN_EXIT(ecsHndName, reloadFlagName) FUNC_DLIB_EXPORT void ECSTAPIPLUGIN_unload(ecs_tapi* ecsHndName, unsigned int reloadFlagName)

#ifdef __cplusplus
}
#endif