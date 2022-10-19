#define T_INCLUDE_PLATFORM_LIBS
#include "ecs_tapi.h"

#include "allocator_tapi.h"
#include "predefinitions_tapi.h"
#include "stdint.h"
#include "stdio.h"
#include "virtualmemorysys_tapi.h"
//                   COMPILE EXPRESSIONS
//////////////////////////////////////////////////////////
static constexpr uint32_t MAX_PATHCHAR          = 256;
static constexpr uint32_t MAX_PLUGINCOUNT       = 1 << 12;
static constexpr uint32_t MAXSYSTEMCOUNT        = 1 << 20;
static constexpr uint32_t MAX_ENTITYTYPECOUNT   = 1 << 24;
static constexpr uint32_t MAX_ENTITYCOUNT       = 1 << 24;
static constexpr uint32_t MAX_COMPTYPECOUNT     = 1 << 24;
static constexpr uint32_t MAX_COMPTYPEPAIRCOUNT = 1 << 26;
static constexpr uint32_t MAX_SYSTEMCHAR =
  32; // You should use null terminator too, so max 31 chars
static constexpr uint32_t MAX_COMPTYPECHAR =
  80; // You should use null terminator too, so max 79 chars
static constexpr uint64_t virmemAllocSize =
  uint64_t(1) << uint64_t(36); // Allocation size for plugin info and dlls

//                   STATICS
////////////////////////////////////////////////////////////////////

// Because this dll will be loaded once for each dll,
//  there is no need for a pointer
static struct tapi_ecs_d*     hidden;
static ecs_tapi*              ecs_funcs;
static unsigned int           pagesize;
static supermemoryblock_tapi* mainMemBlock   = nullptr;
static buffer_allocator_tapi* standard_alloc = nullptr;
static vector_allocator_tapi* f_vector       = nullptr;

//                 VIRTUAL MEMORY FUNCTIONS
///////////////////////////////////////////////////////////

extern "C" void*        virtual_reserve(unsigned long long size);
extern "C" void         virtual_commit(void* ptr, unsigned long long commitsize);
extern "C" void         virtual_decommit(void* ptr, unsigned long long size);
extern "C" void         virtual_free(void* ptr, unsigned long long commit);
extern "C" unsigned int get_pagesize();
extern "C" void*        get_virtualmemory(unsigned long long reserve_size,
                                          unsigned long long first_commit_size);
extern "C" void         load_virtualmemorysystem(VIRTUALMEMORY_TAPI_PLUGIN_TYPE*  virmemsysPTR,
                                                 ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE* allocatorsysPTR);

//                  ECS INTERNAL STRUCTURES
///////////////////////////////////////////////////////////

struct ecs_pluginInfo {
  HINSTANCE pluginDataPtr; // To call FreeLibrary while reload-destroying
  char      PATH[MAX_PATHCHAR];
};

struct ecs_systemInfo {
  char     name[MAX_SYSTEMCHAR];
  uint32_t version = UINT32_MAX;
  void*    ptr     = nullptr;
};

// This is main component type, so you shouldn't store (overriden) parent types with this
// Manager will return the (overriden) parent types with getComponentType anyway
struct ecs_compType {
  uint32_t             typeID = UINT32_MAX;
  char                 name[MAX_COMPTYPECHAR];
  compType_ecstapi     mainTypeHandle;
  ecs_componentManager manager;
  unsigned int         overridenTypeCount = 0;
  ecs_compTypePair*    overridenTypePairs;
};

struct ecs_entityType {
  uint32_t typeID    = UINT32_MAX;
  uint16_t compCount = 0;
  // Each entity stores pointers to actual component data
  // sizeof(entity) = compCount * componentHnd_ecstapi;
  // An entity ID = index in the vector
  // Each componentHnd should valid pointers;
  // if any of them is NULL, then entity is invalid
  void* v_entityList;

  // Each entity stores component handles as the same order as here
  // Allocate list elsewhere, because knowing entityType size will make searching over all entity
  // types faster (and access with ID too) If searches over all entity types are so rare, this could
  // be changed to allocating right after entityType List Size is compCount
  compTypeID_ecstapi* compTypeHndlesList;
};

extern "C" typedef struct ecstapi_entity {
  uint32_t typeID;
  uint32_t entityID;
} * entityHnd_ecstapi;

extern "C" typedef struct ecstapi_idOnlyPointer {
  uint32_t typeID;
  uint32_t padding_to_8byte;
} * entityTypeHnd_ecstapi;

//                 INTERNAL SYSTEM
////////////////////////////////////////////////////////////

typedef struct tapi_ecs_d {
  // Plugins
  //////////////////////
  ecs_pluginInfo* v_pluginInfos;
  // Plugin unloading isn't supported for now!
  pluginHnd_ecstapi create_pluginInfo(ecs_pluginInfo info) {
    uint32_t pluginCount = f_vector->size(v_pluginInfos);
#ifdef TURAN_DEBUGGING
    // Check if it's already added
    for (uint32_t i = 0; i < pluginCount; i++) {
      if (!strcmp(v_pluginInfos[i].PATH, info.PATH)) {
        printf("Plugin is already loaded, call reload!");
        return nullptr;
      }
      if (!v_pluginInfos[i].pluginDataPtr) {
        break;
      }
    }
#endif // TURAN_DEBUGGING

    f_vector->push_back(v_pluginInfos, &info);
    return ( pluginHnd_ecstapi )&v_pluginInfos[pluginCount];
  }
  ecs_pluginInfo* get_pluginInfo(pluginHnd_ecstapi hnd) {
#ifdef TURAN_DEBUGGING
    uintptr_t hnd_uptr = reinterpret_cast<uintptr_t>(hnd),
              v_uptr   = reinterpret_cast<uintptr_t>(v_pluginInfos);
    intptr_t offset    = hnd_uptr - v_uptr;
    if ((offset % sizeof(ecs_pluginInfo)) != 0 || offset < 0) {
      printf("Plugin handle is invalid!");
      return NULL;
    }
#endif
    return ( ecs_pluginInfo* )hnd;
  }

  // Systems
  //////////////////////
  ecs_systemInfo* v_systemInfos;
  void*           findSystem(const char* name) {
              for (uint32_t i = 0; i < MAXSYSTEMCOUNT; i++) {
                if (v_systemInfos[i].ptr == nullptr) {
                  return nullptr;
      }
                if (!strcmp(v_systemInfos[i].name, name)) {
                  return v_systemInfos[i].ptr;
      }
    }
              return nullptr;
  }
  unsigned char registerSystem(ecs_systemInfo info) {
#ifdef TURAN_DEBUGGING
    if (findSystem(info.name)) {
      return 0;
    }
#endif
    for (uint32_t i = 0; i < MAXSYSTEMCOUNT; i++) {
      ecs_systemInfo& currentSys = v_systemInfos[i];
      if (currentSys.ptr == nullptr) {
        currentSys = info;
        return 1;
      }
    }
  }

  // Components & Types
  ////////////////////////

  // These types are main types
  // Only childs are stored, overriden parents aren't
  ecs_compType* v_mainComponentTypes;
  ecs_compType* find_compType_byID(compTypeID_ecstapi id) {
    ecstapi_idOnlyPointer hndle = *reinterpret_cast<ecstapi_idOnlyPointer*>(&id);
    ecs_compType*         r     = &v_mainComponentTypes[hndle.typeID];
#ifdef TURAN_DEBUGGING
    if (r->typeID != hndle.typeID) {
      r = nullptr;
    }
#endif
    return r;
  }

  // Entities & Types
  // You can find an entity's type with its ID
  // Entity types store each component type as a pair of base-overriden component type pointers
  // This is the best for the performance and increases only a little bit of memory usage (otherwise
  // a recursive memory traversal is needed to find overriden component types)
  ///////////////////////

  ecs_entityType* v_entityTypes;
  ecs_entityType* find_entityType_byHnd(entityTypeHnd_ecstapi hnd) {
    ecstapi_idOnlyPointer hndle = *reinterpret_cast<ecstapi_idOnlyPointer*>(&hnd);
    ecs_entityType*       r     = &v_entityTypes[hndle.typeID];
#ifdef TURAN_DEBUGGING
    if (r->typeID != hndle.typeID) {
      r = nullptr;
    }
#endif
    return r;
  }
} ecs_d_tapi;

uint32_t find_overridenCompType(ecs_entityType* eType, compTypeID_ecstapi base,
                                compType_ecstapi* overriden) {
  for (uint32_t mainCompIndex = 0; mainCompIndex < eType->compCount; mainCompIndex++) {
    // If main type matches
    if (base == eType->compTypeHndlesList[mainCompIndex]) {
      *overriden = hidden->find_compType_byID(base)->mainTypeHandle;
      return mainCompIndex;
    }
    ecs_compType* mainCompType =
      hidden->find_compType_byID(eType->compTypeHndlesList[mainCompIndex]);
    for (uint32_t pairIndx = 0; pairIndx < mainCompType->overridenTypeCount; pairIndx++) {
      ecs_compTypePair& pair = mainCompType->overridenTypePairs[pairIndx];
      if (pair.base == base) {
        *overriden = pair.overriden;
        return mainCompIndex;
      }
    }
  }
  return UINT32_MAX;
}

//                        PLUGIN FUNCTIONS
////////////////////////////////////////////////////////////////////

typedef void (*ECSTAPIPLUGIN_loadfnc)(ecs_tapi* ecsHnd, unsigned int reload);
pluginHnd_ecstapi loadPlugin(const char* pluginPath) {
  auto dll = DLIB_LOAD_TAPI(pluginPath);
  if (!dll) {
    printf("dll file isn't found!");
    return nullptr;
  }
  ECSTAPIPLUGIN_loadfnc dll_loader =
    ( ECSTAPIPLUGIN_loadfnc )DLIB_FUNC_LOAD_TAPI(dll, "ECSTAPIPLUGIN_load");
  if (!dll_loader) {
    printf("dll is loaded but plugin entry isn't found!\n");
    return nullptr;
  }
  dll_loader(ecs_funcs, false);

  ecs_pluginInfo info;
  uint32_t       pathLen    = strlen(pluginPath);
  int32_t        pathDif    = int32_t(pathLen) - MAX_PATHCHAR;
  uint32_t       maxcharlen = min(strlen(pluginPath), MAX_PATHCHAR - 1);
  if (maxcharlen > MAX_PATHCHAR - 1) {
    printf("Plugin isn't loaded because it exceeds max char length of path\n");
    DLIB_UNLOAD_TAPI(dll);
    return nullptr;
  }

  memcpy(info.PATH, pluginPath + max(0, pathDif), maxcharlen);
  info.pluginDataPtr = dll;
  return hidden->create_pluginInfo(info);
}
void reloadPlugin(pluginHnd_ecstapi plugin) {
  ecs_pluginInfo* info = hidden->get_pluginInfo(plugin);
  if (!info) {
    return;
  }
  auto dll = DLIB_LOAD_TAPI(info->PATH);
  if (!dll) {
    printf("dll file isn't found!\n");
    return;
  }
  ECSTAPIPLUGIN_loadfnc dll_loader =
    ( ECSTAPIPLUGIN_loadfnc )DLIB_FUNC_LOAD_TAPI(dll, "ECSTAPIPLUGIN_load");
  if (!dll_loader) {
    printf("dll is loaded but plugin entry isn't found!\n");
    return;
  }
  dll_loader(ecs_funcs, true);
}
unsigned char unloadPlugin(pluginHnd_ecstapi plugin) {
  typedef void (*ECSTAPIPLUGIN_unloadfnc)(ecs_tapi * ecsHnd, unsigned int reload);
  printf("Unloading a plugin isn't supported yet!");

  return 0;
}

//                          SYSTEM FUNCTIONS
////////////////////////////////////////////////////////////////////////

void addSystem(const char* name, unsigned int version, void* system_ptr) {
  ecs_systemInfo sysInfo;
  unsigned int   maxcharlen = min(strlen(name), MAX_SYSTEMCHAR - 1);
  memcpy(sysInfo.name, name, maxcharlen);
  sysInfo.name[maxcharlen] = 0;
  sysInfo.ptr              = system_ptr;
  sysInfo.version          = version;
  if (!hidden->registerSystem(sysInfo)) {
    printf("A system with the same name is already registered, so this one failed!\n");
  }
}

void destroySystem(void* systemPtr) { printf("Destroying a system isn't supported yet!\n"); }

void* getSystem(const char* name) { return hidden->findSystem(name); }

//                        COMPONENT FUNCTIONS
////////////////////////////////////////////////////////////////////////////

compTypeID_ecstapi addComponentType(const char* name, compType_ecstapi mainType,
                                    componentManager_ecs manager, const ecs_compTypePair* pairList,
                                    unsigned int pairListSize) {
  f_vector->push_back(hidden->v_mainComponentTypes, hidden->v_mainComponentTypes);
  uint32_t      indx       = f_vector->size(hidden->v_mainComponentTypes) - 1;
  ecs_compType* type       = &hidden->v_mainComponentTypes[indx];
  type->mainTypeHandle     = mainType;
  type->manager            = manager;
  type->typeID             = indx;
  type->overridenTypeCount = pairListSize;
  type->overridenTypePairs = ( ecs_compTypePair* )standard_alloc->malloc(
    mainMemBlock, sizeof(ecs_compTypePair) * pairListSize);
  memcpy(type->overridenTypePairs, pairList, sizeof(ecs_compTypePair) * pairListSize);
  uint32_t namelen = strlen(name) + 1;
  uint32_t copylen = min(namelen + 1, MAX_COMPTYPECHAR - 1);
  memcpy(type->name, name, copylen);
  for (uint32_t i = copylen; i < MAX_COMPTYPECHAR; i++) {
    type->name[i] = '\0';
  }
  ecstapi_idOnlyPointer idOnlyPointer;
  idOnlyPointer.typeID           = type->typeID;
  idOnlyPointer.padding_to_8byte = UINT32_MAX;
  return *reinterpret_cast<compTypeID_ecstapi*>(&idOnlyPointer);
}

//                          ENTITY FUNCTIONS
////////////////////////////////////////////////////////////////////////////

entityTypeHnd_ecstapi addEntityType(const compTypeID_ecstapi* compTypeList, unsigned int listSize) {
  ecs_entityType type;
  type.compCount = listSize;
  type.typeID    = f_vector->size(hidden->v_entityTypes);
  type.v_entityList =
    f_vector->create_vector(listSize * sizeof(componentHnd_ecstapi), mainMemBlock, 0, 1 << 20, 0);
  type.compTypeHndlesList = ( compTypeID_ecstapi* )standard_alloc->malloc(
    mainMemBlock, sizeof(compTypeID_ecstapi) * listSize);
  memcpy(type.compTypeHndlesList, compTypeList, sizeof(compTypeID_ecstapi) * listSize);

  f_vector->push_back(hidden->v_entityTypes, &type);
  ecstapi_idOnlyPointer hnd;
  hnd.padding_to_8byte = UINT32_MAX;
  hnd.typeID           = type.typeID;
  return *reinterpret_cast<entityTypeHnd_ecstapi*>(&hnd);
}
entityHnd_ecstapi createEntity(entityTypeHnd_ecstapi typeHandle) {
  ecs_entityType* eType = hidden->find_entityType_byHnd(typeHandle);
  f_vector->push_back(eType->v_entityList, nullptr);
  uint32_t              index        = f_vector->size(eType->v_entityList) - 1;
  componentHnd_ecstapi* compHndsList = reinterpret_cast<componentHnd_ecstapi*>(
    reinterpret_cast<uintptr_t>(eType->v_entityList) +
    (index * eType->compCount * sizeof(componentHnd_ecstapi)));
  for (uint16_t compIndx = 0; compIndx < eType->compCount; compIndx++) {
    ecs_compType* compType = hidden->find_compType_byID(eType->compTypeHndlesList[compIndx]);
    compHndsList[compIndx] = compType->manager.createComponent();
  }
  ecstapi_entity fin_hnd;
  fin_hnd.entityID = index;
  fin_hnd.typeID   = eType->typeID;
  return *reinterpret_cast<entityHnd_ecstapi*>(&fin_hnd);
}
entityTypeHnd_ecstapi findEntityType_byEntityHnd(entityHnd_ecstapi entityHnd) {
  ecstapi_idOnlyPointer hnd;
  ecstapi_entity        entity = *reinterpret_cast<ecstapi_entity*>(&entityHnd);
#ifdef TURAN_DEBUGGING
  // If debugging, first access type
  // Then return ID of it
  // With this way, wrong accesses minimized
  ecs_entityType* type = &hidden->v_entityTypes[entity.typeID];
  hnd.typeID           = type->typeID;
#else
  // If not debugging, just get type ID from entity handle
  hnd.typeID = entity.typeID;
#endif
  hnd.padding_to_8byte = UINT32_MAX;
  return *reinterpret_cast<entityTypeHnd_ecstapi*>(&hnd);
}
unsigned char doesContains_entityType(entityTypeHnd_ecstapi eTypeHnd, compTypeID_ecstapi compType) {
  ecs_entityType*  eType = hidden->find_entityType_byHnd(eTypeHnd);
  compType_ecstapi empty;
  uint32_t         compTypeIndx = find_overridenCompType(eType, compType, &empty);
  if (compTypeIndx != UINT32_MAX) {
    return 1;
  }
  return 0;
}
componentHnd_ecstapi get_component_byEntityHnd(entityHnd_ecstapi  entityHnd,
                                               compTypeID_ecstapi compTypeID,
                                               compType_ecstapi*  overridenCompType) {
  entityTypeHnd_ecstapi eTypeHnd = findEntityType_byEntityHnd(entityHnd);
  ecs_entityType*       eType    = hidden->find_entityType_byHnd(eTypeHnd);
  // Find overriden component type
  uint32_t       compTypeIndx = find_overridenCompType(eType, compTypeID, overridenCompType);
  uintptr_t      eList        = reinterpret_cast<uintptr_t>(eType->v_entityList);
  ecstapi_entity entity       = *reinterpret_cast<ecstapi_entity*>(&entityHnd);
  return *reinterpret_cast<componentHnd_ecstapi*>(
    eList + (((entity.entityID * eType->compCount) + compTypeIndx) * sizeof(componentHnd_ecstapi)));
}

// This is the entry point of the engine
extern "C" FUNC_DLIB_EXPORT ecs_tapi* load_ecstapi() {
  // Load virtual memory & allocator systems
  VIRTUALMEMORY_TAPI_PLUGIN_TYPE  virmemsys    = nullptr;
  ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE allocatorsys = nullptr;
  load_virtualmemorysystem(&virmemsys, &allocatorsys);
  f_vector       = allocatorsys->vector_manager;
  pagesize       = get_pagesize();
  standard_alloc = allocatorsys->standard;

  mainMemBlock = allocatorsys->createSuperMemoryBlock(virmemAllocSize, ECS_TAPI_NAME);

  ecs_funcs = ( ecs_tapi* )allocatorsys->end_of_page->malloc(mainMemBlock,
                                                             sizeof(ecs_tapi) + sizeof(ecs_d_tapi));

  hidden                                = ( ecs_d_tapi* )(ecs_funcs + 1);
  *hidden                               = ecs_d_tapi();
  ecs_funcs->loadPlugin                 = loadPlugin;
  ecs_funcs->reloadPlugin               = reloadPlugin;
  ecs_funcs->unloadPlugin               = unloadPlugin;
  ecs_funcs->getSystem                  = getSystem;
  ecs_funcs->addSystem                  = addSystem;
  ecs_funcs->destroySystem              = destroySystem;
  ecs_funcs->addComponentType           = addComponentType;
  ecs_funcs->addEntityType              = addEntityType;
  ecs_funcs->createEntity               = createEntity;
  ecs_funcs->findEntityType_byEntityHnd = findEntityType_byEntityHnd;
  ecs_funcs->doesContains_entityType    = doesContains_entityType;
  ecs_funcs->get_component_byEntityHnd  = get_component_byEntityHnd;
  ecs_funcs->data                       = hidden;

  // Initialize ECS
  ////////////////////////
  hidden->v_pluginInfos        = TAPI_VECTOR(ecs_pluginInfo, MAX_PLUGINCOUNT);
  hidden->v_systemInfos        = TAPI_VECTOR(ecs_systemInfo, MAXSYSTEMCOUNT);
  hidden->v_mainComponentTypes = TAPI_VECTOR(ecs_compType, MAX_COMPTYPECOUNT);
  hidden->v_entityTypes        = TAPI_VECTOR(ecs_entityType, MAX_ENTITYTYPECOUNT);

  addSystem(VIRTUALMEMORY_TAPI_PLUGIN_NAME, VIRTUALMEMORY_TAPI_PLUGIN_VERSION, virmemsys);
  addSystem(ALLOCATOR_TAPI_PLUGIN_NAME, ALLOCATOR_TAPI_PLUGIN_VERSION, allocatorsys);

  return ecs_funcs;
}

extern "C" FUNC_DLIB_EXPORT unsigned char unload_ecstapi() { return 0; }