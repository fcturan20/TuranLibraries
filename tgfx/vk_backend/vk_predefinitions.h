#pragma once
// Vulkan Libs
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <assert.h>
#include <stdint.h>
#include <tgfx_core.h>
#include <tgfx_forwarddeclarations.h>
#include <virtualmemorysys_tapi.h>

#include <atomic>
#include <functional>
#include <limits>
#include <mutex>
#include <stdexcept>

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
extern gpudatamanager_public* contentmanager;
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

result_tgfx                  printer(unsigned int error_code);
result_tgfx                  printer(result_tgfx failresult, const char* output);
extern tgfx_PrintLogCallback printer_cb;

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

extern std::function<void()> VKGLOBAL_emptyCallback;
// If you want to throw error if the func doesn't return VK_SUCCESS, don't use throwResultFromCall
// argument If you want to give a specific TGFXResult to printer, you should use returnTGFXResult
// argument This function throws std::exception in Release build if func isn't VK_SUCCESS and
// doesn't use printer() func
inline bool ThrowIfFailed(VkResult func, const char* errorstring,
                          result_tgfx            returnTGFXResult    = result_tgfx_FAIL,
                          VkResult               throwResultFromCall = VK_RESULT_MAX_ENUM,
                          std::function<void()>& callback            = VKGLOBAL_emptyCallback) {
  callback();
#ifdef VULKAN_DEBUGGING
  if (throwResultFromCall != VK_RESULT_MAX_ENUM && func == throwResultFromCall) {
    printer(returnTGFXResult, errorstring);
    return true;
  }
  if (func != VK_SUCCESS) {
    printer(returnTGFXResult, errorstring);
  }
#else
  if (func != VK_SUCCESS) {
    std::runtime_error(std::string(errorstring));
  }
#endif
  return false;
}
#define THROW_RETURN_IF_FAIL(vkCall, errorString, returnTGFXResult) \
  if (ThrowIfFailed(vkCall, errorString, returnTGFXResult)) {       \
    return returnTGFXResult;                                        \
  }

void pNext_addToLast(void* targetStruct, void* attachStruct);

#define VK_PRIM_MIN(primType) std::numeric_limits<primType>::min()
#define VK_PRIM_MAX(primType) std::numeric_limits<primType>::max()

// Use this pre-processor in .cpp files to load extension functions
// Backend is maintainly on Vulkan 1.0 because most of the extensions can be used in it
// Search "Requires support for Vulkan 1.*" in VK Specification to learn minimum API Version
#define loadVkExtFunc(funcName)                                                               \
  PFN_##funcName funcName##_loadVkFunc() {                                                    \
    auto func = ( PFN_##funcName )vkGetInstanceProcAddr(VKGLOBAL_INSTANCE, #funcName);        \
    if (func == nullptr) {                                                                    \
      printer(result_tgfx_FAIL,                                                               \
              "Vulkan failed to load " #funcName " function! Probably an OS update problem"); \
      return nullptr;                                                                         \
    }                                                                                         \
    return func;                                                                              \
  };                                                                                          \
  PFN_##funcName funcName##_loaded = nullptr;

// <------------------------------------------------------------------------------------>
//		CONSTANTS
// Note: For name convenience, all constant should start with VKCONST_
// <------------------------------------------------------------------------------------>

#define vk_uint32c static constexpr uint32_t
#define vk_float32c static constexpr float32_t
#define vk_handleType static constexpr VKHANDLETYPEs
#define vk_uint8c static constexpr uint8_t
// Dynamic Descriptor Types supported by Vulkan
vk_uint32c VKCONST_DYNAMICDESCRIPTORTYPESCOUNT = 4, VKCONST_DESCSETID_DYNAMICSAMPLER = 0,
           VKCONST_DESCSETID_SAMPLEDTEXTURE = 1, VKCONST_DESCSETID_STORAGEIMAGE = 2,
           VKCONST_DESCSETID_BUFFER = 3; // Dynamic Samplers, Storage Image, Sampled Texture, Buffer
// User can pass lots of descriptor sets in a list, this defines the max size
vk_uint32c VKCONST_MAXDESCSET_PERLIST          = 16;
vk_uint32c VKCONST_MAXWINDOWCOUNT              = 16;
vk_uint32c VKCONST_MAXVIEWPORTCOUNT            = 16;
vk_uint32c VKCONST_MAXGPUCOUNT                 = 4;
vk_uint32c VKCONST_MAXRTSLOTCOUNT              = 16;
vk_uint32c VKCONST_MAXCMDBUFFERCOUNT_PERSUBMIT = 16;
vk_uint32c VKCONST_MAXQUEUEFAMCOUNT_PERGPU = 5, VKCONST_MAXQUEUECOUNT_PERFAM = 16;
vk_uint32c VKCONST_MAXSEMAPHORECOUNT_PERSUBMIT = 16;
vk_uint32c VKCONST_MAXSWPCHNCOUNT_PERSUBMIT    = 8; // Max count of swapchain count per submit

// VIRMEM RELATED

vk_uint8c  VKCONST_isPointerContainVKFLAG         = (sizeof(void*) >= sizeof(VkFlags));
vk_uint32c VKCONST_MAXSWPCHNTXTURECOUNT_PERWINDOW = 4;
// Memory manager reserves some of the first virtual memory pages for its own use
vk_uint8c  VKCONST_VIRMEM_MANAGERONLYPAGECOUNT = 2;
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

void* operator new(size_t size);
void* operator new[](size_t size);
void* operator new(size_t size, vk_virmem::dynamicmem* mem);
void* operator new[](size_t size, vk_virmem::dynamicmem* mem);

#define VK_MEMOFFSET_TO_POINTER(offset) (( void* )(uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + (size_t)offset))
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
  PIPELINE
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
  static_assert(sizeof(T) >= 9,
                "VK_LINEAR_OBJARRAY places previous and next free object's index if the object is "
                "deleted, so VKOBJ struct should have at least 9 bytes");
  static_assert(T::HANDLETYPE != VKHANDLETYPEs::UNDEFINED, "VKOBJ's type shouldn't be UNDEFINED");
  static constexpr unsigned int ELEMENTCOUNT_PERPAGE =
    (1 << 12) / sizeof(T); // It is assumed that page size is 4KB
  T*                   data                = NULL;
  uint32_t             OBJCOUNT            = max_object_count;
  std::atomic_uint32_t REMAINING_OBJ_COUNT = 0, MAX_OBJECT_INDEX = 0, LAST_FREED_ELEMENT = 0;
  inline void          link_two_frees(uint32_t previous_element_i, uint32_t next_element_i) {
             std::atomic_uint32_t* nextatomic_of_previous_one = (( char* )&data[previous_element_i]) + 5;
             std::atomic_uint32_t* previousatomic_of_next_one = (( char* )&data[next_element_i]) + 1;
             nextatomic_of_previous_one->store(next_element_i);
             previousatomic_of_next_one->store(previous_element_i);
  }

 public:
  // Give a reserved space for the array
  // Don't default initialize objects, because space is not committed
  VK_LINEAR_OBJARRAY(uint32_t datastart, uint32_t max_objects_count) {
    OBJCOUNT = (max_objects_count % ELEMENTCOUNT_PERPAGE)
      ? (max_objects_count + ELEMENTCOUNT_PERPAGE - (max_objects_count % ELEMENTCOUNT_PERPAGE));
    data = ( T* )(uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + datastart);
    virmemsys->virtual_commit(data, VKCONST_VIRMEMPAGESIZE);
    REMAINING_OBJ_COUNT.store(OBJCOUNT);
  }
  // Give a reserved space for the array
  // Don't default initialize objects, because space is not committed
  VK_LINEAR_OBJARRAY(uint32_t datastart) {
    OBJCOUNT = (max_object_count % ELEMENTCOUNT_PERPAGE)
      ? (max_object_count + ELEMENTCOUNT_PERPAGE - (max_object_count % ELEMENTCOUNT_PERPAGE));
    data = ( T* )(uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + datastart);
    virmemsys->virtual_commit(data, VKCONST_VIRMEMPAGESIZE);
    REMAINING_OBJ_COUNT.store(OBJCOUNT);
  }
  // Allocate enough pages from main allocator directly
  // Don't default initialize objects, because space is not committed
  VK_LINEAR_OBJARRAY() {
    OBJCOUNT =
      (max_object_count % ELEMENTCOUNT_PERPAGE)
        ? (max_object_count + ELEMENTCOUNT_PERPAGE - (max_object_count % ELEMENTCOUNT_PERPAGE))
        : (max_object_count);
    data = ( T* )VK_MEMOFFSET_TO_POINTER(
      vk_virmem::allocatePage((OBJCOUNT * sizeof(T) / VKCONST_VIRMEMPAGESIZE) + 1));
    virmemsys->virtual_commit(data, VKCONST_VIRMEMPAGESIZE);
    REMAINING_OBJ_COUNT.store(OBJCOUNT);
  }
  VK_LINEAR_OBJARRAY<T, TGFXHND, max_object_count>& operator=(
    const VK_LINEAR_OBJARRAY<T, TGFXHND, max_object_count>& copyFrom) {
    data     = copyFrom.data;
    OBJCOUNT = copyFrom.OBJCOUNT;
    REMAINING_OBJ_COUNT.store(copyFrom.REMAINING_OBJ_COUNT.load());
    MAX_OBJECT_INDEX.store(REMAINING_OBJ_COUNT.load());
    LAST_FREED_ELEMENT.store(copyFrom.LAST_FREED_ELEMENT.load());
  }
  T* create_OBJ() {
    if (MAX_OBJECT_INDEX.load() % ELEMENTCOUNT_PERPAGE == ELEMENTCOUNT_PERPAGE - 1) {
      virmemsys->virtual_commit(&data[ELEMENTCOUNT_PERPAGE], VKCONST_VIRMEMPAGESIZE);
    }
    if (MAX_OBJECT_INDEX.load() != OBJCOUNT) {
      unsigned int i  = MAX_OBJECT_INDEX.fetch_add(1);
      data[i].isALIVE = true;
      return &data[i];
    }
    printer(result_tgfx_FAIL, "Reaching the VK_LINEAR_OBJARRAY limit isn't supported for now!");
    return nullptr;
  }
  void destroyOBJfromHANDLE(VKOBJHANDLE handle) {
#ifdef VULKAN_DEBUGGING
    if (handle.type != T::HANDLETYPE || handle.OBJ_memoffset == UINT32_MAX ||
        (handle.OBJ_memoffset + uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) - uintptr_t(data)) %
          (sizeof(T)) ||
        ( T* )(( char* )VKCONST_VIRMEMSPACE_BEGIN + handle.OBJ_memoffset) < data ||
        ( T* )(( char* )VKCONST_VIRMEMSPACE_BEGIN + handle.OBJ_memoffset) > data + OBJCOUNT) {
      printer(result_tgfx_FAIL,
              "Given handle is invalid! You probably used a invalid arithmetic/write operations on "
              "the handle");
    }
#endif
    T* obj = ( T* )VK_MEMOFFSET_TO_POINTER(handle.OBJ_memoffset);
#ifdef VULKAN_DEBUGGING
    if (!obj->isALIVE) {
      printer(result_tgfx_FAIL,
              "Given handle is probably deleted but you try to delete it anyway, so please check "
              "your destroy calls!");
      return;
    }
#endif
    obj->isALIVE = (0);
  }
  T* getOBJfromHANDLE(TGFXHND hnd) {
    VKOBJHANDLE handle = *( VKOBJHANDLE* )&hnd;
#ifdef VULKAN_DEBUGGING
    if (handle.type != T::HANDLETYPE || handle.OBJ_memoffset == UINT32_MAX ||
        (handle.OBJ_memoffset + uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) - uintptr_t(data)) %
          (sizeof(T)) ||
        ( T* )(( char* )VKCONST_VIRMEMSPACE_BEGIN + handle.OBJ_memoffset) < data ||
        ( T* )(( char* )VKCONST_VIRMEMSPACE_BEGIN + handle.OBJ_memoffset) > data + OBJCOUNT) {
      printer(result_tgfx_FAIL,
              "Given handle is invalid! You probably used a invalid arithmetic/write operations on "
              "the handle");
      return NULL;
    }
#endif
    T* obj = ( T* )(( char* )VKCONST_VIRMEMSPACE_BEGIN + handle.OBJ_memoffset);
#ifdef VULKAN_DEBUGGING
    if (!obj->isALIVE) {
      printer(result_tgfx_FAIL,
              "Given handle is probably deleted but you try to use it anyway, so please check your "
              "handle lifetimes!");
      return NULL;
    }
#endif
    return obj;
  }
  TGFXHND returnHANDLEfromOBJ(T* obj) {
    VKOBJHANDLE handle;
    handle.type          = T::HANDLETYPE;
    handle.OBJ_memoffset = uintptr_t(obj) - uintptr_t(VKCONST_VIRMEMSPACE_BEGIN);
    handle.EXTRA_FLAGs   = T::GET_EXTRAFLAGS(obj);
    return *( TGFXHND* )&handle;
  }
  unsigned int size() const { return MAX_OBJECT_INDEX.load(); }
  T*           getOBJbyINDEX(unsigned int i) { return (data[i].isALIVE) ? (&data[i]) : (NULL); }
  T*           operator[](unsigned int index) { return getOBJbyINDEX(index); }
  uint32_t     getINDEXbyOBJ(T* obj) {
#ifdef VULKAN_DEBUGGING
    uint32_t offset = (uintptr_t(obj) - uintptr_t(data));
    if (offset % sizeof(T)) {
      return UINT32_MAX;
    }
    return offset / sizeof(T);
#else
    return ((uintptr_t(obj) - uintptr_t(data))) / sizeof(T);
#endif
  }
};

// Use this for structs that won't add new-delete elements frequently and small list
// It is recommended to not use fancy constructors or assignment operators on the template class
template <typename T, typename TGFXHND, uint16_t maxelementcount>
class VK_STATICVECTOR {
  T                    datas[maxelementcount]{};
  std::atomic_uint16_t currentelement_i = 0;

 public:
  // Returns the index of the object
  unsigned int push_back(const T& obj) {
    T* newObj = add();
    *newObj   = obj;
    return getINDEX_byOBJ(newObj);
  }
  T* operator[](unsigned int i) {
#ifdef VULKAN_DEBUGGING
    if (i >= currentelement_i.load()) {
      printer(result_tgfx_FAIL, "There is no such element in the VK_VECTOR!");
    }
#endif
    return &datas[i];
  }
  const T* operator[](unsigned int i) const {
#ifdef VULKAN_DEBUGGING
    if (i >= currentelement_i.load()) {
      printer(result_tgfx_FAIL, "There is no such element in the VK_VECTOR!");
    }
#endif
    return &datas[i];
  }
  const T* get(unsigned int i) const { return (*this)[i]; }
  T*       data() { return datas; }
  void     erase(int i) {
#ifdef VULKAN_DEBUGGING
    if (i >= currentelement_i.load()) {
      printer(result_tgfx_FAIL, "There is no such element in the VK_VECTOR!");
    }
#endif
    memmove(&datas[i], &datas[i + 1], sizeof(T) * (currentelement_i.fetch_sub(1) - 1 - i));
  }
  void clear() {
    currentelement_i.store(0);
    for (uint32_t i = 0; i < maxelementcount; i++) {
      datas[i] = T();
    }
  }
  unsigned int                  size() const { return currentelement_i.load(); }
  static constexpr unsigned int capacity() { return maxelementcount; }

  T* getOBJfromHANDLE(TGFXHND hnd) {
    VKOBJHANDLE handle = *( VKOBJHANDLE* )&hnd;
#ifdef VULKAN_DEBUGGING
    if (handle.type != T::HANDLETYPE || handle.OBJ_memoffset == UINT32_MAX) {
      printer(result_tgfx_FAIL,
              "Given handle type isn't matching! You probably used a invalid arithmetic/write "
              "operations on the handle");
      return NULL;
    }
    if ((handle.OBJ_memoffset - (uintptr_t(datas) - uintptr_t(VKCONST_VIRMEMSPACE_BEGIN))) %
        (sizeof(T))) {
      printer(result_tgfx_FAIL,
              "Given handle doesn't align with other elements in the array! You probably used a "
              "invalid arithmetic/write operations on the handle");
      return NULL;
    }
    if (( T* )(( char* )VKCONST_VIRMEMSPACE_BEGIN + handle.OBJ_memoffset) < datas ||
        ( T* )(( char* )VKCONST_VIRMEMSPACE_BEGIN + handle.OBJ_memoffset) >
          datas + currentelement_i.load()) {
      printer(result_tgfx_FAIL,
              "Given handle isn't in array range! You probably used a invalid arithmetic/write "
              "operations on the handle");
      return NULL;
    }
#endif
    T* obj = ( T* )(( char* )VKCONST_VIRMEMSPACE_BEGIN + handle.OBJ_memoffset);
#ifdef VULKAN_DEBUGGING
    if (!obj->isALIVE) {
      printer(result_tgfx_FAIL,
              "Given handle is probably deleted but you try to use it anyway, so please check your "
              "handle lifetimes!");
      return NULL;
    }
#endif
    return obj;
  }
  TGFXHND returnHANDLEfromOBJ(T* obj) const {
    VKOBJHANDLE handle;
    handle.type          = T::HANDLETYPE;
    handle.OBJ_memoffset = uintptr_t(obj) - uintptr_t(VKCONST_VIRMEMSPACE_BEGIN);
    handle.EXTRA_FLAGs   = T::GET_EXTRAFLAGS(obj);
    return *( TGFXHND* )&handle;
  }
  uint32_t getINDEX_byOBJ(T* obj) { return uintptr_t(obj - datas) / sizeof(T); }

  T* add() {
    uint32_t indx = currentelement_i.fetch_add(1);
    assert(indx != maxelementcount && "Static vector is exceeded!");
    T* added = (*this)[indx];
    if constexpr (!std::is_pointer_v<T>) {
      added->isALIVE = true;
    }
    return added;
  }
  TGFXHND* returnHANDLELIST() {
    uint32_t listSize = size();
    TGFXHND* list     = new (VKGLOBAL_VIRMEM_CURRENTFRAME) TGFXHND[listSize + 1];
    for (uint32_t i = 0; i < size(); i++) {
      list[i] = returnHANDLEfromOBJ((*this)[i]);
    }
    list[listSize] = ( TGFXHND )core_tgfx_main->INVALIDHANDLE;
    return list;
  }
};

// This is to store render commands
// So there is no erase, just clear all of the buffer
template <typename T, uint32_t maxelementcount>
class VK_VECTOR_ADDONLY {
  uint32_t               PTR;
  std::atomic_uint32_t   currentcmd_i  = 0;
  vk_virmem::dynamicmem* dynamicmemptr = nullptr;

 public:
  VK_VECTOR_ADDONLY() {
    dynamicmemptr =
      vk_virmem::allocate_dynamicmem((maxelementcount * sizeof(T)) / VKCONST_VIRMEMPAGESIZE, &PTR);
    virmemsys->virtual_commit(dynamicmemptr, VKCONST_VIRMEMPAGESIZE);
  }
  inline const VK_VECTOR_ADDONLY<T, maxelementcount>& operator=(
    const VK_VECTOR_ADDONLY<T, maxelementcount>& copyFrom) {
    PTR = copyFrom.PTR;
    currentcmd_i.store(copyFrom.currentcmd_i.load());
    dynamicmemptr = copyFrom.dynamicmemptr;
    return *this;
  }
  void push_back(const T& obj) {
    uint32_t cmd_i = currentcmd_i.load();
    if (cmd_i % (VKCONST_VIRMEMPAGESIZE / sizeof(T))) {
      virmemsys->virtual_commit((( char* )dynamicmemptr) + (sizeof(T) * cmd_i),
                                VKCONST_VIRMEMPAGESIZE);
    }
    T* data     = ( T* )VK_MEMOFFSET_TO_POINTER(PTR);
    data[cmd_i] = obj;
  }
  void clear() {
    virmemsys->virtual_decommit(VK_MEMOFFSET_TO_POINTER(PTR), currentcmd_i * sizeof(T));
    currentcmd_i = 0;
    virmemsys->virtual_commit(VK_MEMOFFSET_TO_POINTER(PTR), VKCONST_VIRMEMPAGESIZE);
  }
  // This function doesn't allocate new memory, just increments currentcmd_i
  void resize(uint32_t newsize) {
#ifdef VULKAN_DEBUGGING
    if (newsize > maxelementcount) {
      printer(result_tgfx_NOTCODED,
              "VK_VECTOR_ADDONLY doesn't allocate new memory but you're trying it!");
    }
#endif
    virmemsys->virtual_commit(dynamicmemptr, (newsize * sizeof(T)));
    currentcmd_i.store(newsize);
  }
  T&       operator[](uint32_t i) { return (( T* )VK_MEMOFFSET_TO_POINTER(PTR))[i]; }
  uint32_t size() { return currentcmd_i.load(); }
};

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