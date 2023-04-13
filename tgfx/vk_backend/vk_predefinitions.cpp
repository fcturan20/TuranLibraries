#include "vk_predefinitions.h"

#include "string_tapi.h"
#include "vk_resource.h"

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
stringSys_tapi*        stringSys         = nullptr;
VkInstance             VKGLOBAL_INSTANCE = VK_NULL_HANDLE;
VkApplicationInfo      VKGLOBAL_APPINFO;
tgfx_logCallback       printer_cb                   = nullptr;
vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_CURRENTFRAME = nullptr;
unsigned char          VKGLOBAL_FRAMEINDEX          = 0;

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
    allocator_main = ( VK_MAIN_ALLOCATOR* )VKCONST_VIRMEMSPACE_BEGIN;
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
      vkPrint(3);
      return UINT32_MAX;
    }
  }
  if (alloc_i == VKCONST_VIRMEM_MAXALLOCCOUNT - 1) {
    vkPrint(4);
  }
  return UINT32_MAX;
}
// Frees memory suballocation
// Don't need size because size is stored in VK_PAGEINFO list
void vk_virmem::free_page(uint32_t suballocation_startoffset) {
  std::unique_lock<std::mutex> lock(allocator_main->allocation_lock);
#ifdef VULKAN_DEBUGGING
  if (suballocation_startoffset % VKCONST_VIRMEMPAGESIZE) {
    vkPrint(5);
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
    vkPrint(6);
    return UINT32_MAX;
  }
  while (!mem->remaining_space.compare_exchange_strong(remaining, remaining - size)) {
    remaining = mem->remaining_space.load();
    if (remaining < size) {
      vkPrint(6);
      return UINT32_MAX;
    }
  }
#else
  uint32_t remaining = mem->remaining_space.fetch_sub(size);
  if (remaining < size) {
    vkPrint(6);
    return UINT32_MAX;
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
result_tgfx vkPrint(unsigned int logCode, const wchar_t* extraInfo)
{
  printer_cb(logCode, extraInfo);
  return core_tgfx_main->getLogMessage(logCode, nullptr);
}