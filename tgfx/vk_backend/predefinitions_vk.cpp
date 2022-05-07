#include "predefinitions_vk.h"
#include "resource.h"

core_tgfx* core_tgfx_main = nullptr;
core_public* core_vk = nullptr;
renderer_public* renderer = nullptr;
gpudatamanager_public* contentmanager = nullptr;
imgui_vk* imgui = nullptr;
GPU_VKOBJ* rendergpu = nullptr;
threadingsys_tapi* threadingsys = nullptr;
unsigned int threadcount = 1;
semaphoresys_vk* semaphoresys = nullptr;
fencesys_vk* fencesys = nullptr;
gpuallocatorsys_vk* gpu_allocator = nullptr;
queuesys_vk* queuesys = nullptr;
virtualmemorysys_tapi* virmemsys = nullptr;
VkInstance Vulkan_Instance = VK_NULL_HANDLE;
VkApplicationInfo Application_Info;
tgfx_PrintLogCallback printer_cb = nullptr;
vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_CURRENTFRAME = nullptr;
unsigned char VKGLOBAL_FRAMEINDEX = 0;

typedef struct error_structs {
	result_tgfx result;
	const char* output;
	error_structs(result_tgfx r, const char* o) : result(r), output(o){}
} error_structs;
static error_structs errors[]{
	error_structs(result_tgfx_NOTCODED, "NO_OUTPUT_CODE!"),
	error_structs(result_tgfx_INVALIDARGUMENT, "Second order subpasses has invalid wait desc"),
	error_structs(result_tgfx_FAIL, "This type of wait description isn't supported by Create_DrawPass()!"),
	error_structs(result_tgfx_FAIL, "VulkanRenderer: Create_TransferPass() has failed because this type of TP creation isn't supported!"),
	error_structs(result_tgfx_FAIL, "GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets isn't valid!"),
	error_structs(result_tgfx_FAIL, "GFXRenderer->Create_DrawPass() has failed because one of the inherited slotsets isn't inherited from the DrawPass' Base Slotset!"),
	error_structs(result_tgfx_FAIL, "Check_WaitHandles() doesn't support this type, there is a possible memory bug!"),
	error_structs(result_tgfx_FAIL, "One of the passes waits for a subpass but its handle doesn't point to a main pass, there is a bug!"),
	error_structs(result_tgfx_FAIL, "Waited subpass isn't found in the draw pass!"),
	error_structs(result_tgfx_FAIL, "There is a bug in the backend, RenderNodes list isn't valid"),
	error_structs(result_tgfx_FAIL, "Bindingtable list has invalid handle!"),
	error_structs(result_tgfx_FAIL, "There are more descsets than supported!"),
	error_structs(result_tgfx_FAIL, "No descset is found!"),
	error_structs(result_tgfx_FAIL, "Subpass handle isn't valid!"),
	error_structs(result_tgfx_FAIL, "Object handle's type didn't match!")
};
static constexpr uint32_t errors_count = sizeof(errors) / sizeof(error_structs);

result_tgfx printer(unsigned int error_code) {
	printer_cb(errors[error_code].result, errors[error_code].output);
	return errors[error_code].result;
}
result_tgfx printer(result_tgfx r, const char* output) {
	printer_cb(r, output);
	return r;
}

//This is the max element count of the VK_MAIN_ALLOCATOR's list array
uint32_t VKCONST_VIRMEM_MAXALLOCCOUNT = 0;
uint64_t VKCONST_VIRMEMPAGESIZE = 0;
void* VKCONST_VIRMEMSPACE_BEGIN = nullptr;
uintptr_t begin_loc = 0;

#include "tgfx_core.h"
#include <string>

struct VK_PAGEINFO {
	uint32_t isALIVE : 1;
	uint32_t isMERGED : 1;	//If the page isn't alive, it may be merged. If it is merged, COUNT is 0. 
	uint32_t PAGECOUNT : 30;
	VK_PAGEINFO() : PAGECOUNT(0), isALIVE(0), isMERGED(0) {}
};
struct VK_MAIN_ALLOCATOR {
	std::mutex allocation_lock;
	//4GB of address space will be allocated
	//List is made to iterate over which pages are allocated
	VK_PAGEINFO* suballocations_list = nullptr;
};
static VK_MAIN_ALLOCATOR* allocator_main;


void Create_BackendAllocator() {
	VKCONST_VIRMEMSPACE_BEGIN = virmemsys->virtual_reserve(UINT32_MAX);
	VKCONST_VIRMEMPAGESIZE = virmemsys->get_pagesize();
	virmemsys->virtual_commit(VKCONST_VIRMEMSPACE_BEGIN, VKCONST_VIRMEMPAGESIZE * VKCONST_VIRMEM_MANAGERONLYPAGECOUNT);
	VKCONST_VIRMEM_MAXALLOCCOUNT = ((VKCONST_VIRMEM_MANAGERONLYPAGECOUNT * VKCONST_VIRMEMPAGESIZE) - sizeof(VK_MAIN_ALLOCATOR)) / sizeof(VK_PAGEINFO);
	begin_loc = uintptr_t(VKCONST_VIRMEMSPACE_BEGIN);

	VK_MAIN_ALLOCATOR mainalloc;
	memcpy(VKCONST_VIRMEMSPACE_BEGIN, &mainalloc, sizeof(VK_MAIN_ALLOCATOR));

	
	allocator_main = (VK_MAIN_ALLOCATOR*)VKCONST_VIRMEMSPACE_BEGIN;
	allocator_main->suballocations_list = (VK_PAGEINFO*)(((char*)allocator_main) + sizeof(VK_MAIN_ALLOCATOR));
	for (unsigned int i = 0; i < VKCONST_VIRMEM_MAXALLOCCOUNT; i++) {
		allocator_main->suballocations_list[i] = VK_PAGEINFO();
	}

	//Place INVALIDHANDLE at the end of the memory reservation
	//So it's less possible to touch the invalid handle with wrong pointer arithmetic in backend
	//Because backend probably won't use 4GBs of memory
	core_tgfx_main->INVALIDHANDLE = (void*)(uintptr_t(allocator_main) + UINT32_MAX + 1);

	VKGLOBAL_VIRMEM_CURRENTFRAME = vk_virmem::allocate_dynamicmem(VKCONST_VIRMEM_PERFRAME_PAGECOUNT);
}

//Returns memory offset from the start of the memory allocation (which is mem ptr)
uint32_t vk_virmem::allocate_page(uint32_t requested_pagecount) {
	std::unique_lock<std::mutex> lock(allocator_main->allocation_lock);
	unsigned int alloc_i = 0, page_i = VKCONST_VIRMEM_MANAGERONLYPAGECOUNT;
	while (alloc_i < VKCONST_VIRMEM_MAXALLOCCOUNT - 1) {
		VK_PAGEINFO& current_page = allocator_main->suballocations_list[alloc_i];
		if (current_page.isALIVE) { page_i += current_page.PAGECOUNT; alloc_i++; continue; }
		if (current_page.isMERGED) { alloc_i++; continue; }
		VK_PAGEINFO& nextpage = allocator_main->suballocations_list[alloc_i + 1];
		if (current_page.PAGECOUNT >= requested_pagecount) {
			if (!nextpage.isALIVE && nextpage.isMERGED && current_page.PAGECOUNT - requested_pagecount) { 
				nextpage.isMERGED = false; nextpage.PAGECOUNT = current_page.PAGECOUNT - requested_pagecount; current_page.PAGECOUNT = requested_pagecount; current_page.isALIVE = true;
			}
			return VKCONST_VIRMEMPAGESIZE * page_i;
		}
		if (!current_page.PAGECOUNT) {
			//Zero initialize the next element, because next allocation will be on it
			nextpage.PAGECOUNT = 0;
			nextpage.isALIVE = 0;
			nextpage.isMERGED = 0;

			current_page.PAGECOUNT = requested_pagecount;
			current_page.isALIVE = true;
			return VKCONST_VIRMEMPAGESIZE * page_i;
		}
	}
	if (alloc_i == VKCONST_VIRMEM_MAXALLOCCOUNT - 1) {
		printer(result_tgfx_FAIL, "You exceeded Max Memory Allocation Count which is super weird. You should give more pages to the VK backend's own memory manager!");
		return UINT32_MAX;
	}
}
//Frees memory suballocation
//Don't need size because size is stored in VK_PAGEINFO list
void vk_virmem::free_page(uint32_t suballocation_startoffset) {
	std::unique_lock<std::mutex> lock(allocator_main->allocation_lock);
#ifdef VULKAN_DEBUGGING
	if (suballocation_startoffset % VKCONST_VIRMEMPAGESIZE) { printer(result_tgfx_FAIL, "VK backend tried to free a suballocation wrong!"); return; }
#endif
	uint32_t page_i = suballocation_startoffset / VKCONST_VIRMEMPAGESIZE, search_alloc_i = 0, search_page_sum = VKCONST_VIRMEM_MANAGERONLYPAGECOUNT;
	while (search_alloc_i < VKCONST_VIRMEM_MAXALLOCCOUNT - 1 && page_i > search_page_sum) {
		search_page_sum += allocator_main->suballocations_list[search_alloc_i].PAGECOUNT;
		search_alloc_i++;
	}
	void* free_address = (void*)(uintptr_t(VKCONST_VIRMEM_MAXALLOCCOUNT) + (VKCONST_VIRMEMPAGESIZE * page_i));
	virmemsys->virtual_decommit(free_address, VKCONST_VIRMEMPAGESIZE * allocator_main->suballocations_list[search_alloc_i].PAGECOUNT);
}
struct vk_virmem::dynamicmem {
	uint32_t all_space = 0; 
	std::atomic_uint32_t remaining_space = 0;
};
vk_virmem::dynamicmem* vk_virmem::allocate_dynamicmem(uint32_t pagecount, uint32_t* dynamicmemstart) {
	uint32_t start = allocate_page(pagecount);
	vk_virmem::dynamicmem* dynamicmem = (vk_virmem::dynamicmem*)(uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + start);
	virmemsys->virtual_commit(dynamicmem, VKCONST_VIRMEMPAGESIZE);
	dynamicmem->all_space = pagecount * VKCONST_VIRMEMPAGESIZE;
	dynamicmem->remaining_space.store(dynamicmem->all_space);
	if (dynamicmemstart) { *dynamicmemstart = start + sizeof(vk_virmem::dynamicmem); }
	return dynamicmem;
}
uint32_t vk_virmem::allocate_from_dynamicmem(dynamicmem* mem, uint32_t size, bool shouldcommit) {
#ifdef VULKAN_DEBUGGING
	uint32_t remaining = mem->remaining_space.load();
	if (remaining < size) { printer(result_tgfx_FAIL, "Vulkan backend's intended memory allocation is bigger than suballocation's size, please report this issue"); return NULL; }
	while (!mem->remaining_space.compare_exchange_strong(remaining, remaining - size)) {
		remaining = mem->remaining_space.load();
		if (remaining < size) { printer(result_tgfx_FAIL, "Vulkan backend's intended memory allocation is bigger than suballocation's size, please report this issue"); return NULL; }
	}
	if (shouldcommit) {
		uintptr_t loc_of_base_mem = uintptr_t(mem);
		uintptr_t location = (loc_of_base_mem + sizeof(dynamicmem) + mem->all_space - remaining);
		virmemsys->virtual_commit((void*)location, size);
	}
	return uintptr_t(mem) - uintptr_t(VKCONST_VIRMEMSPACE_BEGIN) + sizeof(dynamicmem) + mem->all_space - remaining;
#else
	uint32_t remaining = mem->remaining_space.fetch_sub(size);
	if (remaining < size) { printer(result_tgfx_FAIL, "Vulkan backend's intended memory allocation is bigger than suballocation's size, please report this issue"); return NULL; }
	return mem->ending_offset - remaining;
#endif
}
void vk_virmem::free_dynamicmem(dynamicmem* mem) {
	free_page(uintptr_t(mem) - uintptr_t(VKCONST_VIRMEMSPACE_BEGIN));
}
