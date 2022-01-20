/*	Turan API supports:
1) File I/O and Data Types and Serialization/Deserialization
2) GLM math library
3) STD libraries
4) Image read/write
5) Run-time Type Introspection (Just enable in VS 2019, no specific code!)
6) IMGUI (Needs to be implemented for each GFX_API)
7) Basic profiler (Only calculates time)

	Future supports:
1) Multi-thread, ECS, Job
2) Networking
3) Compression, Hashing, Algorithms
4) Unit Testing Framework
*/

#pragma once

#define TURANAPI
#define TURAN_DEBUGGING

#define MAKE_PLUGIN_VERSION_TAPI(major, mid, minor) (((major < 255 ? major : 255) << 16) | \
((mid < 255 ? mid : 255) << 8) | ((minor < 255 ? minor : 255)))
#define GET_PLUGIN_VERSION_TAPI_MAJOR(version) (version >> 16)

// Check windows
#if defined(_WIN32) || defined(_WIN64)
#define T_ENVWINDOWS
#if _WIN64
#define T_ENV64BIT
#else
#define T_ENV32BIT
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define T_ENV64BIT
#else
#define T_ENV32BIT
#endif
#endif

#if defined(T_ENV64BIT)
#define T_SUPPORTEDPLATFORM
#elif defined(T_ENV32BIT)
#define T_SUPPORTEDPLATFORM
#else
#error "Project configurations should be corrupted because environment is neither 32 bit or 64 bit! So project failed to compile"
#endif

#ifdef T_SUPPORTEDPLATFORM



#if defined(_WIN32)
#include "windows.h"

#if defined(_MSC_VER)
    //  Microsoft 
    #define FUNC_DLIB_EXPORT __declspec(dllexport)
#elif defined(__GNUC__)
    //  GCC
    #define FUNC_DLIB_EXPORT __attribute__((visibility("default")))
#endif

    #define DLIB_LOAD_TAPI(dlib_path) LoadLibrary(TEXT(dlib_path))
    #define DLIB_FUNC_LOAD_TAPI(dlib_var_name, func_name) GetProcAddress(dlib_var_name, func_name)
    #define DLIB_UNLOAD_TAPI(dlib_var_name) FreeLibrary(dlib_var_name)
    
#else
    #error Dynamic library build is failed because compiler's function export attribute isn't supported. Please go to API_includes.h for more info.
#endif


#else
#error "Platform isn't supported!"
#endif


