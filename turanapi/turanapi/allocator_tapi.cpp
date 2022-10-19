#include "allocator_tapi.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include "ecs_tapi.h"
#include "virtualmemorysys_tapi.h"
static constexpr uint64_t maxVectorAllocation_IfNotSpecified = uint64_t(1) << uint64_t(34);
static constexpr uint64_t maxSuperMemoryBlockName            = 62; // You should use null terminator
static constexpr uint64_t maxVirmemPerSuperBlock =
  1 << 24; // Reserved address space size for each super block's internal usage
static constexpr uint64_t maxSuperBlockCount         = 1 << 10;
static constexpr uint64_t maxVirmemForAllSuperBlocks = maxVirmemPerSuperBlock * maxSuperBlockCount;

// These can be gathered after reload too
static allocator_sys_tapi* sys;
static unsigned long long  pagesize = 0;
static allocator_tapi_d*   hidden   = nullptr;

// Virtual Memory Functions
extern "C" void*        virtual_reserve(unsigned long long size);
extern "C" void         virtual_commit(void* ptr, unsigned long long commitsize);
extern "C" void         virtual_decommit(void* ptr, unsigned long long size);
extern "C" void         virtual_free(void* ptr, unsigned long long commit);
extern "C" unsigned int get_pagesize();
extern "C" void*        get_virtualmemory(unsigned long long reserve_size,
                                          unsigned long long first_commit_size);

// Size of this structure should be approximately equal to maxVirmemPerSuperBlock
struct supermemoryblock_tapi {
  struct superBlockInfo {
    char               name[maxSuperMemoryBlockName];
    unsigned char      isFreed = true;
    unsigned long long blockSize;
    unsigned int       activeBlockCount = 0;
    void*              ptr              = nullptr;
  };
  superBlockInfo info;
  struct memBlock {
    bool         isFree = true;
    unsigned int size   = 0;
  };
  static constexpr uint32_t maxBlockCountPerSuperBlock =
    (maxVirmemPerSuperBlock - sizeof(superBlockInfo)) / sizeof(memBlock);
  memBlock blocks[maxBlockCountPerSuperBlock];
};

static uint32_t firstPageMemBlockCount;
static uint32_t memBlockCountPerPage;

typedef struct allocator_tapi_d {
  uint32_t               createdSuperBlockCount;
  supermemoryblock_tapi* superBlocks;
  allocator_tapi_d() : createdSuperBlockCount(0), superBlocks(nullptr) {
    superBlocks = ( supermemoryblock_tapi* )virtual_reserve(maxVirmemForAllSuperBlocks);
  }
} allocator_tapi_d;

supermemoryblock_tapi* createSuperMemoryBlock(unsigned long long blockSize,
                                              const char*        superMemBlockName) {
  void* alloc = virtual_reserve(blockSize);
  if (!alloc) {
    printf("Super Memory Block creation failed because virtual memory reserve failed\n");
    return nullptr;
  }

  // Search for a freed super memory block
  for (uint32_t i = 0; i < hidden->createdSuperBlockCount; i++) {
    supermemoryblock_tapi* super = &hidden->superBlocks[i];
    if (!super->info.isFreed) {
      continue;
    }

    super->info.blockSize = blockSize;
    super->info.isFreed   = false;
    super->info.ptr       = alloc;
    uint32_t namelen      = std::min(strlen(superMemBlockName), maxSuperMemoryBlockName);
    memcpy(super->info.name, superMemBlockName, namelen);
    return super;
  }

  // There is no freed super memory block, so allocate new one
  supermemoryblock_tapi* super = &hidden->superBlocks[hidden->createdSuperBlockCount++];
  virtual_commit(super, pagesize);
  super->info.blockSize = blockSize;
  super->info.isFreed   = false;
  super->info.ptr       = alloc;
  uint32_t namelen      = std::min(strlen(superMemBlockName), maxSuperMemoryBlockName);
  memcpy(super->info.name, superMemBlockName, namelen);

  for (unsigned int i = 0; i < firstPageMemBlockCount; i++) {
    super->blocks[i] = supermemoryblock_tapi::memBlock();
  }
  super->info.activeBlockCount = firstPageMemBlockCount;
  // Committed memory will be 0 initialized, so memBlocks will be 0 initialized too
  return super;
}
void freeSuperMemoryBlock(supermemoryblock_tapi* superMemBlock) {
  if (uintptr_t(superMemBlock) - uintptr_t(hidden->superBlocks) % sizeof(supermemoryblock_tapi)) {
    printf("freeSuperMemoryBlock failed because super memory block pointer is invalid!");
  }
  // Free all memory that superMemBlock externally has
  virtual_free(superMemBlock->info.ptr, superMemBlock->info.blockSize);
  // Decommit superMemBlock's internal memory block infos except first page
  virtual_decommit(( void* )(uintptr_t(superMemBlock) + pagesize),
                   sizeof(supermemoryblock_tapi) - pagesize);
  // Default initialize first page's memory block infos
  for (unsigned int i = 0; i < firstPageMemBlockCount; i++) {
    superMemBlock->blocks[i] = supermemoryblock_tapi::memBlock();
  }
  // Info: Set as freed, memset name as 0
  superMemBlock->info.activeBlockCount = firstPageMemBlockCount;
  superMemBlock->info.blockSize        = 0;
  superMemBlock->info.isFreed          = true;
  memset(superMemBlock->info.name, 0, maxSuperMemoryBlockName);
  superMemBlock->info.ptr = nullptr;
}
void* allocateFromSuperMemoryBlock(supermemoryblock_tapi* superMemBlock, unsigned int blockSize) {
  if ((uintptr_t(superMemBlock) - uintptr_t(hidden->superBlocks)) % sizeof(supermemoryblock_tapi)) {
    printf("allocateFromSuperMemoryBlock failed because super memory block pointer is invalid!\n");
  }
  uintptr_t lastVirmemAddress = reinterpret_cast<uintptr_t>(superMemBlock->info.ptr);
  uint32_t  memBlockIndex     = UINT32_MAX;
  for (uint32_t i = 0; i < superMemBlock->info.activeBlockCount; i++) {
    supermemoryblock_tapi::memBlock& curBlock = superMemBlock->blocks[i];
    // This is an unused block info
    if (curBlock.size == 0) {
      memBlockIndex = i;
      break;
    }
    if (curBlock.isFree && curBlock.size >= blockSize) {
      curBlock.isFree = false;
      return reinterpret_cast<void*>(lastVirmemAddress);
    }
    lastVirmemAddress += curBlock.size;
  }
  // If requested block can't fit (remaining space - last page), fail
  // Last page better be unused
  if (lastVirmemAddress + blockSize - reinterpret_cast<uintptr_t>(superMemBlock->info.ptr) >=
      superMemBlock->info.blockSize - pagesize) {
    printf("Your request can't fit in the Super Memory Block!\n");
    return nullptr;
  }
  // If all active memory blocks does allocate memory, create new block info with commiting a page
  if (memBlockIndex == UINT32_MAX) {
    memBlockIndex = superMemBlock->info.activeBlockCount++;
    virtual_commit(superMemBlock->blocks + memBlockIndex, pagesize);
    for (uint32_t i = 1; i < memBlockCountPerPage; i++) {
      superMemBlock->blocks[i + memBlockIndex] = supermemoryblock_tapi::memBlock();
    }
    superMemBlock->info.activeBlockCount += memBlockCountPerPage;
  }
  supermemoryblock_tapi::memBlock& finalBlock = superMemBlock->blocks[memBlockIndex];
  finalBlock.isFree                           = false;
  finalBlock.size                             = blockSize;
  return reinterpret_cast<void*>(lastVirmemAddress);
}
void freeFromSuperMemoryBlock(supermemoryblock_tapi* superMemBlock, void* allocation) {
  if (uintptr_t(superMemBlock) - uintptr_t(hidden->superBlocks) % sizeof(supermemoryblock_tapi)) {
    printf("freeSuperMemoryBlock failed because super memory block pointer is invalid!\n");
  }
  uintptr_t lastVirmemAddress = reinterpret_cast<uintptr_t>(superMemBlock->info.ptr);
  for (uint32_t i = 0; i < superMemBlock->info.activeBlockCount; i++) {
    supermemoryblock_tapi::memBlock& curBlock = superMemBlock->blocks[i];
    if (lastVirmemAddress == reinterpret_cast<uintptr_t>(allocation)) {
      curBlock.isFree = true;
      virtual_decommit(allocation, curBlock.size);
      return;
    }
    if (lastVirmemAddress >= reinterpret_cast<uintptr_t>(allocation) || !curBlock.size) {
      printf("Freeing from a Super Memory Block failed because of invalid allocation ptr!\n");
      return;
    }
    lastVirmemAddress += curBlock.size;
  }
}

// Based on The Machinery's end of page allocator
void* endofpage_malloc(supermemoryblock_tapi* memBlock, unsigned int size) {
  const unsigned long long requestedAllocSize = size + sizeof(unsigned int);
  // Requested size is rounded up to pagesize (these pages will be committed)
  //  Then adds a new page (this page won't be committed)
  const unsigned long end_of_page_size =
    requestedAllocSize + (pagesize - (requestedAllocSize % pagesize)) + pagesize;

  void* base = allocateFromSuperMemoryBlock(memBlock, end_of_page_size);
  virtual_commit(base, end_of_page_size - pagesize);
  // Memory Layout: offsettedSpace (less than a page) - sizeVariable (unsigned int) - data
  // (requestedSize) - reserved (non-committed) last page (page size)
  void* offsetted = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base) +
                                            (pagesize - (requestedAllocSize % pagesize)));
  // Store requested size right before actual data
  *((( unsigned int* )offsetted) - 1) = size + sizeof(unsigned int);
  return offsetted;
}
void endofpage_free(supermemoryblock_tapi* memBlock, void* returnedAllocPTR) {
  const uint32_t requestedAllocSize = *((( unsigned int* )returnedAllocPTR) - 1);
  if (!requestedAllocSize || requestedAllocSize == UINT32_MAX) {
    printf("End of Page allocator failed to free the allocation!");
    return;
  }
  const unsigned long end_of_page_size =
    requestedAllocSize + (pagesize - (requestedAllocSize % pagesize)) + pagesize;
  void* base = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(returnedAllocPTR) -
                                       (pagesize - (requestedAllocSize % pagesize)));
  freeFromSuperMemoryBlock(memBlock, base);
}

// Standard alloc
void* standard_malloc(supermemoryblock_tapi* memBlock, unsigned int size) {
  const unsigned long long requestedAllocSize = size + sizeof(unsigned int);

  void* base = allocateFromSuperMemoryBlock(memBlock, requestedAllocSize);
  virtual_commit(base, requestedAllocSize);
  return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base) + sizeof(unsigned int));
}
void standard_free(supermemoryblock_tapi* memBlock, void* returnedAllocPTR) {
  const uint32_t requestedAllocSize = *((( unsigned int* )returnedAllocPTR) - 1);
  if (!requestedAllocSize || requestedAllocSize == UINT32_MAX) {
    printf("Standard allocator failed to free the allocation!");
    return;
  }
  void* base =
    reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(returnedAllocPTR) - sizeof(unsigned int));
  freeFromSuperMemoryBlock(memBlock, base);
}

struct vector_tapi {
  unsigned char flag;
  unsigned int  count = 0, capacity = 0, elementSize = 0;
  // Functions (constructor etc) according to flag will be right after the vector_tapi
  // Data allocated right after the functions
};
vector_tapi* getVectorTapi(const void* data) {
  return reinterpret_cast<vector_tapi*>(reinterpret_cast<uintptr_t>(data) - sizeof(vector_tapi));
}
vector_element_constructor_func_tapi find_constructor(void* data) {
  uintptr_t loc = reinterpret_cast<uintptr_t>(getVectorTapi(data));
  return *reinterpret_cast<vector_element_constructor_func_tapi*>(loc - sizeof(void*));
}
vector_element_copy_func_tapi find_copyfunc(void* data) {
  uintptr_t    loc = reinterpret_cast<uintptr_t>(getVectorTapi(data));
  vector_tapi* hnd = getVectorTapi(data);
  if (hnd->flag & vector_flagbit_constructor_tapi) {
    loc -= sizeof(void*);
  }
  return *reinterpret_cast<vector_element_copy_func_tapi*>(loc - sizeof(void*));
}
vector_element_destructor_func_tapi find_destructor(void* data) {
  uintptr_t    loc = reinterpret_cast<uintptr_t>(getVectorTapi(data));
  vector_tapi* hnd = getVectorTapi(data);
  if (hnd->flag & vector_flagbit_constructor_tapi) {
    loc -= sizeof(void*);
  }
  if (hnd->flag & vector_flagbit_copy_tapi) {
    loc -= sizeof(void*);
  }
  return *reinterpret_cast<vector_element_destructor_func_tapi*>(loc - sizeof(void*));
}

void destroyElements(void* data, uint64_t destroyCount) {
  uintptr_t    loc = reinterpret_cast<uintptr_t>(data) - sizeof(vector_tapi);
  vector_tapi* hnd = getVectorTapi(data);
  // If there is a destructor, use it for old element
  if (hnd->flag & vector_flagbit_destructor_tapi) {
    vector_element_destructor_func_tapi destructor = find_destructor(data);
    for (uint64_t i = hnd->count - destroyCount; i < hnd->count; i++) {
      destructor(( unsigned char* )data + (hnd->elementSize * i));
    }
  } else { // Or set zero to memory then decommit it
    memset(( unsigned char* )data + (hnd->elementSize * (hnd->count - destroyCount)), 0,
           hnd->elementSize * destroyCount);
    // TODO: This decommit fails sometimes, fix it
    // virtual_decommit((unsigned char*)data + (hnd->elementSize * (hnd->count - destroyCount)),
    // hnd->elementSize * destroyCount);
  }
}
void addElements(void* data, uint64_t elementCount) {
  vector_tapi* hnd = getVectorTapi(data);
  uintptr_t    d   = reinterpret_cast<uintptr_t>(data);
  virtual_commit(reinterpret_cast<void*>(d + (hnd->elementSize * hnd->count)),
                 elementCount * hnd->elementSize);
  if (hnd->flag & vector_flagbit_constructor_tapi) {
    vector_element_constructor_func_tapi constructor = find_constructor(data);
    for (uint64_t i = hnd->count; i < elementCount + hnd->count; i++) {
      constructor(reinterpret_cast<void*>(d + (hnd->elementSize * i)));
    }
  }
  hnd->count += elementCount;
}
unsigned char resize(void* data, unsigned int newItemCount) {
  vector_tapi* hnd = getVectorTapi(data);
  int64_t      dif = int64_t(newItemCount) - int64_t(hnd->count);
  if (dif < 0) {
    destroyElements(data, -dif);
    return 1;
  } else if (dif > 0) {
    addElements(data, dif);
  }
}
void* create_vector(unsigned int elementSize, supermemoryblock_tapi* memblock,
                    unsigned int initialSize, unsigned int maxSize, vector_flags_tapi vectorFlag,
                    ...) {
  // Calculate size of vector struct and allocate it
  uint32_t sizeof_vectorFuncs = 0;
  if (vectorFlag & vector_flagbit_constructor_tapi) {
    sizeof_vectorFuncs += sizeof(void*);
  }
  if (vectorFlag & vector_flagbit_copy_tapi) {
    sizeof_vectorFuncs += sizeof(void*);
  }
  if (vectorFlag & vector_flagbit_destructor_tapi) {
    sizeof_vectorFuncs += sizeof(void*);
  }
  if (vectorFlag & vector_flagbit_invalidator_tapi) {
    sizeof_vectorFuncs += sizeof(void*);
  }
  void* alloc = ( void* )allocateFromSuperMemoryBlock(
    memblock, sizeof_vectorFuncs + sizeof(vector_tapi) + (elementSize * maxSize));
  virtual_commit(alloc, sizeof_vectorFuncs + sizeof(vector_tapi));
  vector_tapi* vector =
    reinterpret_cast<vector_tapi*>(reinterpret_cast<uintptr_t>(alloc) + sizeof_vectorFuncs);

  // Fill vector struct
  vector->capacity    = maxSize;
  vector->count       = 0;
  vector->elementSize = elementSize;
  vector->flag        = vectorFlag;
  va_list args;
  va_start(args, vectorFlag);
  void* lastFuncPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(vector) - sizeof(void*));
  if (vectorFlag & vector_flagbit_constructor_tapi) {
    vector_element_constructor_func_tapi constructor =
      va_arg(args, vector_element_constructor_func_tapi);
    memcpy(lastFuncPtr, &constructor, sizeof(vector_element_constructor_func_tapi));
    lastFuncPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(lastFuncPtr) - sizeof(void*));
  }
  if (vectorFlag & vector_flagbit_copy_tapi) {
    vector_element_copy_func_tapi copyFunc = va_arg(args, vector_element_copy_func_tapi);
    memcpy(lastFuncPtr, &copyFunc, sizeof(vector_element_copy_func_tapi));
    lastFuncPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(lastFuncPtr) - sizeof(void*));
  }
  if (vectorFlag & vector_flagbit_destructor_tapi) {
    vector_element_destructor_func_tapi destructor =
      va_arg(args, vector_element_destructor_func_tapi);
    memcpy(lastFuncPtr, &destructor, sizeof(vector_element_destructor_func_tapi));
    lastFuncPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(lastFuncPtr) - sizeof(void*));
  }
  if (vectorFlag & vector_flagbit_invalidator_tapi) {
    vector_element_copy_func_tapi invalidator = va_arg(args, vector_element_copy_func_tapi);
    memcpy(lastFuncPtr, &invalidator, sizeof(vector_element_copy_func_tapi));
    lastFuncPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(lastFuncPtr) - sizeof(void*));
  }
  va_end(args);

  if (!resize(vector + 1, initialSize)) {
    virtual_free(alloc, sizeof_vectorFuncs + sizeof(vector_tapi) + (elementSize * maxSize));
  }
  return vector + 1;
}
unsigned int get_vector_elementcount(const void* data) {
  vector_tapi* hnd = getVectorTapi(data);
  return hnd->count;
}
unsigned int get_vector_capacity(const void* data) {
  vector_tapi* hnd = getVectorTapi(data);
  return hnd->capacity;
}
unsigned char push_back(void* data, const void* src) {
  vector_tapi* hnd        = getVectorTapi(data);
  void*        dstElement = ( unsigned char* )data + (hnd->elementSize * hnd->count++);
  virtual_commit(dstElement, hnd->elementSize);
  if (src) {
    if (hnd->flag & vector_flagbit_copy_tapi) {
      find_copyfunc(hnd)(src, dstElement);
    } else {
      memcpy(dstElement, src, hnd->elementSize);
    }
  }
  return 1;
}
unsigned char erase(void* data, unsigned int elementIndex) {
  vector_tapi* hnd = getVectorTapi(data);
  if (elementIndex >= hnd->count) {
    return 0;
  }
  // If there is a copy func, use it for moving memory
  if (hnd->flag & vector_flagbit_copy_tapi) {
    vector_element_copy_func_tapi copyFunc = find_copyfunc(hnd);
    for (uint64_t i = elementIndex; i < hnd->count - 1; i++) {
      copyFunc(( unsigned char* )data + (hnd->elementSize * (i + 1)),
               ( unsigned char* )data + (hnd->elementSize * i));
    }
  } // Otherwise just copy memory
  else {
    memmove(( unsigned char* )data + (hnd->elementSize * elementIndex),
            ( unsigned char* )data + (hnd->elementSize * (elementIndex + 1)),
            hnd->elementSize * (hnd->count - elementIndex - 1));
  }
  if (((hnd->elementSize * hnd->count) % pagesize) == 0) {
    virtual_decommit(( unsigned char* )data + hnd->elementSize * hnd->count, pagesize);
  }
  return 1;
}

extern "C" unsigned int getSizeOfAllocatorSys() {
  return sizeof(allocator_sys_tapi) + sizeof(allocator_tapi_d);
}
allocator_sys_tapi* initialize(void* allocatorSys) {
  // Static variables
  pagesize               = get_pagesize();
  firstPageMemBlockCount = (pagesize - sizeof(supermemoryblock_tapi::superBlockInfo)) /
                           sizeof(supermemoryblock_tapi::memBlock);
  memBlockCountPerPage = pagesize / sizeof(supermemoryblock_tapi::memBlock);

  // System struct allocation
  sys                         = ( allocator_sys_tapi* )allocatorSys;
  sys->d                      = ( allocator_tapi_d* )(sys + 1);
  hidden                      = sys->d;
  *hidden                     = allocator_tapi_d();
  sys->standard               = ( buffer_allocator_tapi* )malloc(sizeof(buffer_allocator_tapi));
  sys->end_of_page            = ( buffer_allocator_tapi* )malloc(sizeof(buffer_allocator_tapi));
  sys->vector_manager         = ( vector_allocator_tapi* )malloc(sizeof(vector_allocator_tapi));
  sys->createSuperMemoryBlock = createSuperMemoryBlock;
  sys->freeSuperMemoryBlock   = freeSuperMemoryBlock;

  // Standard Allocator
  sys->standard->malloc = standard_malloc;
  sys->standard->free   = standard_free;

  // End of Page Buffer Allocator
  sys->end_of_page->malloc = &endofpage_malloc;
  sys->end_of_page->free   = &endofpage_free;

  // Vector Allocator
  sys->vector_manager->create_vector = &create_vector;
  sys->vector_manager->erase         = &erase;
  sys->vector_manager->capacity      = get_vector_capacity;
  sys->vector_manager->size          = get_vector_elementcount;
  sys->vector_manager->push_back     = push_back;
  sys->vector_manager->resize        = resize;

  return sys;
}
extern "C" void* initializeAllocator(void* allocatorSys) { return initialize(allocatorSys); }