#include "vk_predefinitions.h"

#include <map>

#include "vk_resource.h"
#include <stdarg.h>

core_tgfx*             core_tgfx_main    = nullptr;
core_public*           core_vk           = nullptr;
renderer_public*       renderer          = nullptr;
gpudatamanager_public* contentManager    = nullptr;
imgui_vk*              imgui             = nullptr;
GPU_VKOBJ*             rendergpu         = nullptr;
threadingsys_tapi*     threadingsys      = nullptr;
unsigned int           threadcount       = 1;
gpuallocatorsys_vk*    gpu_allocator     = nullptr;
manager_vk*            queuesys          = nullptr;
virtualmemorysys_tapi* virmemsys         = nullptr;
profiler_tapi*         profilerSys       = nullptr;
bitsetsys_tapi*        bitsetSys         = nullptr;
VkInstance             VKGLOBAL_INSTANCE = VK_NULL_HANDLE;
VkApplicationInfo      VKGLOBAL_APPINFO;
tgfx_logCallback  printer_cb                   = nullptr;
vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_CURRENTFRAME = nullptr;
unsigned char          VKGLOBAL_FRAMEINDEX          = 0;

typedef struct error_structs {
  result_tgfx result;
  const char* output;
  error_structs(result_tgfx r, const char* o) : result(r), output(o) {}
} error_structs;
static error_structs errors[]{
  // If you can't find the index of the code, do arithmetic on line numbers! :D
  error_structs(result_tgfx_NOTCODED, "NO_OUTPUT_CODE!"),
  error_structs(result_tgfx_INVALIDARGUMENT, "Second order subpasses has invalid wait desc"),
  error_structs(result_tgfx_FAIL,
                "This type of wait description isn't supported by Create_DrawPass()!"),
  error_structs(result_tgfx_FAIL,
                "VulkanRenderer: Create_TransferPass() has failed because this type of TP creation "
                "isn't supported!"),
  error_structs(
    result_tgfx_FAIL,
    "GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets isn't valid!"),
  error_structs(result_tgfx_FAIL,
                "GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets "
                "isn't inherited from the DrawPass' Base Slotset!"),
  error_structs(result_tgfx_FAIL,
                "Check_WaitHandles() doesn't support this type, there is a possible memory bug!"),
  error_structs(result_tgfx_FAIL,
                "One of the passes waits for a subpass but its handle doesn't point to a main "
                "pass, there is a bug!"),
  error_structs(result_tgfx_FAIL, "Waited subpass isn't found in the draw pass!"),
  error_structs(result_tgfx_FAIL, "There is a bug in the backend, RenderNodes list isn't valid"),
  error_structs(result_tgfx_FAIL, "Bindingtable list has invalid handle!"),
  error_structs(result_tgfx_FAIL, "There are more descsets than supported!"),
  error_structs(result_tgfx_FAIL, "No descset is found!"),
  error_structs(result_tgfx_FAIL, "Subpass handle isn't valid!"),
  error_structs(result_tgfx_FAIL, "Object handle's type didn't match!"),
  error_structs(result_tgfx_FAIL,
                "This type of pass isn't supported to be checked for merge status!"),
  error_structs(result_tgfx_FAIL,
                "Pass wait description's type isn't supported, please report this!"),
  error_structs(result_tgfx_FAIL, "Pass wait description's target main pass handle is wrong!"),
  error_structs(
    result_tgfx_FAIL,
    "There are multiple of the same main pass type in a wait list, which isn't supported!"),
  error_structs(result_tgfx_FAIL,
                "SubTP wait handles couldn't be converted because subtp finding has failed! This "
                "is a backend issue, please report this"),
  error_structs(result_tgfx_FAIL,
                "Converting waits has failed because there is merged pass conflicts!"),
  error_structs(result_tgfx_INVALIDARGUMENT,
                "CreateSubTransferPassDescription() has invalid transfer type argument!")};
vk_uint32c errors_count = sizeof(errors) / sizeof(error_structs);

result_tgfx printer(unsigned int error_code) {
  printer_cb(errors[error_code].result, errors[error_code].output);
  return errors[error_code].result;
}
result_tgfx printer(result_tgfx failResult, const char* format, ...) {
  static constexpr uint32_t maxLetter = 1 << 12;
  char    buf[maxLetter] = {};

  va_list args;
  va_start(args, format);

  int bufIter = 0;
  uint32_t formatLen = strlen(format);
  for (uint32_t formatIter = 0; formatIter < formatLen; formatIter++) {
    if (format[formatIter] == '%' && formatIter + 1 <= formatLen) {
      formatIter++;
      if (format[formatIter] == 'd') {
        int i = va_arg(args, int);
        snprintf(&buf[bufIter], 4, "%d", i);
        bufIter += 4;
      } else if (format[formatIter] == 'c') {
        // A 'char' variable will be promoted to 'int'
        // A character literal in C is already 'int' by itself
        int c        = va_arg(args, int);
        buf[bufIter] = c;
        bufIter++;
      } else if (format[formatIter] == 'f') {
        double d = va_arg(args, double);
        snprintf(&buf[bufIter], 8, "%f", d);
        bufIter += 8;
      } else if (format[formatIter] == 's') {
        const char* s      = va_arg(args, const char*);
        int         strLen = strlen(s);
        if (strLen <= maxLetter - 1 - bufIter) {
          memcpy(&buf[bufIter], s, strLen);
          bufIter += strLen;
        }
      }
    } else {
      buf[bufIter++] = format[formatIter];
    }
  }

  va_end(args);

  printer_cb(failResult, buf);
  return failResult;
}

// This is the max element count of the VK_MAIN_ALLOCATOR's list array
uint32_t  VKCONST_VIRMEM_MAXALLOCCOUNT = 0;
uint64_t  VKCONST_VIRMEMPAGESIZE       = 0;
void*     VKCONST_VIRMEMSPACE_BEGIN    = nullptr;
uintptr_t begin_loc                    = 0;

struct VK_PAGEINFO {
  uint32_t isALIVE : 1;
  uint32_t isMERGED : 1; // If the page isn't alive, it may be merged. If it is merged, COUNT is 0.
  uint32_t PAGECOUNT : 30;
  VK_PAGEINFO() : PAGECOUNT(0), isALIVE(0), isMERGED(0) {}
};
struct VK_MAIN_ALLOCATOR {
  std::mutex allocation_lock;
  // 4GB of address space will be allocated
  // List is made to iterate over which pages are allocated
  VK_PAGEINFO* suballocations_list = nullptr;
};
static VK_MAIN_ALLOCATOR* allocator_main;

std::function<void()> VKGLOBAL_emptyCallback = []() {};

void vk_createBackendAllocator() {
  // Allocate enough space for all vulkan backend
  {
    VKCONST_VIRMEMSPACE_BEGIN = virmemsys->virtual_reserve(UINT32_MAX);
    VKCONST_VIRMEMPAGESIZE    = virmemsys->get_pagesize();
    virmemsys->virtual_commit(VKCONST_VIRMEMSPACE_BEGIN,
                              VKCONST_VIRMEMPAGESIZE * VKCONST_VIRMEM_MANAGERONLYPAGECOUNT);
    VKCONST_VIRMEM_MAXALLOCCOUNT =
      ((VKCONST_VIRMEM_MANAGERONLYPAGECOUNT * VKCONST_VIRMEMPAGESIZE) - sizeof(VK_MAIN_ALLOCATOR)) /
      sizeof(VK_PAGEINFO);
    begin_loc = uintptr_t(VKCONST_VIRMEMSPACE_BEGIN);
  }


  // Create main allocator (manages suballocations)
  {
    allocator_main  = ( VK_MAIN_ALLOCATOR* )VKCONST_VIRMEMSPACE_BEGIN;
    VK_MAIN_ALLOCATOR defaultInit;
    memcpy(allocator_main, &defaultInit, sizeof(VK_MAIN_ALLOCATOR));

    allocator_main = ( VK_MAIN_ALLOCATOR* )VKCONST_VIRMEMSPACE_BEGIN;
    allocator_main->suballocations_list =
      ( VK_PAGEINFO* )((( char* )allocator_main) + sizeof(VK_MAIN_ALLOCATOR));
    for (unsigned int i = 0; i < VKCONST_VIRMEM_MAXALLOCCOUNT; i++) {
      allocator_main->suballocations_list[i] = VK_PAGEINFO();
    }
  }

  VKGLOBAL_VIRMEM_CURRENTFRAME = vk_virmem::allocate_dynamicmem(VKCONST_VIRMEM_PERFRAME_PAGECOUNT);
}

// Returns memory offset from the start of the memory allocation (which is mem ptr)
uint32_t vk_virmem::allocatePage(uint32_t requestedSize, uint32_t* roundedUpSize) {
  // Calculate required page size
  {
    std::unique_lock<std::mutex> lock(allocator_main->allocation_lock);
    requestedSize += VKCONST_VIRMEMPAGESIZE - (requestedSize % VKCONST_VIRMEMPAGESIZE);
    if (roundedUpSize) {
      *roundedUpSize = requestedSize;
    }
  }
  uint32_t requestedPageCount = requestedSize / VKCONST_VIRMEMPAGESIZE;

  // Find or create an enough sub-allocation
  unsigned int alloc_i = 0, page_i = VKCONST_VIRMEM_MANAGERONLYPAGECOUNT;
  while (alloc_i < VKCONST_VIRMEM_MAXALLOCCOUNT - 1) {
    VK_PAGEINFO& current_page = allocator_main->suballocations_list[alloc_i];
    if (current_page.isALIVE) {
      page_i += current_page.PAGECOUNT;
      alloc_i++;
      continue;
    }
    if (current_page.isMERGED) {
      alloc_i++;
      continue;
    }
    VK_PAGEINFO& nextpage = allocator_main->suballocations_list[alloc_i + 1];
    if (current_page.PAGECOUNT >= requestedPageCount) {
      if (!nextpage.isALIVE && nextpage.isMERGED && current_page.PAGECOUNT - requestedPageCount) {
        nextpage.isMERGED  = false;
        nextpage.PAGECOUNT = current_page.PAGECOUNT - requestedPageCount;
      }
      current_page.PAGECOUNT = requestedPageCount;
      current_page.isALIVE   = true;
      return VKCONST_VIRMEMPAGESIZE * page_i;
    }
    if (!current_page.PAGECOUNT) {
      // Zero initialize the next element, because next allocation will be on it
      nextpage.PAGECOUNT = 0;
      nextpage.isALIVE   = 0;
      nextpage.isMERGED  = 0;

      current_page.PAGECOUNT = requestedPageCount;
      current_page.isALIVE   = true;
      return VKCONST_VIRMEMPAGESIZE * page_i;
    } else {
      printer(result_tgfx_FAIL, "Backend page allocation fail!");
      return UINT32_MAX;
    }
  }
  if (alloc_i == VKCONST_VIRMEM_MAXALLOCCOUNT - 1) {
    printer(result_tgfx_FAIL,
            "You exceeded Max Memory Allocation Count which is super weird. You should give more "
            "pages to the VK backend's own memory manager!");
  }
  return UINT32_MAX;
}
// Frees memory suballocation
// Don't need size because size is stored in VK_PAGEINFO list
void vk_virmem::free_page(uint32_t suballocation_startoffset) {
  std::unique_lock<std::mutex> lock(allocator_main->allocation_lock);
#ifdef VULKAN_DEBUGGING
  if (suballocation_startoffset % VKCONST_VIRMEMPAGESIZE) {
    printer(result_tgfx_FAIL, "VK backend tried to free a suballocation wrong!");
    return;
  }
#endif
  uint32_t page_i = suballocation_startoffset / VKCONST_VIRMEMPAGESIZE, search_alloc_i = 0,
           search_page_sum = VKCONST_VIRMEM_MANAGERONLYPAGECOUNT;
  while (search_alloc_i < VKCONST_VIRMEM_MAXALLOCCOUNT - 1 && page_i > search_page_sum) {
    search_page_sum += allocator_main->suballocations_list[search_alloc_i].PAGECOUNT;
    search_alloc_i++;
  }
  VK_PAGEINFO& alloc = allocator_main->suballocations_list[search_alloc_i];
  void*        free_address =
    ( void* )((VKCONST_VIRMEMPAGESIZE * page_i) + uintptr_t(VKCONST_VIRMEMSPACE_BEGIN));
  virmemsys->virtual_decommit(free_address, VKCONST_VIRMEMPAGESIZE * alloc.PAGECOUNT);
  alloc.isALIVE  = false;
  alloc.isMERGED = false;
}
struct vk_virmem::dynamicmem {
  uint32_t             all_space       = 0;
  std::atomic_uint32_t remaining_space = 0;
};
vk_virmem::dynamicmem* vk_virmem::allocate_dynamicmem(uint32_t size, uint32_t* dynamicmemstart) {
  uint32_t               roundedUpSize = 0;
  uint32_t               start         = allocatePage(size, &roundedUpSize);
  vk_virmem::dynamicmem* dynamicmem =
    ( vk_virmem::dynamicmem* )(uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + start);
  virmemsys->virtual_commit(dynamicmem, VKCONST_VIRMEMPAGESIZE);
  dynamicmem->all_space = roundedUpSize;
  dynamicmem->remaining_space.store(dynamicmem->all_space - sizeof(vk_virmem::dynamicmem));
  if (dynamicmemstart) {
    *dynamicmemstart = start + sizeof(vk_virmem::dynamicmem);
  }
  return dynamicmem;
}
uint32_t vk_virmem::allocate_from_dynamicmem(dynamicmem* mem, uint32_t size, bool shouldcommit) {
#ifdef VULKAN_DEBUGGING
  uint32_t remaining = mem->remaining_space.load();
  if (remaining < size) {
    printer(result_tgfx_FAIL,
            "Vulkan backend's intended memory allocation is bigger than suballocation's size, "
            "please report this issue");
    return NULL;
  }
  while (!mem->remaining_space.compare_exchange_strong(remaining, remaining - size)) {
    remaining = mem->remaining_space.load();
    if (remaining < size) {
      printer(result_tgfx_FAIL,
              "Vulkan backend's intended memory allocation is bigger than suballocation's size, "
              "please report this issue");
      return NULL;
    }
  }
#else
  uint32_t remaining = mem->remaining_space.fetch_sub(size);
  if (remaining < size) {
    printer(result_tgfx_FAIL,
            "Vulkan backend's intended memory allocation is bigger than suballocation's size, "
            "please report this issue");
    return NULL;
  }
#endif
  uintptr_t loc_of_base_mem = uintptr_t(mem);
  uintptr_t location        = (loc_of_base_mem + sizeof(dynamicmem) + mem->all_space - remaining);
  if (shouldcommit) {
    virmemsys->virtual_commit(( void* )location, size);
  }
  return location - uintptr_t(VKCONST_VIRMEMSPACE_BEGIN);
}
void vk_virmem::free_dynamicmem(dynamicmem* mem) {
  free_page(uintptr_t(mem) - uintptr_t(VKCONST_VIRMEMSPACE_BEGIN));
}

void pNext_addToLast(void* targetStruct, void* attachStruct) {
  // pNext is always second member of a struct
  void** target_pNext = ( void** )((( VkStructureType* )targetStruct) + 2);
  // Iterate until you find last pNext filled
  while (*target_pNext) {
    target_pNext = ( void** )((( VkStructureType* )*target_pNext) + 2);
  }
  *target_pNext = attachStruct;
}
/*
void* operator new(size_t size) {
  printer(result_tgfx_FAIL, "Default new() operator shouldn't be called by backend");
  assert_vk(0);
  return nullptr;
}
void* operator new[](size_t size) {
  printer(result_tgfx_FAIL, "Default new() operator shouldn't be called by backend");
  assert_vk(0);
  return nullptr;
}*/
void* operator new(size_t size, vk_virmem::dynamicmem* mem) {
  return VK_ALLOCATE_AND_GETPTR(mem, size);
}
void* operator new[](size_t size, vk_virmem::dynamicmem* mem) {
  return VK_ALLOCATE_AND_GETPTR(mem, size);
}