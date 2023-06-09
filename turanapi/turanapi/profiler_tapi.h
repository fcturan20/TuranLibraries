#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#define PROFILER_TAPI_PLUGIN_NAME "tapi_profiler"
#define PROFILER_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
#define PROFILER_TAPI_PLUGIN_LOAD_TYPE struct tapi_profiler_type*

typedef struct tapi_profiledscope_handle* profiledscope_handle_tapi;
struct tapi_profiler {
  // You can set handle as NULL if you're gonna use threadlocal_finish_last_profiling
  void (*start_profiling)(profiledscope_handle_tapi* handle, const char* name,
                          unsigned long long* duration, unsigned char timingType);
  void (*finish_profiling)(profiledscope_handle_tapi* handle);
  // Use this if both:
  // 1) You're profiling operations that are run on the same thread...
  // 2) Finish profiling that has started last
  // So you shouldn't use this to create nested profiling systems like:
  // Start Profiling A, Call A, A Starts Profiling B, A calls B, B finishes, A finishes Profiling B,
  // A finishes, End Profiling A Systems like above isn't supported!
  void (*threadlocal_finish_last_profiling)(unsigned char shouldPrint);
};

struct tapi_profiler_type {
  struct tapi_profiler_d* data;
  struct tapi_profiler*   funcs;
};

#define TURAN_PROFILE_SCOPE_NAS(profilertapi, name, duration_ptr) \
  profiledscope_handle_tapi profile##__LINE__;                    \
  profilertapi->start_profiling(&profile##__LINE__, name, duration_ptr, 0)
#define TURAN_PROFILE_SCOPE_MCS(profilertapi, name, duration_ptr) \
  profiledscope_handle_tapi profile##__LINE__;                    \
  profilertapi->start_profiling(&profile##__LINE__, name, duration_ptr, 1)
#define TURAN_PROFILE_SCOPE_MLS(profilertapi, name, duration_ptr) \
  profiledscope_handle_tapi profile##__LINE__;                    \
  profilertapi->start_profiling(&profile##__LINE__, name, duration_ptr, 2)
#define TURAN_PROFILE_SCOPE_SEC(profilertapi, name, duration_ptr) \
  profiledscope_handle_tapi profile##__LINE__;                    \
  profilertapi->start_profiling(&profile##__LINE__, name, duration_ptr, 3)
#define STOP_PROFILE_TAPI(profilertapi) profilertapi->threadlocal_finish_last_profiling(1)

#ifdef __cplusplus
}
#endif