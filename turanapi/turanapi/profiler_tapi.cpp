#include "profiler_tapi.h"

#include <assert.h>

#include <chrono>
#include <iostream>
#include <string>

#include "ecs_tapi.h"
#include "threadingsys_tapi.h"

typedef struct tapi_profiler_d {
  tapi_profiler_type*        type;
  profiledscope_handle_tapi* last_handles;
  unsigned int               threadcount;
  tapi_threadingSys*         threadsys;
} profiler_tapi_d;

struct profiledScope {
 public:
  bool                isRecording : 1;
  unsigned char       timingType : 2;
  unsigned long long  startPoint;
  unsigned long long* duration;
  std::string         name;
};

tapi_profiler_d* profiler_data = nullptr;

constexpr long long getTime(unsigned char timingType) {
  switch (timingType) {
    case 0:
      return std::chrono::time_point_cast<std::chrono::nanoseconds>(
               std::chrono::high_resolution_clock::now())
        .time_since_epoch()
        .count();
      break;
    case 1:
      return std::chrono::time_point_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now())
        .time_since_epoch()
        .count();
      break;
    case 2:
      return std::chrono::time_point_cast<std::chrono::milliseconds>(
               std::chrono::high_resolution_clock::now())
        .time_since_epoch()
        .count();
      break;
    case 3:
      return std::chrono::time_point_cast<std::chrono::seconds>(
               std::chrono::high_resolution_clock::now())
        .time_since_epoch()
        .count();
      break;
    default: assert(0 && "Timing type is invalid!");
  }
}

void start_profiling(profiledscope_handle_tapi* handle, const char* name,
                     unsigned long long* duration, unsigned char timingType) {
  unsigned int threadindex =
    (profiler_data->threadcount == 1) ? (0) : (profiler_data->threadsys->this_thread_index());
  profiledScope* profile                   = new profiledScope;
  profile->startPoint                      = getTime(timingType);
  profile->isRecording                     = true;
  profile->duration                        = duration;
  profile->timingType                      = timingType;
  profile->name                            = name;
  *handle                                  = ( profiledscope_handle_tapi )profile;
  profiler_data->last_handles[threadindex] = *handle;
}

void finish_profiling(profiledscope_handle_tapi* handle) {
  profiledScope* profil = ( profiledScope* )*handle;
  *profil->duration     = getTime(profil->timingType) - profil->startPoint;
  delete profil;
}
const char* timeNames[] = {"nanoseconds", "microseconds", "milliseconds", "seconds"};
void        threadlocal_finish_last_profiling(unsigned char shouldPrint) {
  unsigned int threadindex =
    (profiler_data->threadcount == 1) ? (0) : (profiler_data->threadsys->this_thread_index());
  profiledScope*      profile  = ( profiledScope* )profiler_data->last_handles[threadindex];
  unsigned long long* duration = profile->duration;
  std::string         name     = profile->name;
  unsigned char       timingType = profile->timingType;
  finish_profiling(&profiler_data->last_handles[threadindex]);
  if (shouldPrint) {
    printf("%s took %llu %s!\n", name.c_str(), *duration, timeNames[timingType]);
  }
}

ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
  unsigned int                       threadcount = 1;
  THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE threadsystype =
    ( THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE )ecssys->getSystem(THREADINGSYS_TAPI_PLUGIN_NAME);
  if (threadsystype) {
    threadcount = threadsystype->funcs->thread_count();
  }

  tapi_profiler_type* type = ( tapi_profiler_type* )malloc(sizeof(tapi_profiler_type));
  type->data               = ( tapi_profiler_d* )malloc(sizeof(tapi_profiler_d));
  type->funcs              = ( tapi_profiler* )malloc(sizeof(tapi_profiler));
  profiler_data            = type->data;

  ecssys->addSystem(PROFILER_TAPI_PLUGIN_NAME, PROFILER_TAPI_PLUGIN_VERSION, type);

  type->funcs->start_profiling                   = &start_profiling;
  type->funcs->finish_profiling                  = &finish_profiling;
  type->funcs->threadlocal_finish_last_profiling = &threadlocal_finish_last_profiling;

  type->data->threadsys   = threadsystype->funcs;
  type->data->threadcount = threadcount;
  type->data->last_handles =
    ( profiledscope_handle_tapi* )malloc(sizeof(profiledscope_handle_tapi) * threadcount);
  memset(type->data->last_handles, 0, sizeof(profiledscope_handle_tapi) * threadcount);
}

ECSPLUGIN_EXIT(ecssys, reloadFlag) {}