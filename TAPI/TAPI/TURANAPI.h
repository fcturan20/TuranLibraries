#pragma once

//User may define preprocessors wrong. Before including anything else, say the error
#ifndef __cplusplus
#ifdef USE_TAPINAMESPACE
#error "You can't use namespace in C, please use C++"
#define STATIC_TAPIERROR
#endif	//USE_TAPINAMESPACE

#ifdef USE_TAPIENUMCLASS
#error "You can't use enum class in C, please use C++"
#define STATIC_TAPIERROR
#endif


#ifdef TAPI_LOGGER
#ifndef TAPI_FILESYSTEM
#error "You can't use TAPI_LOGGER without TAPI_FILESYSTEM"
#define STATIC_TAPIERROR
#endif
#endif

#ifdef TAPI_THREADING_CPP_HELPER
#error "You can't use TAPI_THREADING_CPP_HELPER in C"
#define STATIC_TAPIERROR
#endif

#endif

#ifndef STATIC_TAPIERROR
#include <TAPI/API_includes.h>



#ifdef TAPI_THREADING
#include <TAPI/ThreadedJobCore.h>
#endif

#ifdef TAPI_BITSET
#include <TAPI/Bitset.h>
#endif

#ifdef TAPI_FILESYSTEM
#include <TAPI/FileSystem_Core.h>
#endif

#ifdef TAPI_LOGGER
#include <TAPI/Logger_Core.h>
#endif

#ifdef TAPI_PROFILER
#include <TAPI/Profiler_Core.h>
#endif



#endif
