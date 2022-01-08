#pragma once


#include "predefinitions.h"



#ifdef TAPI_THREADING
#include "ThreadedJobCore.h"
#endif

#ifdef TAPI_BITSET
#include "Bitset.h"
#endif

#ifdef TAPI_FILESYSTEM
#include "FileSystem_Core.h"
#endif

#ifdef TAPI_LOGGER
#include "Logger_Core.h"
#endif

#ifdef TAPI_PROFILER
#include "Profiler_Core.h"
#endif
