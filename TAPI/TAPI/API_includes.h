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



// Check windows
#if _WIN32 || _WIN64
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

#define TAPIHANDLE(object) typedef struct tapi_dontuse_##object * tapi_##object;


//Stop the application and ask the user to close or continue!
//I'm using this to remind myself there are problems while I'm working on another thing!
void TURANAPI tapi_breakpoint(const char* Breakpoint_Reason);


typedef enum tapi_result {
	tapi_result_SUCCESS = 0,
	tapi_result_FAIL = 1,
	tapi_result_NOTCODED = 2,
	tapi_result_INVALIDARGUMENT = 3,
	tapi_result_WRONGTIMING = 4	//This means the operation is called at the wrong time!
} tapi_result;


//Some basic functionality to do debugging!
#define GET_VARIABLE_NAME(Variable) (#Variable)
#define SLEEP_THREAD(Variable) std::this_thread::sleep_for(std::chrono::seconds(Variable));



#else
#error "Platform isn't supported!"
#endif
