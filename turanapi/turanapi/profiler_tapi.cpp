#include "profiler_tapi.h"

#include <assert.h>

#include <chrono>
#include <iostream>
#include <string>

#include "ecs_tapi.h"
#include "threadingsys_tapi.h"

struct tlProfiledScope {
 public:
  bool                isRecording : 1;
  unsigned char       timingType : 2;
  unsigned long long  startPoint;
  unsigned long long* duration;
  std::string         name;
};
typedef struct tlProfPriv {
  tlProf*        sys;
  tlProfiledScope** last_handles;
  unsigned int               threadcount;
  const tlJob*         threadsys;
} profiler_tapi_d;

tlProfPriv* priv = nullptr;

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

tlProfiledScope* start_profiling(const char* name,
                     unsigned long long* duration, unsigned char timingType) {
  unsigned int threadindex =
    (priv->threadcount == 1) ? (0) : (priv->threadsys->thisThreadIndx());
  tlProfiledScope* profile                   = new tlProfiledScope;
  profile->startPoint                      = getTime(timingType);
  profile->isRecording                     = true;
  profile->duration                        = duration;
  profile->timingType                      = timingType;
  profile->name                            = name;
  priv->last_handles[threadindex] = profile;
  return profile;
}

void finish_profiling(tlProfiledScope* profil) {
  *profil->duration     = getTime(profil->timingType) - profil->startPoint;
  delete profil;
}
const char* timeNames[] = {"nanoseconds", "microseconds", "milliseconds", "seconds"};
void        threadlocal_finish_last_profiling(unsigned char shouldPrint) {
  unsigned int threadindex =
    (priv->threadcount == 1) ? (0) : (priv->threadsys->thisThreadIndx());
  tlProfiledScope*      profile  = ( tlProfiledScope* )priv->last_handles[threadindex];
  unsigned long long* duration = profile->duration;
  std::string         name     = profile->name;
  unsigned char       timingType = profile->timingType;
  finish_profiling(priv->last_handles[threadindex]);
  if (shouldPrint) {
    printf("%s took %llu %s!\n", name.c_str(), *duration, timeNames[timingType]);
  }
}

ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
  unsigned int                       threadcount = 1;
  THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE threadsystype =
    ( THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE )ecssys->getSystem(THREADINGSYS_TAPI_PLUGIN_NAME);
  if (threadsystype) {
    threadcount = threadsystype->threadCount();
  }

  tlProf* type = ( tlProf* )malloc(sizeof(tlProf));
  priv = ( tlProfPriv* )malloc(sizeof(tlProfPriv));

  ecssys->addSystem(PROFILER_TAPI_PLUGIN_NAME, PROFILER_TAPI_PLUGIN_VERSION, type);

  type->start                   = &start_profiling;
  type->end                  = &finish_profiling;
  type->endLastLocalProfile     = &threadlocal_finish_last_profiling;
  type->data                    = priv;

  priv->threadsys   = threadsystype;
  priv->threadcount        = threadcount;
  priv->last_handles =
    ( tlProfiledScope** )malloc(sizeof(tlProfiledScope*) * threadcount);
  memset(priv->last_handles, 0, sizeof(tlProfiledScope*) * threadcount);
}

ECSPLUGIN_EXIT(ecssys, reloadFlag) {}