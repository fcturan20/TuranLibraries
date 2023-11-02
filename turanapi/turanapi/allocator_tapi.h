#pragma once
#include "predefinitions_tapi.h"
#define ALLOCATOR_TAPI_PLUGIN_NAME "tapi_allocator"
#define ALLOCATOR_TAPI_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0, 0, 0)
#define ALLOCATOR_TAPI_PLUGIN_LOAD_TYPE const struct tlAllocator*

#ifdef __cplusplus
extern "C" {
#endif
//        ALLOCATOR API
/////////////////////////////////////
//  This API is used to provide memory allocator functionality as simple as possible
//  There are 3 types of allocators:
//    1) Classic buffer allocators (allocator_tapi)
//    2) Vector allocators (std::vector with custom functionality)
//    3) List allocators (OrderedList, Hash etc.)
//  SuperMemoryBlock: A very big address space possibly for a system
//   Allocators allocates for a system by using a SuperMemoryBlock pointer
//   AllocatorSystem has its own address space for storing internal structures of SuperMemoryBlocks
struct tlSuperBlock;

struct tlVM {
  // Reserve address space from virtual memory
  // size is in bytes
  void* (*reserve)(unsigned long long size);
  // Initialize the reserved memory with zeros.
  void (*commit)(void* reservedmem, unsigned long long commitsize);
  // Return back the committed memory to reserved state
  // This will help if you want to catch some bugs that points to memory you just freed.
  void (*decommit)(void* committedmem, unsigned long long size);
  // Return allocated address space back to OS
  void (*free)(void* ptr, unsigned long long size);
  // Get page size of the system
  unsigned int (*pageSize)();
};

struct tlBuffer {
  void* (*malloc)(struct tlSuperBlock* memBlock, unsigned int size);
  // @param returnedAllocPTR: This should be the same pointer as returned by malloc()
  void (*free)(struct tlSuperBlock* memBlock, void* returnedAllocPTR);
  void (*setMallocName)(void* allocation, const wchar_t* name);
};

typedef void (*tlVectorElementConstructorFnc)(void* elementPtr);
typedef void (*tlVectorElementCopyFnc)(const void* srcElement, void* dstElement);
typedef void (*tlVectorElementDestructorFnc)(void* elementPtr);

//         VECTOR ALLOCATOR API
/////////////////////////////////////
// There is no static vector type info, so user should define their vectors by defining them in
// run-time Vectors allocate an adress space of "maxSize * elementSize" bytes from the allocator
// Then manages commitment of

enum tlVectorFlagBits {
  tlVectorFlagBit_constructor = 1,
  tlVectorFlagBit_copy        = 2,
  tlVectorFlagBit_destructor  = 4,
  tlVectorFlagBit_invalidator = 8
};
typedef int tlVectorFlag;
struct tlVector {
  // Use maxSize as much as you can because vector struct and data will be right back each other.
  // So overall faster access etc.
  // @param elementSize: Size of a single element
  // @param memblock: Super memory block to allocate address from
  // @param initialSize: Elements to push at initialization
  // @param maxSize: Define an upper limit for element count
  // @param vectorFlag: Defines how variadic arguments are interpreted
  // @return Array pointer, so cast it to your own type
  void* (*create)(struct tlSuperBlock* memblock, unsigned int elementSize, unsigned int initialSize,
                  unsigned int maxSize, tlVectorFlag vectorFlag, ...);
  unsigned int (*size)(const void* hnd);
  unsigned int (*capacity)(const void* hnd);
  // @param src: Source data to copy from
  // @param copyFunc: Used to copy the element
  // @return 0 if fails; 1 if succeeds. May fail if mem commit fails or upper limit is reached.
  unsigned char (*pushBack)(void* hnd, const void* src);
  // @return 0 if there is no such object, 1 if succeeds
  unsigned char (*erase)(void* hnd, unsigned int elementIndex);
  // @param constructor: if new items will be added, this is used to initialize
  // @param destructor: if old items will be destroyed, this is used to destroy
  // @return 0 if fails; 1 if succeeds. May fail if mem allocation fais or upper limit is reached.
  unsigned char (*resize)(void* hnd, unsigned int newItemCount);
};
// Vector allocator should be named f_vector
#define tlVectorCreate(type, memblock, maxSize) \
  (( type* )f_vector->create(memblock, sizeof(type), 0, maxSize, 0))
#define tlVectorSize(vectorHnd) f_vector->size(vectorHnd)
#define tlVectorPushBack(vectorHnd, src) f_vector->pushBack(vectorHnd, src)
#define tlVectorCapacity(vectorHnd) f_vector->capacity(vectorHnd)
#define tlVectorErase(vectorHnd, elementIndx) f_vector->erase(vectorHnd, elementIndx)
#define tlVectorResize(vectorHnd, newItemCount) f_vector->erase(vectorHnd, newItemCount)
typedef struct tlAllocator {
  const struct tlAllocatorPriv* d;
  struct tlSuperBlock* (*allocateSuperMemoryBlock)(unsigned long long blockSize,
                                                   const wchar_t*     superMemBlockName);
  void (*freeSuperMemoryBlock)(struct tlSuperBlock* superMemBlock);
  const struct tlVM*     virtualMemory;
  const struct tlVector* vectorManager;
  const struct tlBuffer* standard;
  // Allocations are rounded up to pagesize and an extra page is allocated
  // Use for debugging (out of space accesses will be crashes etc.)
  const struct tlBuffer* endOfPage;
};

#ifdef __cplusplus
}
#endif