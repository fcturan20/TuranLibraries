#pragma once
#include "TCore.h"

TCORE_BEGIN_C_LINKAGE
TCORE_PLUGIN_DEFINE(TCProfiler, "tcProfiler", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

TCORE_DEFINE_HANDLE(TCProfiledScope);
typedef enum TCDurationType
{
	TC_DURATION_TYPE_NANOSECONDS,
	TC_DURATION_TYPE_MICROSECONDS,
	TC_DURATION_TYPE_MILLISECONDS,
	TC_DURATION_TYPE_SECONDS
} TCDurationType;

typedef struct ITCProfiler
{
	TCProfiledScope* (*Begin)(const char* name, unsigned long long* duration, TCDurationType duration_type);
	void (*End)(TCProfiledScope* scope);
	// Use this if both:
	// 1) You're profiling operations that are run on the same thread...
	// 2) Finish profiling that has started last
	// So you shouldn't use this to create nested profiling systems like:
	// Start Profiling A, Call A, A Starts Profiling B, A calls B, B finishes, A finishes Profiling B,
	// A finishes, End Profiling A Systems like above isn't supported!
	void (*EndLastLocalProfile)(unsigned char shouldPrint);
} ITCProfiler;

#define TURAN_PROFILE_SCOPE_NAS(profilertapi, name, duration_ptr)                                                      \
	TCProfiledScope* profile##__LINE__ = profilertapi->Begin(name, duration_ptr, TC_DURATION_TYPE_NANOSECONDS)
#define TURAN_PROFILE_SCOPE_MCS(profilertapi, name, duration_ptr)                                                      \
	TCProfiledScope* profile##__LINE__ = profilertapi->Begin(name, duration_ptr, TC_DURATION_TYPE_MICROSECONDS)
#define TURAN_PROFILE_SCOPE_MLS(profilertapi, name, duration_ptr)                                                      \
	TCProfiledScope* profile##__LINE__ = profilertapi->Begin(name, duration_ptr, TC_DURATION_TYPE_MILLISECONDS)
#define TURAN_PROFILE_SCOPE_SEC(profilertapi, name, duration_ptr)                                                      \
	TCProfiledScope* profile##__LINE__ = profilertapi->Begin(name, duration_ptr, TC_DURATION_TYPE_SECONDS)
#define STOP_PROFILE_TAPI(profilertapi) profilertapi->EndLastLocalProfile(1)

TCORE_END_C_LINKAGE