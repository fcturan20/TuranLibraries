#pragma once

#define TCORE_C_LINKAGE extern "C"
#define TCORE_BEGIN_C_LINKAGE                                                                                          \
	extern "C"                                                                                                         \
	{
#define TCORE_END_C_LINKAGE }
#define TCORE_DEFINE_HANDLE(name) typedef struct name* name##Handle

#define TCORE_MAKE_PLUGIN_VERSION(major, mid, minor)                                                                   \
	(((major < 255 ? major : 255) << 16) | ((mid < 255 ? mid : 255) << 8) | ((minor < 255 ? minor : 255)))
#define TCORE_GET_PLUGIN_MAJOR(version) (version >> 16)

TCORE_BEGIN_C_LINKAGE

#pragma region Platform Detection & Macro Definition

// Check 32-64 bit
#if defined(_WIN32) || defined(_WIN64)
#define T_ENVWINDOWS
#elif defined(__APPLE__) || defined(__MACH__)
#define T_ENVMACOS
#elif defined(__linux__)
#define T_ENVLINUX
#endif

#if defined(__x86_64__) || defined(__ppc64__) || defined(__aarch64__) || defined(_WIN64) || defined(__LP64__) ||       \
	defined(_LP64)
#define T_ENV64BIT
#elif defined(__i386__) || defined(__arm__) || defined(_WIN32) || defined(__ILP32__) || defined(_ILP32)
#define T_ENV32BIT
#endif

// Fail if the platform is 32 bit
#if defined(T_ENV64BIT)
#define T_SUPPORTEDPLATFORM
#elif defined(T_ENV32BIT)
#error "32 bit platform is not supported! Please use 64 bit platform to build this project."
#else
#error                                                                                                                 \
	"Project configurations should be corrupted because environment is neither 32 bit or 64 bit! So project failed to compile"
#endif

// Define TCORE_FUN_EXPORT as a C macro for the compiler
// Microsoft
#if defined(_MSC_VER)
#if defined(__cplusplus)
#define TCORE_FUN_EXPORT TCORE_C_LINKAGE __declspec(dllexport)
#else
#define TCORE_FUN_EXPORT __declspec(dllexport)
#endif
// GCC
#elif defined(__GNUC__)
// Add TCORE_C_LINKAGE for C++ compilers
#if defined(__cplusplus)
#define TCORE_FUN_EXPORT TCORE_C_LINKAGE __attribute__((visibility("default")))
#else
#define TCORE_FUN_EXPORT __attribute__((visibility("default")))
#endif
#endif

// Define TCORE_CPP_20 as a C++ macro for the compiler on all platforms
#if defined(__cplusplus) && __cplusplus >= 202002L
#define TCORE_CPP_20
// Define TCORE_USE_CPP_WRAPPER if you want to use C++ wrapper for TCore API
#endif

typedef enum TCResult
{
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
static_assert(sizeof(TBool) == 1, "TBool should be 1 byte in size!");

typedef unsigned long long TSize;
static_assert(sizeof(TSize) == 8, "TSize should be 8 bytes in size!");

typedef unsigned long long TUint;
static_assert(sizeof(TUint) == 8, "TUint should be 8 bytes in size!");

// Modifiable buffer
typedef struct TCBuffer
{
	void* Data;
	unsigned long Size;
} TCBuffer;

// Constant (non-modifiable) buffer
typedef struct TCReadBuffer
{
	const void* Data;
	unsigned long Size;
} TCReadBuffer;

#if defined(NDEBUG)
#define TCORE_SOFT_CHECK(condition, message)                                                                           \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		printf("Soft check failed: %s\n", message);                                                                    \
		__debugbreak(); // MSVC
}

#define TCORE_HARD_CHECK(condition, message)                                                                           \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		printf("Hard check failed: %s\n", message);                                                                    \
		exit(1);                                                                                                       \
	}
#else
#include <stdio.h>
#include <stdlib.h>
#define TCORE_SOFT_CHECK(condition, message)                                                                           \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		perror("Soft check failed: " message);                                                                         \
		abort();                                                                                                       \
	}
#define TCORE_HARD_CHECK(condition, message)                                                                           \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		perror("Hard check failed: " message);                                                                         \
		abort();                                                                                                       \
	}
#endif

TCORE_END_C_LINKAGE