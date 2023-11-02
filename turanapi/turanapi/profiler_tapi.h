#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define PROFILER_TAPI_PLUGIN_NAME "tapi_profiler"
#define PROFILER_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
#define PROFILER_TAPI_PLUGIN_LOAD_TYPE const struct tlProf*

struct tlProfiledScope;
struct tlProf {
  const struct tlProfPriv* data;
  struct tlProfiledScope* (*start)(const char* name,
                          unsigned long long* duration, unsigned char timingType);
  void (*end)(struct tlProfiledScope* scope);
  // Use this if both:
  // 1) You're profiling operations that are run on the same thread...
  // 2) Finish profiling that has started last
  // So you shouldn't use this to create nested profiling systems like:
  // Start Profiling A, Call A, A Starts Profiling B, A calls B, B finishes, A finishes Profiling B,
  // A finishes, End Profiling A Systems like above isn't supported!
  void (*endLastLocalProfile)(unsigned char shouldPrint);
};

#define TURAN_PROFILE_SCOPE_NAS(profilertapi, name, duration_ptr) \
  tlProfiledScope* profile##__LINE__ = profilertapi->start(name, duration_ptr, 0)
#define TURAN_PROFILE_SCOPE_MCS(profilertapi, name, duration_ptr) \
  tlProfiledScope* profile##__LINE__ = profilertapi->start(name, duration_ptr, 1)
#define TURAN_PROFILE_SCOPE_MLS(profilertapi, name, duration_ptr) \
  tlProfiledScope* profile##__LINE__ = profilertapi->start(name, duration_ptr, 2)
#define TURAN_PROFILE_SCOPE_SEC(profilertapi, name, duration_ptr) \
  tlProfiledScope* profile##__LINE__ = profilertapi->start(name, duration_ptr, 3)
#define STOP_PROFILE_TAPI(profilertapi) profilertapi->endLastLocalProfile(1)

#ifdef __cplusplus
}
#endif