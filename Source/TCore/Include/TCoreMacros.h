#pragma once

#define TCORE_C_LINKAGE extern "C"
#define TCORE_BEGIN_C_LINKAGE extern "C" {
#define TCORE_END_C_LINKAGE }
#define TCORE_DEFINE_HANDLE(name) typedef struct name* name##Handle

#define TCORE_MAKE_PLUGIN_VERSION(major, mid, minor) (((major < 255 ? major : 255) << 16) | \
((mid < 255 ? mid : 255) << 8) | ((minor < 255 ? minor : 255)))
#define TCORE_GET_PLUGIN_MAJOR(version) (version >> 16)

TCORE_BEGIN_C_LINKAGE

#pragma region Platform Detection & Macro Definition

// Check 32-64 bit
#if defined(_WIN32) || defined(_WIN64)
#define T_ENVWINDOWS
#if _WIN64
#define T_ENV64BIT
#else
#define T_ENV32BIT
#endif
#elif defined(__APPLE__) || defined(__MACH__) || defined(__GNUC__)
#define T_ENVMACOS
#if __x86_64__ || __ppc64__ || __aarch64__
#define T_ENV64BIT
#else
#define T_ENV32BIT
#endif
#endif

// Fail if the platform is 32 bit
#if defined(T_ENV64BIT)
#define T_SUPPORTEDPLATFORM
#elif defined(T_ENV32BIT)
#error "32 bit platform is not supported! Please use 64 bit platform to build this project."
#else
#error "Project configurations should be corrupted because environment is neither 32 bit or 64 bit! So project failed to compile"
#endif

// Define TCORE_FUN_EXPORT as a C macro for the compiler
// Microsoft
#if defined(_MSC_VER)
//Add extern "C" for C++ compilers
#if defined(__cplusplus)
#define TCORE_FUN_EXPORT TCORE_C_LINKAGE __declspec(dllexport)
#else
#define TCORE_FUN_EXPORT __declspec(dllexport)
#endif
// GCC
#elif defined(__GNUC__)
//Add TCORE_C_LINKAGE for C++ compilers
#if defined(__cplusplus)
#define TCORE_FUN_EXPORT TCORE_C_LINKAGE __attribute__((visibility("default")))
#else
#define TCORE_FUN_EXPORT __attribute__((visibility("default")))
#endif
#endif

#if defined(T_SUPPORTEDPLATFORM) & defined(T_INCLUDE_PLATFORM_LIBS)
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "windows.h"
    #define TCORE_DLL_EXTENSION ".dll"
#elif defined(__APPLE__) || defined(__MACH__) || defined(__GNUC__)
    #include <dlfcn.h>
    #define TCORE_DLL_EXTENSION ".dylib"
#else
    #error Dynamic library build is failed because compiler's function export attribute isn't supported. Please go to API_includes.h for more info.
#endif
#endif

typedef enum TCResult {
    TC_RESULT_SUCCESS = 0,
    TC_RESULT_FAILURE = 1,
    TC_RESULT_UNKNOWN = 2,
    TC_RESULT_INVALID_ARGUMENT = 3,
    TC_RESULT_OUT_OF_MEMORY = 4,
    TC_RESULT_NOT_FOUND = 5,
    TC_RESULT_ALREADY_EXISTS = 6,
    TC_RESULT_UNSUPPORTED = 7,
    TC_RESULT_TIMEOUT = 8,
    TC_RESULT_PERMISSION_DENIED = 9,
    TC_RESULT_UNIMPLEMENTED = 10,
    TC_RESULT_ABSENT_DEPENDENCY = 11
} TCResult;

typedef unsigned char TBool;
#define TTRUE 1
#define TFALSE 0

typedef struct TCBuffer {
    void* data;
    unsigned long size;
} TCBuffer;

typedef struct TCReadBuffer {
    const void* data;
    unsigned long size;
} TCReadBuffer;

TCORE_END_C_LINKAGE