#include "main.h"
#include "ecs_tapi.h"
#include "allocator_tapi.h"
#include "pecfManager/pecfManager.h"
#include "threadingsys_tapi.h"
#include "profiler_tapi.h"
#include "logger_tapi.h"
#include "filesys_tapi.h"
#include "allocator_tapi.h"
#include "array_of_strings_tapi.h"
#include <stdio.h>
#include <random>
#include <string>

allocator_sys_tapi* allocatorSys = nullptr;
uint64_t destructionCount = 0;
struct pluginElement {
  pluginHnd_ecstapi pluginPTR;
  const char* pluginMainSysName;
  unsigned char copyCount;
  /*
  pluginElement() : pluginPTR(nullptr), pluginMainSysName(nullptr), copyCount(0){}
  ~pluginElement() {
    destructionCount++;
  }
  pluginElement::pluginElement(const pluginElement& src) { 
    pluginPTR = src.pluginPTR; 
    pluginMainSysName = src.pluginMainSysName;
    copyCount = src.copyCount;
  }
  static void defaultInitializePluginStruct(void* ptr) {
    pluginElement* plgn = (pluginElement*)ptr;
    plgn->pluginMainSysName = nullptr;
    plgn->pluginPTR = nullptr;
    plgn->copyCount = 0;
  }
  static void defaultCopyFuncPluginStruct(const void* src, void* dst) {
    const pluginElement* srcPlgn = (pluginElement*)src;
    pluginElement* dstPlgn = (pluginElement*)dst;
    dstPlgn->copyCount = srcPlgn->copyCount + 1;
    dstPlgn->pluginMainSysName = srcPlgn->pluginMainSysName;
    dstPlgn->pluginPTR = srcPlgn->pluginPTR;
  }
  static void defaultDestructorPluginStruct(void* ptr) {
    destructionCount++;
  }*/
};

void load_systems() {
  pluginHnd_ecstapi threadingPlugin = editorECS->loadPlugin("tapi_threadedjobsys.dll");
  auto threadingSys = (THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE)editorECS->getSystem(THREADINGSYS_TAPI_PLUGIN_NAME);
  printf("Thread Count: %u\n", threadingSys->funcs->thread_count());

  pluginHnd_ecstapi arrayOfStringsPlugin = editorECS->loadPlugin("tapi_array_of_strings_sys.dll");
  auto AoSsys = (ARRAY_OF_STRINGS_TAPI_LOAD_TYPE)editorECS->getSystem(ARRAY_OF_STRINGS_TAPI_PLUGIN_NAME);

  pluginHnd_ecstapi profilerPlugin = editorECS->loadPlugin("tapi_profiler.dll");
  auto profilerSys = (PROFILER_TAPI_PLUGIN_LOAD_TYPE)editorECS->getSystem(PROFILER_TAPI_PLUGIN_NAME);

  pluginHnd_ecstapi filesysPlugin = editorECS->loadPlugin("tapi_filesys.dll");
  auto filesys = (FILESYS_TAPI_PLUGIN_LOAD_TYPE)editorECS->getSystem(FILESYS_TAPI_PLUGIN_NAME);
  

  pluginHnd_ecstapi loggerPlugin = editorECS->loadPlugin("tapi_logger.dll");
  auto loggerSys = (LOGGER_TAPI_PLUGIN_LOAD_TYPE)editorECS->getSystem(LOGGER_TAPI_PLUGIN_NAME);
  
  allocatorSys = (ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE)editorECS->getSystem(ALLOCATOR_TAPI_PLUGIN_NAME);
  unsigned long long Resizes[1000];
  static constexpr uint64_t vectorMaxSize = uint64_t(1) << uint64_t(36);
  for (unsigned int i = 0; i < 1000; i++) {
    Resizes[i] = rand() % vectorMaxSize;
  }
  
  supermemoryblock_tapi* superMemBlock = allocatorSys->createSuperMemoryBlock(1 << 30, "Vector Perf Test");
  pluginElement* v_pluginElements = nullptr;
  {
    profiledscope_handle_tapi profiling; unsigned long long duration = 0;
    TURAN_PROFILE_SCOPE_MCS(profilerSys->funcs, "Vector Custom", &duration);
    v_pluginElements = (pluginElement*)allocatorSys->vector_manager->create_vector(
      sizeof(pluginElement), superMemBlock, 10, 1000, 
      0
      //(vector_flagbits_tapi)(vector_flagbit_constructor_tapi | vector_flagbit_copy_tapi | vector_flagbit_destructor_tapi), 
      //pluginElement::defaultInitializePluginStruct, pluginElement::defaultCopyFuncPluginStruct, pluginElement::defaultDestructorPluginStruct
    );
    for (unsigned int i = 0; i < 1000; i++) {
      allocatorSys->vector_manager->resize(v_pluginElements, Resizes[i]);
    }
    STOP_PROFILE_PRINTLESS_TAPI(profilerSys->funcs);
    loggerSys->funcs->log_status(("Loading systems took: " + std::to_string(duration)).c_str());
  }
}