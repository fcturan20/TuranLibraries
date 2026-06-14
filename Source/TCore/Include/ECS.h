// ECS
#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

// Component Inheritance:
// A component is just a C struct. A component handle is just points to a component data.
// A component manager creates and destroys components of the same C struct.
// A component type is a pointer to component manager if you don't use component inheritance.
// Component Inheritance is a little bit more complex than C++ inheritance.
// Rule 1: A component can be derived from only a single component, not more.
// Rule 2: ComponentName should store whole inheritance list
//  Ex: comp C is derived from comp B, comp B is derived from comp A. comp C's name should be
//  "C/B/A".
// Rule 3: Derived components doesn't have to store variables of the base component.
//  But because of this, component type handles should be used as a system for accessing variables
//  (get-setters) And derived comp should implement new comp types (systems) for the base comp type
//  @example
//    Base comp B stores Vec3 pos variable and reads it with BType->readPos()
//    Derived comp C doesn't store Vec3 pos because all comp Cs will be at pos(0,0,0)
//    So derived comp C will create new BType and will implement readPos as returning (0,0,0)
//    When user wants to access pos of comp C, it will get BType of comp C
//    Returned BType will store readPos func pointer as the func that returns (0,0,0)

TCORE_PLUGIN_DEFINE(TSEcs, "tcEcs", TCEcs, TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

// To help users to minimize accessing issues, each type name has its pointer
// With this way, users can't use an entityTypeID mistakenly as entityID etc.
struct tcEntityType;
struct tcComponentTypeID; // Identifier for component types
struct tcComponent;
struct tcEntity;

// Use this to match base of a component type with a overriden one
struct tcComponentTypePair {
  struct tcComponentTypeID* base;
  void*                     overriden; // Pointer to overriden type
};

// Each component should handle its allocations in its own manager
// So there is no general componentManager for all components
// If a system will use a component; it should include the header that has component's type
struct tcComponentManagerDescription {
  struct tcComponent* (*createComponent)();
  unsigned char (*validateComponent)();
  void (*destroyComponent)(struct tcComponent* hnd);
};

typedef struct TCEcs {
  // SYSTEM
  ////////////////////////////

  // Get the system registered by a plugin
  const void* (*getSystem)(const char* name);
  // Make a system accessible from other systems
  void (*addSystem)(TCPlugin, const char* name, unsigned int version, const void* system_ptr);
  void (*destroySystem)(const void* systemPTR);

  // COMPONENT
  ////////////////////////////

  // @param mainType: If main type can't be inherited, set NULL.
  // @return NULL if there is any component inheritance conflicts
  struct tcComponentTypeID* (*addComponentType)(const char* name, void* mainType,
                                                struct tcComponentManagerDescription manager,
                                                const struct tcComponentTypePair*    pairList,
                                                unsigned int                         pairListSize);
  struct tcComponentManagerDescription (*getCompManager)(struct tcComponentTypeID* compType);

  // ENTITY
  ////////////////////////////

  struct tcEntityType* (*addEntityType)(const struct tcComponentTypeID* const* compTypeList,
                                        unsigned int                           listSize);
  // Create an entity
  struct tcEntity* (*createEntity)(struct tcEntityType* typeHandle);
  // Find entity type handle
  struct tcEntityType* (*findEntityType_byEntityHnd)(struct tcEntity* entityHnd);
  //@return 1 if entity type contains the component type; otherwise 0
  unsigned char (*doesContains_entityType)(struct tcEntityType*      entityType,
                                           struct tcComponentTypeID* compType);
  // Get a specific type of component of an entity
  // @param compTypeID: ID of the component type user wants to access
  // @param returnedCompType: Pointer to overriden component type, you should cast and use this to
  // access data of the component
  // @return nullptr if there is no such component; otherwise valid pointer to use with new
  // compTypeID
  // @example
  // compTypeID_ecstapi baseXXXCompTypeID = XXXCompManager->GetComponentTypeID();
  // compType_ecstapi overridenCompType;
  // compHnd_ecstapi compData = get_comp_byEntityHnd(firstEntity, baseXXXComponentType,
  // &overridenCompType); int ABCvarValue = ((XXX*)overridenCompType)->get_ABCvar(compData); NOTE:
  // Don't do ->    int ABCvarValue =
  // ((XXX*)XXXCompManager->GetComponentType())->get_ABCvar(compData);
  //  because it will break inheritance
  struct tcComponent* (*get_component_byEntityHnd)(struct tcEntity*          entityID,
                                                   struct tcComponentTypeID* compTypeID,
                                                   void**                    returnedCompType);
} TCEcs;

TCORE_END_C_LINKAGE