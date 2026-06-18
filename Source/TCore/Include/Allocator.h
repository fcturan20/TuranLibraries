#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCAllocator, "tcAllocator", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

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
TCORE_DEFINE_HANDLE(TCSuperBlock);

typedef struct ITCBuffer
{
	void* (*Malloc)(TCSuperBlockHandle memBlock, unsigned int size, const char* name);
	// @param returnedAllocPTR: This should be the same pointer as returned by malloc()
	void (*Free)(void* returnedAllocPTR);
} ITCBuffer;

TCORE_DEFINE_HANDLE(TCVector);
typedef struct ITCVector
{
	// Use maxSize as much as you can because vector struct and data will be right back each other.
	// So overall faster access etc.
	// @param elementSize: Size of a single element
	// @param memblock: Super memory block to allocate address from
	// @param initialSize: Elements to push at initialization
	// @param maxSize: Define an upper limit for element count
	// @return Array pointer, so cast it to your own type
	TCVectorHandle (*Create)(TCSuperBlockHandle memblock,
							 unsigned int elementSize,
							 unsigned int initialSize,
							 unsigned int maxSize);
	unsigned int (*Size)(const TCVectorHandle hnd);
	unsigned int (*Capacity)(const TCVectorHandle hnd);
	// @param src: Source data to copy from
	// @param copyFunc: Used to copy the element
	// @return 0 if fails; 1 if succeeds. May fail if mem commit fails or upper limit is reached.
	unsigned char (*PushBack)(TCVectorHandle hnd, const void* src);
	// @return 0 if there is no such object, 1 if succeeds
	unsigned char (*Erase)(TCVectorHandle hnd, unsigned int elementIndex);
	// @param constructor: if new items will be added, this is used to initialize
	// @param destructor: if old items will be destroyed, this is used to destroy
	// @return 0 if fails; 1 if succeeds. May fail if mem allocation fais or upper limit is reached.
	unsigned char (*Resize)(TCVectorHandle hnd, unsigned int newItemCount);
	void (*Destroy)(TCVectorHandle hnd);
} ITCVector;

typedef struct ITCAllocator
{
	TCSuperBlockHandle (*AllocateSuperMemoryBlock)(unsigned long long blockSize, const char* superMemBlockName);
	void (*FreeSuperMemoryBlock)(TCSuperBlockHandle superMemBlock);
	const ITCVector* VectorManager;
	const ITCBuffer* StandardAllocator;
	// Allocations are rounded up to pagesize and an extra page is allocated
	// Use for debugging (out of space accesses will be crashes etc.)
	const ITCBuffer* EndOfPageAllocator;
} ITCAllocator;

#define TCVectorManager TCAllocator->VectorManager
#define TCStdAllocator TCAllocator->StandardAllocator
#define TCEopAllocator TCAllocator->EndOfPageAllocator

TCORE_END_C_LINKAGE