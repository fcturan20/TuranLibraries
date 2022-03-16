#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // For CommandLineToArgvW

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <d3dx12.h>
#include <algorithm>
#include <cassert>
#include <chrono>

#include <tgfx_forwarddeclarations.h>

//Systems 

struct core_tgfx;
extern core_tgfx* core_tgfx_main;
struct core_public;
extern core_public* core_dx;
struct renderer_public;
extern renderer_public* renderer;
struct gpudatamanager_public;
extern gpudatamanager_public* contentmanager;
struct imgui_dx;
extern imgui_dx* imgui;
struct gpu_public;
extern gpu_public* rendergpu;
typedef struct threadingsys_tapi threadingsys_tapi;
extern threadingsys_tapi* threadingsys;
extern unsigned int threadcount;
struct allocatorsys_dx;
extern allocatorsys_dx* allocatorsys;
struct queuesys_dx;
extern queuesys_dx* queuesys;

extern tgfx_PrintLogCallback printer;


//Synchronization systems and objects

struct semaphoresys_dx;
extern semaphoresys_dx* semaphoresys;
struct fencesys_dx;
extern fencesys_dx* fencesys;
#ifdef VULKAN_DEBUGGING
typedef unsigned int semaphore_idtype_dx;
static constexpr semaphore_idtype_dx invalid_semaphoreid = UINT32_MAX;
typedef unsigned int fence_idtype_dx;
static constexpr fence_idtype_dx invalid_fenceid = UINT32_MAX;
#else
struct semaphore_dx;
typedef semaphore_dx* semaphore_idtype_dx;
static constexpr semaphore_idtype_dx invalid_semaphoreid = nullptr;
struct fence_dx;
typedef fence_dx* fence_idtype_dx;
static constexpr fence_idtype_dx invalid_fenceid = nullptr;
#endif


//Structs

struct texture_dx;
struct memorytype_dx;
struct queuefam_dx;
struct extension_manager;	//Stores activated extensions and set function pointers according to that
struct pass_dx;
struct drawpass_dx;
struct transferpass_dx;
struct windowpass_dx;
struct computepass_dx;
struct subdrawpass_dx;
struct indexeddrawcall_dx;
struct nonindexeddrawcall_dx;
struct irtslotset_dx;
struct rtslotset_dx;
struct window_dx;

//Enums
enum class desctype_dx : unsigned char {
	SAMPLER = 0,
	IMAGE = 1,
	SBUFFER = 2,
	UBUFFER = 3
};

//Resources
struct texture_dx;

extern HINSTANCE hInstance;
extern UINT g_RTVDescriptorSize;
extern ComPtr<IDXGIFactory4> dxgiFactory;

#define TD3D12_WCLASSNAME L"TGFXD3D12"