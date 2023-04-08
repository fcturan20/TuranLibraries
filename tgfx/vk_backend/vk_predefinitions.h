#pragma once
// Vulkan Libs
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <assert.h>
#include <bitset_tapi.h>
#include <stdint.h>
#include <tgfx_core.h>
#include <tgfx_forwarddeclarations.h>
#include <virtualmemorysys_tapi.h>

#include <atomic>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>

// NAMING RULES:
// 1) Variables directly passed to Vulkan should have "vk_" prefix
//	Examples: VkBuffer vk_bufferHnd; unsigned int vk_memoryTypeIndex;
// 2) Variables that are internally stored should have "m_" prefix
//  Examples: unsigned int m_sizeOfBuffer;
// 3) Variables that are given the user by their address should be constructed with _VKOBJ template
// below and naming should be all upper case

// Systems
typedef struct tgfx_core core_tgfx;
extern core_tgfx*        core_tgfx_main;
struct core_public;
extern core_public* core_vk;
struct renderer_public;
extern renderer_public* renderer;
struct gpudatamanager_public;
extern gpudatamanager_public* contentManager;
struct manager_vk;
extern manager_vk* manager;
struct imgui_vk;
extern imgui_vk* imgui;
struct GPU_VKOBJ;
// extern GPU_VKOBJ* rendergpu;
typedef struct threadingsys_tapi threadingsys_tapi;
extern threadingsys_tapi*        threadingsys;
extern unsigned int              threadcount;
struct gpuallocatorsys_vk;
extern gpuallocatorsys_vk*    gpu_allocator;
extern virtualmemorysys_tapi* virmemsys;
typedef struct profiler_tapi  profiler_tapi;
extern profiler_tapi*         profilerSys;
extern bitsetsys_tapi*        bitsetSys;

extern tgfx_logCallback printer_cb;
result_tgfx                    vkPrint(unsigned int logCode, const wchar_t* extraInfo = nullptr) {
  printer_cb(logCode, extraInfo);
  return core_tgfx_main->getLogMessage(logCode, nullptr);
}

// <------------------------------------------------------------------------------------>
//		Forward Declarations
// <------------------------------------------------------------------------------------>

struct TEXTURE_VKOBJ;
struct memorytype_vk;
struct QUEUE_VKOBJ;
struct manager_vk;
struct extManager_vkDevice;
struct IRTSLOTSET_VKOBJ;
struct RTSLOTSET_VKOBJ;
struct WINDOW_VKOBJ;
struct FENCE_VKOBJ;

void pNext_addToLast(void* targetStruct, void* attachStruct);

#define VK_PRIM_MIN(primType) std::numeric_limits<primType>::min()
#define VK_PRIM_MAX(primType) std::numeric_limits<primType>::max()

// Use this pre-processor in .cpp files to load extension functions
// Backend is maintainly on Vulkan 1.0 because most of the extensions can be used in it
// Search "Requires support for Vulkan 1.*" in VK Specification to learn minimum API Version
#define declareVkExtFunc(funcName)               \
  PFN_##funcName        funcName##_loadVkFunc(); \
  extern PFN_##funcName funcName##_loaded;

#define defineVkExtFunc(funcName)                                                             \
  PFN_##funcName funcName##_loadVkFunc() {                                                    \
    auto func = ( PFN_##funcName )vkGetInstanceProcAddr(VKGLOBAL_INSTANCE, #funcName);        \
    if (func == nullptr) {                                                                    \
      vkPrint(16,                                                               \
              L"Vulkan failed to load " #funcName " function! Probably an OS update problem"); \
      return nullptr;                                                                         \
    }                                                                                         \
    return func;                                                                              \
  };                                                                                          \
  PFN_##funcName funcName##_loaded = nullptr;

#define loadVkExtFunc(funcName) funcName##_loaded = funcName##_loadVkFunc()

// <------------------------------------------------------------------------------------>
//		CONSTANTS
// Note: For name convenience, all constant should start with VKCONST_
// <------------------------------------------------------------------------------------>

#define vk_uint32c static constexpr uint32_t
#define vk_float32c static constexpr float32_t
#define vk_handleType static constexpr VKHANDLETYPEs
#define vk_uint8c static constexpr uint8_t
// User can pass lots of descriptor sets in a list, this defines the max size
vk_uint32c VKCONST_MAXDESCSET_PERLIST          = 12;
vk_uint32c VKCONST_MAXVIEWPORTCOUNT            = 16;
vk_uint32c VKCONST_MAXGPUCOUNT                 = 4;
vk_uint32c VKCONST_MAXRTSLOTCOUNT              = 16;
vk_uint32c VKCONST_MAXQUEUEFAMCOUNT_PERGPU     = 5;
vk_uint32c VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT = 16;
vk_uint32c VKCONST_MAXSWPCHNCOUNT_PERSUBMIT    = 8; // Max count of swapchain count per submit

// VIRMEM RELATED

vk_uint32c VKCONST_MAXSWPCHNTXTURECOUNT_PERWINDOW = 4;
// Memory manager reserves some of the first virtual memory pages for its own use
vk_uint8c  VKCONST_VIRMEM_MANAGERONLYPAGECOUNT = 16;
vk_uint32c VKCONST_VIRMEM_PERFRAME_PAGECOUNT =
  1 << 16; // Define how many pages will be allocated per frame for dynamic memory
extern void*    VKCONST_VIRMEMSPACE_BEGIN;
extern uint64_t VKCONST_VIRMEMPAGESIZE;

//--------------------------------------------------------------------------------------
//		GLOBALS
// Note: For name convenience, all globals should start with VKGLOBAL_
//--------------------------------------------------------------------------------------

extern VkInstance        VKGLOBAL_INSTANCE;
extern VkApplicationInfo VKGLOBAL_APPINFO;

//--------------------------------------------------------------------------------------
//		MEMORY MANAGEMENT
// Vulkan backend has different memory management strategies
// This part is to provide general functionality for them
//--------------------------------------------------------------------------------------

class vk_virmem {
 public:
  struct dynamicmem;
  // Returns memory offset from the start of the memory allocation (which is mem ptr)
  // Rounds up to pageSize
  static uint32_t allocatePage(uint32_t size, uint32_t* roundedUpSize = nullptr);
  // Frees memory suballocation
  // Don't need size because size is stored in VK_PAGEINFO list
  static void free_page(uint32_t suballocation_startoffset);
  // Use dynamic memory for allocations like data handles (that can be used for a frame etc)
  // If you want to get allocated space (except the dynamic mem data), you can use dynamicmemstart
  // argument
  static dynamicmem* allocate_dynamicmem(uint32_t size, uint32_t* dynamicmemstart = nullptr);
  // If you want to allocate some mem but don't want to commit it, you should set shouldcommit as
  // false
  static uint32_t allocate_from_dynamicmem(dynamicmem* mem, uint32_t size,
                                           bool shouldcommit = true);
  static void     free_dynamicmem(dynamicmem* mem);
};
// This is used to store data handles that will be alive for only one frame
// This will be changed every frame but named as const to find easily
extern vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_CURRENTFRAME;

// void* operator new(size_t size);
// void* operator new[](size_t size);
void* operator new(size_t size, vk_virmem::dynamicmem* mem);
void* operator new[](size_t size, vk_virmem::dynamicmem* mem);

#define VK_MEMOFFSET_TO_POINTER(offset) \
  (( void* )(uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + uint32_t(offset)))
#define VK_POINTER_TO_MEMOFFSET(ptr) \
  (uint32_t)(uintptr_t(ptr) - uintptr_t(VKCONST_VIRMEMSPACE_BEGIN))
#define VK_ALLOCATE_AND_GETPTR(dynamicmem, size) \
  (( void* )VK_MEMOFFSET_TO_POINTER(vk_virmem::allocate_from_dynamicmem(dynamicmem, size)))

// There can be only 65536 types of handles that users can access (which is enough)
enum class VKHANDLETYPEs : unsigned short {
  UNDEFINED = 0,
  INTERNAL, // You can use INTERNAL for objects that won't be returned to user as handle but you
            // want the vkobjectstructure for the data backend uses
  SAMPLER,
  BINDINGTABLEINST, // Returned as binding table
  VERTEXATTRIB,
  SHADERSOURCE,
  BUFFER,
  RTSLOTSET,
  IRTSLOTSET,
  TEXTURE,
  WINDOW,
  GPU,
  MONITOR,
  WAITDESC_SUBDP,
  WAITDESC_SUBCP,
  WAITDESC_SUBTP,
  VIEWPORT,
  HEAP,
  GPUQUEUE,
  FENCE,
  CMDBUFFER,
  CMDBUNDLE,
  PIPELINE,
  SUBRASTERPASS
};

struct VKOBJHANDLE {
  VKHANDLETYPEs type : 16;
  uint16_t EXTRA_FLAGs : 16; // This is generally for padding (because I want this struct to match a
                             // 8byte pointer's size)
  uint32_t OBJ_memoffset : 32; // It's offset of the object from the start of the memory allocation
};
static_assert(
  sizeof(VKOBJHANDLE) == sizeof(void*),
  "VKOBJHANDLE class is used to return handles to the user and it's assumed to have the size of a "
  "pointer. Please fix all VKOBJHANDLE funcs/enums to match your pointer size!");
struct VKDATAHANDLE {
  VKHANDLETYPEs type : 16;
  uint32_t      DATA_memoffset : 32;
};
static_assert(
  sizeof(VKDATAHANDLE) == sizeof(void*),
  "VKDATAHANDLE class is used to return handles to the user and it's assumed to have the size of a "
  "pointer. Please fix all VKDATAHANDLE funcs/enums to match your pointer size!");

#define VK_USE_STD

#ifdef VK_USE_STD
#include <vector>
template <typename T, typename TGFXHND, unsigned int max_object_count = 1 << 20>
class VK_LINEAR_OBJARRAY {
  static_assert(T::HANDLETYPE != VKHANDLETYPEs::UNDEFINED, "VKOBJ's type shouldn't be UNDEFINED");
  const unsigned int elementCountPerPage() { return VKCONST_VIRMEMPAGESIZE / sizeof(T); }
  std::vector<T*>    data;

 public:
  VK_LINEAR_OBJARRAY() {}
  T* create_OBJ() {
    T* o = new T;
    data.push_back(o);
    return o;
  }
  void destroyObj(unsigned int objIndx) {
    T* o = data[objIndx];
    delete o;
    data.erase(data.begin() + objIndx);
  }
  bool isValid(T* obj) {
    for (T* o : data) {
      if (obj == o) {
        return true;
      }
    }
    return false;
  }
  unsigned int size() const { return data.size(); }
  T*           getOBJbyINDEX(unsigned int i) { return (isValid(data[i])) ? (data[i]) : (NULL); }
  T*           operator[](unsigned int index) { return getOBJbyINDEX(index); }
  uint32_t     getINDEXbyOBJ(T* obj) {
    for (uint32_t i = 0; i < data.size(); i++) {
      if (data[i] == obj) {
        return i;
      }
    }
    return UINT32_MAX;
  }
};
#else
// Give a non-committed memory region's pointer while starting or let it handle all allocations
// Backend should allocate all of the objects that will be returned to the user here
// Backend also can use this to for non-returned objects (which backend uses internally and no other
// system is aware of the data) 	but in this case you should VKHANDLETYPEs::INTERNAL How it works:
// It pushes back each new element until MAX_OBJECT_INDEX == OBJCOUNT
//  So freed objects are just freed, won't be touched again till linked lists begin
// After MAX_OBJECT_INDEX == OBJCOUNT; Array starts using linked list stored in free elements to
// create objects in holes between alive elements Linked lists are managed at the end of every frame
template <typename T, typename TGFXHND, unsigned int max_object_count = 1 << 20>
class VK_LINEAR_OBJARRAY {
  static_assert(T::HANDLETYPE != VKHANDLETYPEs::UNDEFINED, "VKOBJ's type shouldn't be UNDEFINED");
  const unsigned int elementCountPerPage() { return VKCONST_VIRMEMPAGESIZE / sizeof(T); }
  T* data = nullptr;
  // Active Object -> 1, Free Object -> 0
  bitset_tapi* bitset = nullptr;
  std::atomic_uint32_t maxObjIndx = 0;
  void commitNewPage() {
    uint32_t elementIndx = maxObjIndx.fetch_add(elementCountPerPage());
    virmemsys->virtual_commit(&data[elementIndx], VKCONST_VIRMEMPAGESIZE);
  }
  unsigned int getFreeBit(int8_t* bitset) { return bitsetSys->getFirstBitIndx(bitset, true); }

 public:
  // Allocate enough pages from main allocator directly
  // Don't default initialize objects, because space is not committed
  VK_LINEAR_OBJARRAY() {
    data = ( T* )VK_MEMOFFSET_TO_POINTER(
      vk_virmem::allocatePage((max_object_count * sizeof(T) / VKCONST_VIRMEMPAGESIZE) + 1));
    bitset = bitsetSys->createBitset((max_object_count / 8) + 1);
    assert(sizeof(T) <= VKCONST_VIRMEMPAGESIZE && "Size of the struct is bigger than page size");
    commitNewPage();
  }
  T* create_OBJ() {
    uint32_t bitIndx = bitsetSys->getFirstBitIndx(bitset, false);
    bitsetSys->setBit(bitset, bitIndx, true);
    if (bitIndx >= maxObjIndx.load()) {
      commitNewPage();
    }
    return &data[bitIndx];
  }
  void destroyObj(unsigned int objIndx) {
    bitsetSys->setBit(bitset, objIndx, false);
    data[objIndx] = {};
  }
  bool isValid(T* obj) {
    uintptr_t objINT = uintptr_t(obj), dataINT = uintptr_t(data);
    if (objINT < dataINT || (objINT - dataINT) % sizeof(T) ||
        objINT >= dataINT + (sizeof(T) * maxObjIndx.load())) {
      printer(result_tgfx_FAIL,
              "Given handle doesn't align with other elements in the array! You probably used a "
              "invalid arithmetic/write operations on the handle");
      return false;
    }
    uint32_t indx = getINDEXbyOBJ(obj);
    // Check if object is active
    if (bitsetSys->getBitValue(bitset, indx) == 1) {
      return true;
    }
    return false;
  }
  unsigned int size() const { return maxObjIndx.load(); }
  T* getOBJbyINDEX(unsigned int i) { return (isValid(&data[i])) ? (&data[i]) : (NULL); }
  T* operator[](unsigned int index) { return getOBJbyINDEX(index); }
  uint32_t getINDEXbyOBJ(T* obj) {
    uintptr_t offset = uintptr_t(obj) - uintptr_t(data);
    return offset / sizeof(T);
  }
};
#endif

template <typename T, typename TGFXHND>
T* getOBJ(TGFXHND hnd) {
#ifdef VK_USE_STD
  return ( T* )hnd;
  #else
  VKOBJHANDLE handle = *( VKOBJHANDLE* )&hnd;
#ifdef VULKAN_DEBUGGING
  if (handle.type != T::HANDLETYPE || handle.OBJ_memoffset == UINT32_MAX ||
      handle.OBJ_memoffset == 0) {
    printer(result_tgfx_FAIL,
            "Given handle type isn't matching! You probably used a invalid arithmetic/write "
            "operations on the handle");
    return NULL;
  }
#endif
  return ( T* )(( char* )VKCONST_VIRMEMSPACE_BEGIN + handle.OBJ_memoffset);
  #endif
}

template <typename TGFXHND, typename T>
TGFXHND getHANDLE(T* obj) {
  #ifdef VK_USE_STD
  return ( TGFXHND )obj;
  #else
  VKOBJHANDLE handle   = {};
  handle.type          = T::HANDLETYPE;
  handle.OBJ_memoffset = uintptr_t(obj) - uintptr_t(VKCONST_VIRMEMSPACE_BEGIN);
  if constexpr (std::is_function<typename std::remove_pointer<T>::type>::value) {
    handle.EXTRA_FLAGs = T::GET_EXTRAFLAGS(obj);
  }
  return *( TGFXHND* )&handle;
  #endif
}

#ifdef VK_USE_STD
template <typename T, typename TGFXHND>
class VK_ARRAY {
  std::vector<T> data;

 public:
  bool isValid(T* obj) const {
    for (uint32_t i = 0; i < data.size(); i++) {
      if (&data[i] == obj) {
        return true;
      }
    }
    return false;
  }
  void init(uint32_t size) {
    data.resize(size);
  }
  void     init(T* i_data, uint32_t size) {
    data = std::vector<T>(i_data, i_data + size);
  }
  T*       operator[](uint32_t i) { return &data[i]; }
  const T* operator[](uint32_t i) const { return &data[i]; }
  uint32_t size() const { return data.size(); }
};
#else
template <typename T, typename TGFXHND>
class VK_ARRAY {
  T*             data;
  uint32_t       count;

 public:
  bool isValid(T* obj) const {
    uintptr_t objINT = uintptr_t(obj), dataINT = uintptr_t(data);
    if (objINT < dataINT || (objINT - dataINT) % sizeof(T) ||
        objINT >= dataINT + (sizeof(T) * count)) {
      printer(result_tgfx_FAIL,
              "Given handle doesn't align with other elements in the array! You probably used a "
              "invalid arithmetic/write operations on the handle");
      return false;
    }
    return true;
  }
  void init(uint32_t size) {
    data = ( T* )VK_MEMOFFSET_TO_POINTER(vk_virmem::allocatePage(sizeof(T) * size));
    virmemsys->virtual_commit(data, sizeof(T) * size);
    count = size;
  }
  void init(T* i_data, uint32_t size) {
    data  = i_data;
    count = size;
  }
  T*       operator[](uint32_t i) { return &data[i]; }
  const T* operator[](uint32_t i) const { return &data[i]; }
  uint32_t size() const { return count; }
};
#endif

//--------------------------------------------------------------------------------------
//	VKOBJECT TUTORIAL:
// VKOBJECTs have 2 states; INVALID (destroyed or not created) and VALID (created and not deleted
// yet) If isDELETED = true, VK_LINEAR_OBJARRAY places previous and next free element's indexes as
// next data in the structure If you create an object from LINEAR_OBJARRAY, you should change every
// variable of the structure
//	because the LINEAR_OBJARRAY may corrupt your data because of deletion process
//--------------------------------------------------------------------------------------
// This is an example of a vulkan object structure that a LINEAR_OBJARRAY can create handles from
// So VKOBJECT structure should have following funcs: assignment operator, static GET_EXTRAFLAGS(),
// deleted new() operator
// and should have following variables: vk_handleType HANDLETYPE, std::atomic_bool
// isDELETED (should be first variable)
struct EXAMPLE_VKOBJECT_STRUCT {
  bool            isALIVE    = true;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::UNDEFINED;
  static uint16_t GET_EXTRAFLAGS(EXAMPLE_VKOBJECT_STRUCT* obj) { return 0; }

  uint64_t normal_objdata;
};