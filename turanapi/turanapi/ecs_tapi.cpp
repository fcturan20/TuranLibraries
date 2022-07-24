#define TAPI_INCLUDE_PLATFORM_LIBS
#include "predefinitions_tapi.h"
#include "virtualmemorysys_tapi.h"
#include "allocator_tapi.h"
#include "stdio.h"
#include "stdint.h"
#include "ecs_tapi.h"
//                   COMPILE EXPRESSIONS
//////////////////////////////////////////////////////////
static constexpr uint32_t MAXPATHCHAR = 256;
static constexpr uint32_t MAXPLUGINCOUNT = 1 << 12;
static constexpr uint32_t MAXSYSTEMCOUNT = 1 << 20;
static constexpr uint32_t MAXENTITYTYPECOUNT = 1 << 24;
static constexpr uint32_t MAXCOMPTYPECOUNT = 1 << 24;
static constexpr uint32_t MAXSYSTEMCHAR = 32;  // You should use null terminator too, so max 31 chars
static constexpr uint32_t virmemAllocSize = 1 << 30;  // Allocation size for plugin info and dlls

//                 VIRTUAL MEMORY FUNCTIONS
///////////////////////////////////////////////////////////


extern "C" void* virtual_reserve(unsigned long long size);
extern "C" void virtual_commit(void* ptr, unsigned long long commitsize);
extern "C" void virtual_decommit(void* ptr, unsigned long long size);
extern "C" void virtual_free(void* ptr, unsigned long long commit);
extern "C" unsigned int get_pagesize();
extern "C" void* get_virtualmemory(unsigned long long reserve_size, unsigned long long first_commit_size);
extern "C" void load_virtualmemorysystem(VIRTUALMEMORY_TAPI_PLUGIN_TYPE * virmemsysPTR, ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE * allocatorsysPTR);


//                  ECS INTERNAL STRUCTURES
///////////////////////////////////////////////////////////


struct ecs_pluginInfo {
  HINSTANCE pluginDataPtr;        // To call FreeLibrary while reload-destroying
  char PATH[MAXPATHCHAR];
};

struct ecs_systemInfo {
  char name[MAXSYSTEMCHAR];
  uint32_t version = UINT32_MAX;
  void* ptr = nullptr;
};


struct ecs_entity {
  uint64_t ID = UINT64_MAX;
  uint64_t typeID = UINT64_MAX;
};

struct ecs_componentType {
  uint64_t typeID = UINT64_MAX;
  ecs_componentManager manager;
};

struct ecs_entityType {
  uint64_t typeID = UINT64_MAX;
  uint32_t compCount = 0;
  void** componentTypeListPTR;    //Allocate them in elsewhere, 
  //because knowing entityType size will lead to faster lookup
};




//                            STATICS
////////////////////////////////////////////////////////////////////

//Because this dll will be loaded once for each dll, 
// there is no need for a pointer
static ecs_d* hidden;
static ecs_tapi* ecs_funcs;
static unsigned int pagesize = get_pagesize();
static supermemoryblock_tapi* mainMemBlock = nullptr;
static vector_allocator_tapi* f_vector = nullptr;



//                 INTERNAL SYSTEM
////////////////////////////////////////////////////////////

struct ecs_d {
  // Plugins
  //////////////////////
  ecs_pluginInfo* v_pluginInfos;
  //Plugin unloading isn't supported for now!
  pluginHnd_ecstapi create_pluginInfo(ecs_pluginInfo info) {
    uint32_t pluginCount = f_vector->size(v_pluginInfos);
#ifdef TURAN_DEBUGGING
    //Check if it's already added
    for (uint32_t i = 0; i < pluginCount; i++) {
      if (!strcmp(v_pluginInfos[i].PATH, info.PATH)) { printf("Plugin is already loaded, call reload!"); return nullptr; }
      if (!v_pluginInfos[i].pluginDataPtr) { break; }
    }
#endif // TURAN_DEBUGGING

    f_vector->push_back(v_pluginInfos, &info);
    return (pluginHnd_ecstapi)&v_pluginInfos[pluginCount];
  }
  ecs_pluginInfo* get_pluginInfo(pluginHnd_ecstapi hnd) {
#ifdef TURAN_DEBUGGING
    uintptr_t hnd_uptr = reinterpret_cast<uintptr_t>(hnd), v_uptr = reinterpret_cast<uintptr_t>(v_pluginInfos);
    intptr_t offset = hnd_uptr - v_uptr;
    if ((offset % sizeof(ecs_pluginInfo)) != 0 || offset < 0) { printf("Plugin handle is invalid!"); return NULL;}
#endif
    return (ecs_pluginInfo*)hnd;
  }

  // Systems
  //////////////////////
  ecs_systemInfo* v_systemInfos;
  void* findSystem(const char* name) {
    for (uint32_t i = 0; i < MAXSYSTEMCOUNT; i++) {
      if (v_systemInfos[i].ptr == nullptr) { return nullptr; }
      if (!strcmp(v_systemInfos[i].name, name)) {
        return v_systemInfos[i].ptr;
      }
    }
    return nullptr;
  }
  unsigned char registerSystem(ecs_systemInfo info) {
#ifdef TURAN_DEBUGGING
    if (findSystem(info.name)) { return 0; }
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

  //These types are main types (most child)
  ecs_componentType* v_mainComponentTypes;
  


  // Entities & Types
  // You can find an entity's type with its ID
  // Entity types store each component type as a pair of base-overriden component type pointers
  // This is best for the performance and increases only a little bit of memory usage (otherwise a recursive memory traversal is needed to find overriden component types)
  ///////////////////////

  ecs_entityType* v_entityTypes;

};




//                        FUNCTIONS
////////////////////////////////////////////////////////////////////

typedef void (*ECSTAPIPLUGIN_loadfnc)(ecs_tapi* ecsHnd, unsigned int reload);
pluginHnd_ecstapi loadPlugin(const char* pluginPath){
  auto dll = DLIB_LOAD_TAPI(pluginPath);
  if (!dll) { printf("dll file isn't found!"); return nullptr; }
  ECSTAPIPLUGIN_loadfnc dll_loader = (ECSTAPIPLUGIN_loadfnc)DLIB_FUNC_LOAD_TAPI(dll, "ECSTAPIPLUGIN_load");
  if (!dll_loader) { printf("dll is loaded but plugin entry isn't found!\n"); return nullptr; }
  dll_loader(ecs_funcs, false);

  ecs_pluginInfo info;
  uint32_t pathLen = strlen(pluginPath);
  int32_t pathDif = int32_t(pathLen) - MAXPATHCHAR;
  uint32_t maxcharlen = min(strlen(pluginPath), MAXPATHCHAR - 1);
  if (maxcharlen > MAXPATHCHAR - 1) {
    printf("Plugin isn't loaded because it exceeds max char length of path\n"); 
    DLIB_UNLOAD_TAPI(dll); return nullptr;
  }

  memcpy(info.PATH, pluginPath + max(0, pathDif), maxcharlen);
  info.pluginDataPtr = dll;
  return hidden->create_pluginInfo(info);
}
void reloadPlugin(pluginHnd_ecstapi plugin){
  ecs_pluginInfo* info = hidden->get_pluginInfo(plugin);
  if (!info) { return; }
  auto dll = DLIB_LOAD_TAPI(info->PATH);
  if (!dll) { printf("dll file isn't found!\n"); return; }
  ECSTAPIPLUGIN_loadfnc dll_loader = (ECSTAPIPLUGIN_loadfnc)DLIB_FUNC_LOAD_TAPI(dll, "ECSTAPIPLUGIN_load");
  if (!dll_loader) { printf("dll is loaded but plugin entry isn't found!\n"); return; }
  dll_loader(ecs_funcs, true);
}
unsigned char unloadPlugin(pluginHnd_ecstapi plugin){
  typedef void (*ECSTAPIPLUGIN_unloadfnc)(ecs_tapi* ecsHnd, unsigned int reload);
  printf("Unloading a plugin isn't supported yet!");

  return 0;
}
void* getSystem(const char* name){
  return hidden->findSystem(name);
}
componentHnd_ecstapi get_component_byEntityHnd(entityHnd_ecstapi entityID, compTypeHnd_ecstapi* compType){
  // Find entity type
  // Search components of the entity type
  // For each component type, search base components to find the given compType
  // If givenCompType is found, set the given pointer to the one derived class gives
  return nullptr;
}


void addSystem(const char* name, unsigned int version, void* system_ptr){
  ecs_systemInfo sysInfo;
  unsigned int maxcharlen = min(strlen(name), MAXSYSTEMCHAR - 1);
  memcpy(sysInfo.name, name, maxcharlen);
  sysInfo.name[maxcharlen] = 0;
  sysInfo.ptr = system_ptr;
  sysInfo.version = version;
  if (!hidden->registerSystem(sysInfo)) {
    printf("A system with the same name is already registered, so this one failed!\n");
  }
}

void destroySystem(void* systemPtr){
  printf("Destroying a system isn't supported yet!\n");
}



//This is the entry point of the engine
extern "C" FUNC_DLIB_EXPORT ecs_tapi* load_ecstapi(){
  //Load virtual memory & allocator systems
  VIRTUALMEMORY_TAPI_PLUGIN_TYPE virmemsys = nullptr;
  ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE allocatorsys = nullptr;
  load_virtualmemorysystem(&virmemsys, &allocatorsys);
  f_vector = allocatorsys->vector_manager;

  mainMemBlock = allocatorsys->createSuperMemoryBlock(virmemAllocSize, ECS_TAPI_NAME);
  ecs_funcs = (ecs_tapi*)allocatorsys->end_of_page->malloc(mainMemBlock, sizeof(ecs_tapi) + sizeof(ecs_d));
  hidden = (ecs_d*)(ecs_funcs + 1);
  *hidden = ecs_d();
  ecs_funcs->loadPlugin = loadPlugin;
  ecs_funcs->reloadPlugin = reloadPlugin;
  ecs_funcs->unloadPlugin = unloadPlugin;
  ecs_funcs->getSystem = getSystem;
  ecs_funcs->get_component_byEntityHnd = get_component_byEntityHnd;
  ecs_funcs->addSystem = addSystem;
  ecs_funcs->destroySystem = destroySystem;
  ecs_funcs->data = hidden;

  
  //Initialize ECS
  ////////////////////////
  hidden->v_pluginInfos = TAPI_VECTOR(ecs_pluginInfo, MAXPLUGINCOUNT);
  hidden->v_systemInfos = TAPI_VECTOR(ecs_systemInfo, MAXSYSTEMCOUNT);
  hidden->v_mainComponentTypes = TAPI_VECTOR(ecs_componentType, MAXCOMPTYPECOUNT);
  hidden->v_entityTypes = TAPI_VECTOR(ecs_entityType, MAXENTITYTYPECOUNT);

  addSystem(VIRTUALMEMORY_TAPI_PLUGIN_NAME, VIRTUALMEMORY_TAPI_PLUGIN_VERSION, virmemsys);
  addSystem(ALLOCATOR_TAPI_PLUGIN_NAME, ALLOCATOR_TAPI_PLUGIN_VERSION, allocatorsys);

  return ecs_funcs;
}

extern "C" FUNC_DLIB_EXPORT unsigned char unload_ecstapi(){
    return 0;
}