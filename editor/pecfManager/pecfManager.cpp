#include "pecfManager.h"

#include <stdint.h>

#include "ecs_tapi.h"
#include "main.h"
#include "predefinitions_tapi.h"
#include "stdio.h"

//  Because both engine and TuranLibraries don't know about the editor
//   primitiveTypes of their systems should be added by default here
//  Adding a pecfType should be in a pecfManagerPlugin, but there is no need to add a plugin for
//  them
//   because editor is already built upon TuranEngine.

// CompHnd_pecf should be cast to this type
struct compID {
  unsigned int typeID, compID;
};
static_assert(sizeof(compID) <= sizeof(void*), "sizeof compID can't be larger than pointer size!");

struct idOnlyHnd {
  unsigned int ID = 0, padding_8bytes = UINT32_MAX;
};
struct funcInfo {
  pecf_funcProperties props;
};

struct pecf_manager_private {
  pecf_funcProperties* v_funcs;
  // Plugin unloading isn't supported for now!
  pecf_funcProperties create_funcInfo(pecf_funcProperties info) {
    /*
    uint32_t pluginCount = f_vector->size(v_pluginInfos);
#ifdef TURAN_DEBUGGING
    //Check if it's already added
    for (uint32_t i = 0; i < pluginCount; i++) {
      if (!strcmp(v_pluginInfos[i].PATH, info.PATH)) { printf("Plugin is already loaded, call
reload!"); return nullptr; } if (!v_pluginInfos[i].pluginDataPtr) { break; }
    }
#endif // TURAN_DEBUGGING

    f_vector->push_back(v_pluginInfos, &info);
    return (pluginHnd_ecstapi)&v_pluginInfos[pluginCount];
  }
  pecf_funcProperties* get_pluginInfo(funcHnd_pecf hnd) {
#ifdef TURAN_DEBUGGING
    uintptr_t hnd_uptr = reinterpret_cast<uintptr_t>(hnd), v_uptr =
reinterpret_cast<uintptr_t>(v_pluginInfos); intptr_t offset = hnd_uptr - v_uptr; if ((offset %
sizeof(ecs_pluginInfo)) != 0 || offset < 0) { printf("Plugin handle is invalid!"); return NULL; }
#endif
    return (ecs_pluginInfo*)hnd;
  */}
};
static pecf_manager_private hidden;
static pecf_manager*        mngr;

struct entityHnd {
  unsigned int entityID, padding_to_8bytes;
};
static_assert(sizeof(entityHnd) <= sizeof(entityHnd_pecf),
              "EntityHnd can't be larger than pointer size");

static defaultComp_pecf compType;
// static compType_ecstapi defaultCompType = (compType_ecstapi)&compType;
static compTypeID_ecstapi defaultCompTypeID;

funcTypeHnd_pecf add_funcType(funcProperties_pecf props, primitiveVariable_pecf* inputArgs,
                              unsigned int inputArgsCount, primitiveVariable_pecf* outputArgs,
                              unsigned int outputArgsCount) {
  return nullptr;
}

//        PRIMITIVE FUNCTIONs
/////////////////////////////////////////////

primTypeHnd_pecf add_primitiveType(const char* primName, unsigned int primDataSize,
                                   const primitiveVariable_pecf* varList, unsigned int varCount) {
  return nullptr;
}
// To display a component's variables, a window needs these informations
// But this doesn't help in programming side, because primitive variable should be created in
// proper C struct
void get_primitiveTypeInfo(primTypeHnd_pecf hnd, const char** primName, unsigned int* primDataSize,
                           primitiveVariable_pecf** varList, unsigned int* varCount) {}

//        COMPONENT FUNCTIONs
/////////////////////////////////////////////

compTypeHnd_pecf add_componentType(componentManagerInfo_pecf mngr) { return nullptr; }
void             get_componentPrimInfo(compTypeHnd_pecf comp, unsigned int index,
                                       primitiveVariable_pecf* primInfo) {}
void             get_componentPrimData(compHnd_pecf comp, unsigned int index, void* dst) {}
void             set_componentPrimData(compHnd_pecf comp, unsigned int index, const void* src) {}

//          ENTITY FUNCTIONs
////////////////////////////////////////////

entityHnd_pecf add_entity(const char* entityTypeName) {
  // Create an entity with default PECF component
  return nullptr;
}
// @return 1 if succeeds, 0 if fails
compHnd_pecf add_componentToEntity(entityHnd_pecf entity, compTypeHnd_pecf compType) {
  // Unlike ECS entities, PECF entities are unique so there is no PECF Entity Type
  // Also that means each componentHnd of an entity should be stored by user too

  // Create new entity type with the extra component type
  // Create new entity from the type
  // Copy all components to new entity
  // Fix default PECF component to point to new entityHnd
  return 0;
}
const compHnd_pecf* get_componentsOfEntity(entityHnd_pecf entity, unsigned int* compListSize) {
  return nullptr;
}

//      CALLBACK REGISTRATIONS
/////////////////////////////////////

unsigned char regOnChanged_entity(entity_onChangedFunc callback, entityHnd_ecstapi entity,
                                  unsigned char reg_orUnreg) {
  return 0;
}
unsigned char regOnChanged_comp(comp_onChangedFunc callback, compHnd_pecf comp,
                                unsigned char reg_orUnreg) {
  return 0;
}

void setFuncPtrs_pecf() {
  /*
  mngr->add_funcType = add_funcType;

  mngr->add_primitiveType = add_primitiveType;
  mngr->get_primitiveTypeInfo = get_primitiveTypeInfo;

  mngr->add_componentType = add_componentType;
  mngr->get_componentPrimInfo = get_componentPrimInfo;
  mngr->get_componentPrimData = get_componentPrimData;
  mngr->set_componentPrimData = set_componentPrimData;

  mngr->add_entity = add_entity;
  mngr->add_componentToEntity = add_componentToEntity;
  mngr->get_componentsOfEntity = get_componentsOfEntity;

  mngr->regOnChanged_comp = regOnChanged_comp;
  mngr->regOnChanged_entity = regOnChanged_entity;
  */
}
void initialize_pecfManager() {
  mngr = new pecf_manager;
  setFuncPtrs_pecf();
  // Now other plugins can access PECF Manager using editor ECS
  editorECS->addSystem(PECF_MANAGER_PLUGIN_NAME, PECF_MANAGER_PLUGIN_VERSION, mngr);
}