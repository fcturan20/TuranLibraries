#include "Allocator.h"

#include <Windows.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <cstring>

#include "VirtualMemory.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCAllocator)
TCORE_PLUGIN_INIT(TCVirtualMemory)
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCAllocator)
TCORE_PLUGIN_ENTRY_POINT_END()

static constexpr uint64_t maxVectorAllocation_IfNotSpecified = uint64_t(1) << uint64_t(34);
static constexpr uint64_t maxSuperMemoryBlockName            = 62; // You should use null terminator
static constexpr uint64_t maxVirmemPerSuperBlock =
  1 << 24; // Reserved address space size for each super block's internal usage
static constexpr uint64_t maxSuperBlockCount         = 1 << 10;
static constexpr uint64_t maxVirmemForAllSuperBlocks = maxVirmemPerSuperBlock * maxSuperBlockCount;

// These can be gathered after reload too
static size_t GPageSize = 0;

// Size of this structure should be approximately equal to maxVirmemPerSuperBlock
struct TCSuperBlock {
  struct SuperBlockInfo {
    char               name[maxSuperMemoryBlockName];
    bool               isFreed = true;
    unsigned long long blockSize;
    unsigned int       activeBlockCount = 0;
    void*              ptr              = nullptr;
  };
  SuperBlockInfo info;
  struct MemBlock {
    bool         isFree = true;
    unsigned int size   = 0;
  };
  static constexpr uint32_t maxBlockCountPerSuperBlock =
    (maxVirmemPerSuperBlock - sizeof(SuperBlockInfo)) / sizeof(MemBlock);
  MemBlock blocks[maxBlockCountPerSuperBlock];
};

static uint32_t firstPageMemBlockCount;
static uint32_t memBlockCountPerPage;

struct tlAllocatorPriv {
  tlAllocator*  sys;
  uint32_t      createdSuperBlockCount;
  TCSuperBlock* superBlocks;
  tlAllocatorPriv() : createdSuperBlockCount(0), superBlocks(nullptr) {
    superBlocks = ( TCSuperBlock* )tlReserve(maxVirmemForAllSuperBlocks);
  }
};
static tlAllocatorPriv* priv = nullptr;

TCSuperBlock* allocateSuperMemoryBlock(unsigned long long blockSize,
                                       const char*        superMemBlockName) {
  void* alloc = tlReserve(blockSize);
  if (!alloc) {
    printf("Super Memory Block creation failed because virtual memory reserve failed\n");
    return nullptr;
  }

  // Search for a freed super memory block
  uint32_t namelen = std::min(wcslen(superMemBlockName), maxSuperMemoryBlockName);
  for (uint32_t i = 0; i < priv->createdSuperBlockCount; i++) {
    TCSuperBlock* super = &priv->superBlocks[i];
    if (!super->info.isFreed) {
      continue;
    }

    super->info.blockSize = blockSize;
    super->info.isFreed   = false;
    super->info.ptr       = alloc;
    memcpy(super->info.name, superMemBlockName, namelen * sizeof(char));
    return super;
  }

  // There is no freed super memory block, so allocate new one
  TCSuperBlock* super = &priv->superBlocks[priv->createdSuperBlockCount++];
  tlCommit(super, GPageSize);
  super->info.blockSize = blockSize;
  super->info.isFreed   = false;
  super->info.ptr       = alloc;
  memcpy(super->info.name, superMemBlockName, namelen * sizeof(char));

  for (unsigned int i = 0; i < firstPageMemBlockCount; i++) {
    super->blocks[i] = TCSuperBlock::memBlock();
  }
  super->info.activeBlockCount = firstPageMemBlockCount;
  // Committed memory will be 0 initialized, so memBlocks will be 0 initialized too
  return super;
}
void freeSuperMemoryBlock(TCSuperBlock* superMemBlock) {
  if (uintptr_t(superMemBlock) - uintptr_t(priv->superBlocks) % sizeof(TCSuperBlock)) {
    printf("freeSuperMemoryBlock failed because super memory block pointer is invalid!");
  }
  // Free all memory that superMemBlock externally has
  tlFree(superMemBlock->info.ptr, superMemBlock->info.blockSize);
  // Decommit superMemBlock's internal memory block infos except first page
  tlDecommit(( void* )(uintptr_t(superMemBlock) + GPageSize), sizeof(TCSuperBlock) - GPageSize);
  // Default initialize first page's memory block infos
  for (unsigned int i = 0; i < firstPageMemBlockCount; i++) {
    superMemBlock->blocks[i] = TCSuperBlock::memBlock();
  }
  // Info: Set as freed, memset name as 0
  superMemBlock->info.activeBlockCount = firstPageMemBlockCount;
  superMemBlock->info.blockSize        = 0;
  superMemBlock->info.isFreed          = true;
  memset(superMemBlock->info.name, 0, maxSuperMemoryBlockName);
  superMemBlock->info.ptr = nullptr;
}
void* allocateFromSuperMemoryBlock(TCSuperBlock* superMemBlock, unsigned int blockSize) {
  if ((uintptr_t(superMemBlock) - uintptr_t(priv->superBlocks)) % sizeof(TCSuperBlock)) {
    printf("allocateFromSuperMemoryBlock failed because super memory block pointer is invalid!\n");
  }
  uintptr_t lastVirmemAddress = reinterpret_cast<uintptr_t>(superMemBlock->info.ptr);
  uint32_t  memBlockIndex     = UINT32_MAX;
  for (uint32_t i = 0; i < superMemBlock->info.activeBlockCount; i++) {
    TCSuperBlock::memBlock& curBlock = superMemBlock->blocks[i];
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
      superMemBlock->info.blockSize - GPageSize) {
    printf("Your request can't fit in the Super Memory Block!\n");
    return nullptr;
  }
  // If all active memory blocks does allocate memory, create new block info with commiting a page
  if (memBlockIndex == UINT32_MAX) {
    memBlockIndex = superMemBlock->info.activeBlockCount++;
    tlCommit(superMemBlock->blocks + memBlockIndex, GPageSize);
    for (uint32_t i = 1; i < memBlockCountPerPage; i++) {
      superMemBlock->blocks[i + memBlockIndex] = TCSuperBlock::memBlock();
    }
    superMemBlock->info.activeBlockCount += memBlockCountPerPage;
  }
  TCSuperBlock::memBlock& finalBlock = superMemBlock->blocks[memBlockIndex];
  finalBlock.isFree                  = false;
  finalBlock.size                    = blockSize;
  return reinterpret_cast<void*>(lastVirmemAddress);
}
void freeFromSuperMemoryBlock(TCSuperBlock* superMemBlock, void* allocation) {
  if (uintptr_t(superMemBlock) - uintptr_t(priv->superBlocks) % sizeof(TCSuperBlock)) {
    printf("freeSuperMemoryBlock failed because super memory block pointer is invalid!\n");
  }
  uintptr_t lastVirmemAddress = reinterpret_cast<uintptr_t>(superMemBlock->info.ptr);
  for (uint32_t i = 0; i < superMemBlock->info.activeBlockCount; i++) {
    TCSuperBlock::memBlock& curBlock = superMemBlock->blocks[i];
    if (lastVirmemAddress == reinterpret_cast<uintptr_t>(allocation)) {
      curBlock.isFree = true;
      tlDecommit(allocation, curBlock.size);
      return;
    }
    if (lastVirmemAddress >= reinterpret_cast<uintptr_t>(allocation) || !curBlock.size) {
      printf("Freeing from a Super Memory Block failed because of invalid allocation ptr!\n");
      return;
    }
    lastVirmemAddress += curBlock.size;
  }
}

struct EndOfPageAllocatorServices {
  // Based on The Machinery's end of page allocator
  static void* Malloc(TCSuperBlock* memBlock, unsigned int size) {
    const unsigned long long requestedAllocSize = size + sizeof(unsigned int);
    // Requested size is rounded up to GPageSize (these pages will be committed)
    //  Then adds a new page (this page won't be committed)
    const unsigned long end_of_page_size =
      requestedAllocSize + (GPageSize - (requestedAllocSize % GPageSize)) + GPageSize;

    void* base = allocateFromSuperMemoryBlock(memBlock, end_of_page_size);
    tlCommit(base, end_of_page_size - GPageSize);
    // Memory Layout: offsettedSpace (less than a page) - sizeVariable (unsigned int) - data
    // (requestedSize) - reserved (non-committed) last page (page size)
    void* offsetted = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base) +
                                              (GPageSize - (requestedAllocSize % GPageSize)));
    // Store requested size right before actual data
    *((( unsigned int* )offsetted) - 1) = size + sizeof(unsigned int);
    return offsetted;
  }
  static void Free(TCSuperBlock* memBlock, void* returnedAllocPTR) {
    const uint32_t requestedAllocSize = *((( unsigned int* )returnedAllocPTR) - 1);
    if (!requestedAllocSize || requestedAllocSize == UINT32_MAX) {
      printf("End of Page allocator failed to free the allocation!");
      return;
    }
    const unsigned long end_of_page_size =
      requestedAllocSize + (GPageSize - (requestedAllocSize % GPageSize)) + GPageSize;
    void* base = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(returnedAllocPTR) -
                                         (GPageSize - (requestedAllocSize % GPageSize)));
    freeFromSuperMemoryBlock(memBlock, base);
  }
};

// Standard alloc
struct StandardAllocatorServices {
  static void* stdMalloc(TCSuperBlock* memBlock, unsigned int size) {
    const unsigned long long requestedAllocSize = size + sizeof(unsigned int);

    void* base = allocateFromSuperMemoryBlock(memBlock, requestedAllocSize);
    tlCommit(base, requestedAllocSize);
    return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base) + sizeof(unsigned int));
  }
  static void stdFree(TCSuperBlock* memBlock, void* returnedAllocPTR) {
    const uint32_t requestedAllocSize = *((( unsigned int* )returnedAllocPTR) - 1);
    if (!requestedAllocSize || requestedAllocSize == UINT32_MAX) {
      printf("Standard allocator failed to free the allocation!");
      return;
    }
    void* base =
      reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(returnedAllocPTR) - sizeof(unsigned int));
    freeFromSuperMemoryBlock(memBlock, base);
  }
};

struct TCVector {
  unsigned char flag;
  unsigned int  count = 0, capacity = 0, elementSize = 0;
  // Functions (constructor etc) according to flag will be right after the TCVector
  // Data allocated right after the functions
};
TCVector* getVectorTapi(const void* data) {
  return reinterpret_cast<TCVector*>(reinterpret_cast<uintptr_t>(data) - sizeof(TCVector));
}
tlVectorElementConstructorFnc find_constructor(void* data) {
  uintptr_t loc = reinterpret_cast<uintptr_t>(getVectorTapi(data));
  return *reinterpret_cast<tlVectorElementConstructorFnc*>(loc - sizeof(void*));
}
tlVectorElementCopyFnc find_copyfunc(void* data) {
  uintptr_t loc = reinterpret_cast<uintptr_t>(getVectorTapi(data));
  TCVector* hnd = getVectorTapi(data);
  if (hnd->flag & tlVectorFlagBit_constructor) {
    loc -= sizeof(void*);
  }
  return *reinterpret_cast<tlVectorElementCopyFnc*>(loc - sizeof(void*));
}
tlVectorElementDestructorFnc find_destructor(void* data) {
  uintptr_t loc = reinterpret_cast<uintptr_t>(getVectorTapi(data));
  TCVector* hnd = getVectorTapi(data);
  if (hnd->flag & tlVectorFlagBit_constructor) {
    loc -= sizeof(void*);
  }
  if (hnd->flag & tlVectorFlagBit_copy) {
    loc -= sizeof(void*);
  }
  return *reinterpret_cast<tlVectorElementDestructorFnc*>(loc - sizeof(void*));
}

void destroyElements(void* data, uint64_t destroyCount) {
  uintptr_t loc = reinterpret_cast<uintptr_t>(data) - sizeof(TCVector);
  TCVector* hnd = getVectorTapi(data);
  // If there is a destructor, use it for old element
  if (hnd->flag & tlVectorFlagBit_destructor) {
    tlVectorElementDestructorFnc destructor = find_destructor(data);
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
  TCVector* hnd = getVectorTapi(data);
  uintptr_t d   = reinterpret_cast<uintptr_t>(data);
  tlCommit(reinterpret_cast<void*>(d + (hnd->elementSize * hnd->count)),
           elementCount * hnd->elementSize);
  if (hnd->flag & tlVectorFlagBit_constructor) {
    tlVectorElementConstructorFnc constructor = find_constructor(data);
    for (uint64_t i = hnd->count; i < elementCount + hnd->count; i++) {
      constructor(reinterpret_cast<void*>(d + (hnd->elementSize * i)));
    }
  }
  hnd->count += elementCount;
}
unsigned char tlVectorResizeFnc(void* data, unsigned int newItemCount) {
  TCVector* hnd = getVectorTapi(data);
  int64_t   dif = int64_t(newItemCount) - int64_t(hnd->count);
  if (dif < 0) {
    destroyElements(data, -dif);
    return 1;
  } else if (dif > 0) {
    addElements(data, dif);
  }
}
void* tlVectorCreateFnc(TCSuperBlock* memblock, unsigned int elementSize, unsigned int initialSize,
                        unsigned int maxSize, tlVectorFlag vectorFlag, ...) {
  // Calculate size of vector struct and allocate it
  uint32_t sizeof_vectorFuncs = 0;
  if (vectorFlag & tlVectorFlagBit_constructor) {
    sizeof_vectorFuncs += sizeof(void*);
  }
  if (vectorFlag & tlVectorFlagBit_copy) {
    sizeof_vectorFuncs += sizeof(void*);
  }
  if (vectorFlag & tlVectorFlagBit_destructor) {
    sizeof_vectorFuncs += sizeof(void*);
  }
  if (vectorFlag & tlVectorFlagBit_invalidator) {
    sizeof_vectorFuncs += sizeof(void*);
  }
  void* alloc = ( void* )allocateFromSuperMemoryBlock(
    memblock, sizeof_vectorFuncs + sizeof(TCVector) + (elementSize * maxSize));
  tlCommit(alloc, sizeof_vectorFuncs + sizeof(TCVector));
  TCVector* vector =
    reinterpret_cast<TCVector*>(reinterpret_cast<uintptr_t>(alloc) + sizeof_vectorFuncs);

  // Fill vector struct
  vector->capacity    = maxSize;
  vector->count       = 0;
  vector->elementSize = elementSize;
  vector->flag        = vectorFlag;
  va_list args;
  va_start(args, vectorFlag);
  void* lastFuncPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(vector) - sizeof(void*));
  if (vectorFlag & tlVectorFlagBit_constructor) {
    tlVectorElementConstructorFnc constructor = va_arg(args, tlVectorElementConstructorFnc);
    memcpy(lastFuncPtr, &constructor, sizeof(tlVectorElementConstructorFnc));
    lastFuncPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(lastFuncPtr) - sizeof(void*));
  }
  if (vectorFlag & tlVectorFlagBit_copy) {
    tlVectorElementCopyFnc copyFunc = va_arg(args, tlVectorElementCopyFnc);
    memcpy(lastFuncPtr, &copyFunc, sizeof(tlVectorElementCopyFnc));
    lastFuncPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(lastFuncPtr) - sizeof(void*));
  }
  if (vectorFlag & tlVectorFlagBit_destructor) {
    tlVectorElementDestructorFnc destructor = va_arg(args, tlVectorElementDestructorFnc);
    memcpy(lastFuncPtr, &destructor, sizeof(tlVectorElementDestructorFnc));
    lastFuncPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(lastFuncPtr) - sizeof(void*));
  }
  if (vectorFlag & tlVectorFlagBit_invalidator) {
    tlVectorElementCopyFnc invalidator = va_arg(args, tlVectorElementCopyFnc);
    memcpy(lastFuncPtr, &invalidator, sizeof(tlVectorElementCopyFnc));
    lastFuncPtr = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(lastFuncPtr) - sizeof(void*));
  }
  va_end(args);

  if (!tlVectorResizeFnc(vector + 1, initialSize)) {
    tlFree(alloc, sizeof_vectorFuncs + sizeof(TCVector) + (elementSize * maxSize));
  }
  return vector + 1;
}
unsigned int tlVectorSizeFnc(const void* data) {
  TCVector* hnd = getVectorTapi(data);
  return hnd->count;
}
unsigned int tlVectorCapacityFnc(const void* data) {
  TCVector* hnd = getVectorTapi(data);
  return hnd->capacity;
}
unsigned char tlVectorPushBackFnc(void* data, const void* src) {
  TCVector* hnd        = getVectorTapi(data);
  void*     dstElement = ( unsigned char* )data + (hnd->elementSize * hnd->count++);
  tlCommit(dstElement, hnd->elementSize);
  if (src) {
    if (hnd->flag & tlVectorFlagBit_copy) {
      find_copyfunc(hnd)(src, dstElement);
    } else {
      memcpy(dstElement, src, hnd->elementSize);
    }
  }
  return 1;
}
unsigned char tlVectorEraseFnc(void* data, unsigned int elementIndex) {
  TCVector* hnd = getVectorTapi(data);
  if (elementIndex >= hnd->count) {
    return 0;
  }
  // If there is a copy func, use it for moving memory
  if (hnd->flag & tlVectorFlagBit_copy) {
    tlVectorElementCopyFnc copyFunc = find_copyfunc(hnd);
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
  if (((hnd->elementSize * hnd->count) % GPageSize) == 0) {
    tlDecommit(( unsigned char* )data + hnd->elementSize * hnd->count, GPageSize);
  }
  return 1;
}

tlAllocator* initialize() {
  // Static variables
  GPageSize = tlGetPageSize();
  firstPageMemBlockCount =
    (GPageSize - sizeof(TCSuperBlock::superBlockInfo)) / sizeof(TCSuperBlock::memBlock);
  memBlockCountPerPage = GPageSize / sizeof(TCSuperBlock::memBlock);

  // System struct allocation
  priv                          = new tlAllocatorPriv;
  priv->sys                     = new tlAllocator;
  tlAllocator* sys              = priv->sys;
  sys->d                        = priv;
  sys->allocateSuperMemoryBlock = allocateSuperMemoryBlock;
  sys->freeSuperMemoryBlock     = freeSuperMemoryBlock;

  return sys;
}
extern "C" void* initializeAllocator() { return initialize(); }

TCResult TCAllocator_Initialize(const void** outPluginAPI) {
  TCAllocatorServices* services      = new TCAllocatorServices;
  services->AllocateSuperMemoryBlock = allocateSuperMemoryBlock;
  services->FreeSuperMemoryBlock     = freeSuperMemoryBlock;
  services->VectorManager            = f_vector;
  services->StandardAllocator        = standard_alloc;
  services->EndOfPageAllocator       = standard_alloc; // TODO: Implement end of page allocator

  // Virtual memory system
  {
    tlVM* vm                = ( tlVM* )malloc(sizeof(tlVM));
    vm->pageSize            = &tlGetPageSize;
    vm->commit              = &tlCommit;
    vm->free                = &tlFree;
    vm->reserve             = &tlReserve;
    vm->decommit            = &tlDecommit;
    services->virtualMemory = vm;
  }

  // Standard Allocator
  {
    TCBufferServices* Allocator = ( TCBufferServices* )malloc(sizeof(TCBufferServices));
    Allocator->Malloc           = stdMalloc;
    Allocator->Free             = stdFree;
    services->StandardAllocator = Allocator;
  }

  // End of Page Buffer Allocator
  {
    TCBufferServices* bufAllocator = ( TCBufferServices* )malloc(sizeof(TCBufferServices));
    bufAllocator->Malloc           = &eopMalloc;
    bufAllocator->Free             = &eopFree;
    services->EndOfPageAllocator   = bufAllocator;
  }

  // Vector Allocator
  {
    TCVectorServices* f_vector = ( TCVectorServices* )malloc(sizeof(TCVectorServices));
    f_vector->Create           = &tlVectorCreateFnc;
    f_vector->Erase            = &tlVectorEraseFnc;
    f_vector->Capacity         = &tlVectorCapacityFnc;
    f_vector->Size             = &tlVectorSizeFnc;
    f_vector->PushBack         = &tlVectorPushBackFnc;
    f_vector->Resize           = &tlVectorResizeFnc;
    services->VectorManager    = f_vector;
  }

  TCAllocator   = services;
  *outPluginAPI = TCAllocator;

  return TC_RESULT_SUCCESS;
}

TCResult TCAllocator_OnPreShutdown() { return TC_RESULT_SUCCESS; }

TCResult TCAllocator_Shutdown() { return TC_RESULT_SUCCESS; }

void TCAllocator_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, bool isLoaded) {}
