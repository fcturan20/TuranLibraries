#pragma once

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

// Define FUNC_DLIB_EXPORT as a C macro for the compiler
// Microsoft
#if defined(_MSC_VER)
//Add extern "C" for C++ compilers
#if defined(__cplusplus)
#define FUNC_DLIB_EXPORT extern "C" __declspec(dllexport)
#else
#define FUNC_DLIB_EXPORT __declspec(dllexport)
#endif

// GCC
#elif defined(__GNUC__)
//Add extern "C" for C++ compilers
#if defined(__cplusplus)
#define FUNC_DLIB_EXPORT extern "C" __attribute__((visibility("default")))
#else
#define FUNC_DLIB_EXPORT __attribute__((visibility("default")))
#endif
#endif


#if defined(T_SUPPORTEDPLATFORM) & defined(T_INCLUDE_PLATFORM_LIBS)
#if defined(_WIN32)
#include "windows.h"


    #define DLIB_LOAD_TAPI(dlib_path) LoadLibrary(TEXT(dlib_path))
    #define DLIB_FUNC_LOAD_TAPI(dlib_var_name, func_name) GetProcAddress(dlib_var_name, func_name)
    #define DLIB_UNLOAD_TAPI(dlib_var_name) FreeLibrary(dlib_var_name)
    
#else
    #error Dynamic library build is failed because compiler's function export attribute isn't supported. Please go to API_includes.h for more info.
#endif


#endif



typedef enum tapi_string_type string_type_tapi;
#define stringArgument_tapi(name) string_type_tapi name##Type, const void* name##Data