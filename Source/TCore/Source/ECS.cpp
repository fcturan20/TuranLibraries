#define T_INCLUDE_PLATFORM_LIBS
#include "ECS.h"

#include <algorithm>
#include <cstring>
#include <dynalo/dynalo.hpp>
#include <string>

#include "stdint.h"
#include "stdio.h"
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
static unsigned int         pagesize;
static struct tlSuperBlock* mainMemBlock   = nullptr;
static const struct tlBuffer*     standard_alloc = nullptr;
static const tlVector*            f_vector       = nullptr;

//                  ECS INTERNAL STRUCTURES
///////////////////////////////////////////////////////////

struct ecs_pluginInfo {
  void* pluginDataPtr; // dlopen/LoadLibrary-compatible opaque handle
  char      PATH[MAX_PATHCHAR];
};

struct ecs_systemInfo {
  char     name[MAX_SYSTEMCHAR];
  uint32_t version = UINT32_MAX;
  const void*    ptr     = nullptr;
};

// This is main component type, so you shouldn't store (overriden) parent types with this
// Manager will return the (overriden) parent types with getComponentType anyway
struct ecs_compType {
  uint32_t                    typeID = UINT32_MAX;
  char                        name[MAX_COMPTYPECHAR];
  void*                       mainTypeHandle;
  tlComponentManagerDescription             manager;
  unsigned int                overridenTypeCount = 0;
  tlComponentTypePair* overridenTypePairs;
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
  struct tlComponentTypeID** compTypeHndlesList;
};

extern "C" struct tlEntity {
  uint32_t typeID;
  uint32_t entityID;
};

extern "C" struct tlComponentTypeID {
  uint32_t typeID           = UINT32_MAX;
  uint32_t padding_to_8byte = UINT32_MAX;
};
typedef struct tlComponentTypeID ecstapi_idOnlyPointer;

//                 INTERNAL SYSTEM
////////////////////////////////////////////////////////////

typedef struct tlECSPriv {
  // Plugins
  //////////////////////
  ecs_pluginInfo* v_pluginInfos;
  // Plugin unloading isn't supported for now!
  tlPlugin* create_pluginInfo(ecs_pluginInfo info) {
    uint32_t pluginCount = f_vector->size(v_pluginInfos);
#ifdef TURAN_DEBUGGING
    // Check if it's already added
    for (uint32_t i = 0; i < pluginCount; i++) {
      if (!std::strcmp(v_pluginInfos[i].PATH, info.PATH)) {
        printf("Plugin is already loaded, call reload!");
        return nullptr;
      }
      if (!v_pluginInfos[i].pluginDataPtr) {
        break;
      }
    }
#endif // TURAN_DEBUGGING

    f_vector->pushBack(v_pluginInfos, &info);
    return ( tlPlugin* )&v_pluginInfos[pluginCount];
  }
  ecs_pluginInfo* get_pluginInfo(tlPlugin* hnd) {
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
  const void*           findSystem(const char* name) {
    for (uint32_t i = 0; i < MAXSYSTEMCOUNT; i++) {
      if (v_systemInfos[i].ptr == nullptr) {
        return nullptr;
      }
      if (!std::strcmp(v_systemInfos[i].name, name)) {
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
  ecs_compType* find_compType_byID(struct tlComponentTypeID* id) {
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
  ecs_entityType* find_entityType_byHnd(struct tlEntityType* hnd) {
    ecstapi_idOnlyPointer hndle = *reinterpret_cast<ecstapi_idOnlyPointer*>(&hnd);
    ecs_entityType*       r     = &v_entityTypes[hndle.typeID];
#ifdef TURAN_DEBUGGING
    if (r->typeID != hndle.typeID) {
      r = nullptr;
    }
#endif
    return r;
  }
} tapi_ecs_d;

uint32_t find_overridenCompType(ecs_entityType* eType, tlComponentTypeID* base,
                                void** overriden) {
  for (uint32_t mainCompIndex = 0; mainCompIndex < eType->compCount; mainCompIndex++) {
    // If main type matches
    if (base == eType->compTypeHndlesList[mainCompIndex]) {
      *overriden = priv->find_compType_byID(base)->mainTypeHandle;
      return mainCompIndex;
    }
    ecs_compType* mainCompType =
      priv->find_compType_byID(eType->compTypeHndlesList[mainCompIndex]);
    for (uint32_t pairIndx = 0; pairIndx < mainCompType->overridenTypeCount; pairIndx++) {
      tlComponentTypePair& pair = mainCompType->overridenTypePairs[pairIndx];
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

typedef void (*ECSTAPIPLUGIN_loadfnc)(struct tlECS* ecsHnd, unsigned int reload);


//                          SYSTEM FUNCTIONS
////////////////////////////////////////////////////////////////////////

void addSystem(const char* name, unsigned int version, const void* system_ptr) {
  ecs_systemInfo sysInfo;
  unsigned int   maxcharlen = std::min<unsigned int>(std::strlen(name), MAX_SYSTEMCHAR - 1);
  std::memcpy(sysInfo.name, name, maxcharlen);
  sysInfo.name[maxcharlen] = 0;
  sysInfo.ptr              = system_ptr;
  sysInfo.version          = version;
  if (!priv->registerSystem(sysInfo)) {
    printf("A system with the same name is already registered, so this one failed!\n");
  }
}

void destroySystem(const void* systemPtr) { printf("Destroying a system isn't supported yet!\n"); }

const void* getSystem(const char* name) { return priv->findSystem(name); }

//                        COMPONENT FUNCTIONS
////////////////////////////////////////////////////////////////////////////

struct tlComponentTypeID* addComponentType(const char* name, void* mainType,
                                                  struct tlComponentManagerDescription            manager,
                                                  const tlComponentTypePair* pairList,
                                                  unsigned int                      pairListSize) {
  f_vector->pushBack(priv->v_mainComponentTypes, priv->v_mainComponentTypes);
  uint32_t      indx       = f_vector->size(priv->v_mainComponentTypes) - 1;
  ecs_compType* type       = &priv->v_mainComponentTypes[indx];
  type->mainTypeHandle     = mainType;
  type->manager            = manager;
  type->typeID             = indx;
  type->overridenTypeCount = pairListSize;
  type->overridenTypePairs = ( tlComponentTypePair* )standard_alloc->malloc(
    mainMemBlock, sizeof(tlComponentTypePair) * pairListSize);
  std::memcpy(type->overridenTypePairs, pairList, sizeof(tlComponentTypePair) * pairListSize);
  uint32_t namelen = std::strlen(name) + 1;
  uint32_t copylen = std::min<uint32_t>(namelen + 1, MAX_COMPTYPECHAR - 1);
  std::memcpy(type->name, name, copylen);
  for (uint32_t i = copylen; i < MAX_COMPTYPECHAR; i++) {
    type->name[i] = '\0';
  }
  ecstapi_idOnlyPointer idOnlyPointer;
  idOnlyPointer.typeID           = type->typeID;
  idOnlyPointer.padding_to_8byte = UINT32_MAX;
  return *reinterpret_cast<tlComponentTypeID**>(&idOnlyPointer);
}

//                          ENTITY FUNCTIONS
////////////////////////////////////////////////////////////////////////////

struct tlEntityType* addEntityType(
  const struct tlComponentTypeID* const* compTypeList, unsigned int listSize) {
  ecs_entityType type;
  type.compCount = listSize;
  type.typeID    = f_vector->size(priv->v_entityTypes);
  type.v_entityList =
    f_vector->create(mainMemBlock, listSize * sizeof(tlComponent*), 0, 1 << 20, 0);
  type.compTypeHndlesList = ( tlComponentTypeID** )standard_alloc->malloc(
    mainMemBlock, sizeof(tlComponentTypeID) * listSize);
  std::memcpy(type.compTypeHndlesList, compTypeList, sizeof(tlComponentTypeID) * listSize);

  f_vector->pushBack(priv->v_entityTypes, &type);
  ecstapi_idOnlyPointer hnd;
  hnd.padding_to_8byte = UINT32_MAX;
  hnd.typeID           = type.typeID;
  return *reinterpret_cast<struct tlEntityType**>(&hnd);
}
tlEntity* createEntity(struct tlEntityType* typeHandle) {
  ecs_entityType* eType = priv->find_entityType_byHnd(typeHandle);
  f_vector->pushBack(eType->v_entityList, nullptr);
  uint32_t             index        = f_vector->size(eType->v_entityList) - 1;
  tlComponent** compHndsList = reinterpret_cast<tlComponent**>(
    reinterpret_cast<uintptr_t>(eType->v_entityList) +
    (index * eType->compCount * sizeof(tlComponent*)));
  for (uint16_t compIndx = 0; compIndx < eType->compCount; compIndx++) {
    ecs_compType* compType = priv->find_compType_byID(eType->compTypeHndlesList[compIndx]);
    compHndsList[compIndx] = compType->manager.createComponent();
  }
  tlEntity fin_hnd;
  fin_hnd.entityID = index;
  fin_hnd.typeID   = eType->typeID;
  return *reinterpret_cast<tlEntity**>(&fin_hnd);
}
struct tlEntityType* findEntityType_byEntityHnd(tlEntity* entityHnd) {
  ecstapi_idOnlyPointer hnd;
  tlEntity       entity = *reinterpret_cast<tlEntity*>(&entityHnd);
#ifdef TURAN_DEBUGGING
  // If debugging, first access type
  // Then return ID of it
  // With this way, wrong accesses minimized
  ecs_entityType* type = &priv->v_entityTypes[entity.typeID];
  hnd.typeID           = type->typeID;
#else
  // If not debugging, just get type ID from entity handle
  hnd.typeID = entity.typeID;
#endif
  hnd.padding_to_8byte = UINT32_MAX;
  return *reinterpret_cast<struct tlEntityType**>(&hnd);
}
unsigned char doesContains_entityType(struct tlEntityType*      eTypeHnd,
                                      struct tlComponentTypeID* compType) {
  ecs_entityType* eType        = priv->find_entityType_byHnd(eTypeHnd);
  void*           empty        = nullptr;
  uint32_t        compTypeIndx = find_overridenCompType(eType, compType, &empty);
  if (compTypeIndx != UINT32_MAX) {
    return 1;
  }
  return 0;
}
tlComponent* get_component_byEntityHnd(tlEntity*                 entityHnd,
                                              struct tlComponentTypeID* compTypeID,
                                              void**                           overridenCompType) {
  struct tlEntityType* eTypeHnd = findEntityType_byEntityHnd(entityHnd);
  ecs_entityType*             eType    = priv->find_entityType_byHnd(eTypeHnd);
  // Find overriden component type
  uint32_t        compTypeIndx = find_overridenCompType(eType, compTypeID, overridenCompType);
  uintptr_t       eList        = reinterpret_cast<uintptr_t>(eType->v_entityList);
  tlEntity entity       = *reinterpret_cast<tlEntity*>(&entityHnd);
  return *reinterpret_cast<tlComponent**>(
    eList + (((entity.entityID * eType->compCount) + compTypeIndx) * sizeof(tlComponent*)));
}

extern "C" void* initializeAllocator();

// This is the entry point of the engine
TCResult InitializeECS(const void** outECSAPI) {
  typedef void* (*initializeAllocatorFnc)();
  initializeAllocatorFnc initializeAllocatorPtr = &initializeAllocator;
  if (!initializeAllocatorPtr) {
    printf("Allocator bootstrap function is not found: initializeAllocator\n");
    return TC_RESULT_FAILURE;
  }
  // Load virtual memory & allocator systems
  ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE allocSys =
    ( ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE )initializeAllocatorPtr();
  pagesize       = allocSys->virtualMemory->pageSize();
  f_vector       = allocSys->vectorManager;
  standard_alloc = allocSys->standard;

  mainMemBlock = allocSys->allocateSuperMemoryBlock(virmemAllocSize, L"" ECS_TAPI_NAME);

  TCEcs* ecs = ( TCEcs* )allocSys->endOfPage->malloc(mainMemBlock, sizeof(TCEcs));

  ecs->getSystem                  = getSystem;
  ecs->addSystem                  = addSystem;
  ecs->destroySystem              = destroySystem;
  ecs->addComponentType           = addComponentType;
  ecs->addEntityType              = addEntityType;
  ecs->createEntity               = createEntity;
  ecs->findEntityType_byEntityHnd = findEntityType_byEntityHnd;
  ecs->doesContains_entityType    = doesContains_entityType;
  ecs->get_component_byEntityHnd  = get_component_byEntityHnd;
  TSEcs = ecs;
  *outECSAPI = ecs;

  // Initialize ECS
  ////////////////////////
  priv->v_pluginInfos        = tlVectorCreate(ecs_pluginInfo, mainMemBlock, MAX_PLUGINCOUNT);
  priv->v_systemInfos        = tlVectorCreate(ecs_systemInfo, mainMemBlock, MAXSYSTEMCOUNT);
  priv->v_mainComponentTypes = tlVectorCreate(ecs_compType, mainMemBlock, MAX_COMPTYPECOUNT);
  priv->v_entityTypes = tlVectorCreate(ecs_entityType, mainMemBlock, MAX_ENTITYTYPECOUNT);

  addSystem(ALLOCATOR_TAPI_PLUGIN_NAME, ALLOCATOR_TAPI_PLUGIN_VERSION, allocSys);

  return ecs_funcs;
}

void OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, bool isLoaded) {
}

TCResult OnPreShutdown() {
  return TC_RESULT_SUCCESS;
}

void Shutdown(TCPluginHandle plugin) {
  
}

void FillPluginFunctions(TCPluginFunctions* outPluginFunctions) {
  outPluginFunctions->Initialize   = InitializeECS;
  outPluginFunctions->OnPluginLoadStateChange = OnPluginLoadStateChange;
  outPluginFunctions->OnPreShutdown = OnPreShutdown;
  outPluginFunctions->Shutdown = Shutdown;
}

TCORE_PLUGIN_ENTRY_POINT_START(TSEcs)
TCORE_PLUGIN_ENTRY_POINT_END()