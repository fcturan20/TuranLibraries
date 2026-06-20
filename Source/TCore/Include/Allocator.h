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
	void* (*Malloc)(TCSuperBlockHandle memBlock, TSize size, const char* name);
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
	TCVectorHandle (*Create)(TCSuperBlockHandle memory_block, TUint element_size, TSize initial_size, TSize max_size);
	TSize (*Size)(const TCVectorHandle hnd);
	TSize (*Capacity)(const TCVectorHandle hnd);
	// @param src: Source data to copy from
	// @param copyFunc: Used to copy the element
	TCResult (*PushBack)(TCVectorHandle hnd, const void* src);
	// @return 0 if there is no such object, 1 if succeeds
	TCResult (*Erase)(TCVectorHandle hnd, TSize element_index);
	// @param constructor: if new items will be added, this is used to initialize
	// @param destructor: if old items will be destroyed, this is used to destroy
	TCResult (*Resize)(TCVectorHandle hnd, TSize new_item_count);
	void (*Destroy)(TCVectorHandle hnd);
} ITCVector;

typedef struct ITCAllocator
{
	TCSuperBlockHandle (*AllocateSuperMemoryBlock)(TSize block_size, const char* super_mem_block_name);
	void (*FreeSuperMemoryBlock)(TCSuperBlockHandle super_mem_block);
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

// C++ wrapper
#define TCORE_USE_CPP_WRAPPER
#if defined(TCORE_CPP_20) & defined(TCORE_USE_CPP_WRAPPER)
namespace TCore
{

extern TCSuperBlockHandle GSuperMemoryBlock;

// Write a C++ wrapper for the vector allocator
template <typename T>
class Vector
{
public:
	Vector(TCSuperBlockHandle memblock = GSuperMemoryBlock,
		   unsigned int initial_size = 0,
		   unsigned int initial_capacity = 0)
	{
		Handle = TCVectorManager->Create(memblock, sizeof(T), initial_size, initial_capacity);
		for (size_t i = 0; i < initial_size; i++)
			new ((*this)[i]) T();
	}

	Vector(const Vector& other)
	{
		Handle = TCVectorManager->Create(GSuperMemoryBlock, sizeof(T), other.Size(), other.Capacity());
		for (TSize i = 0; i < other.Size(); i++)
			new ((*this)[i]) T(other[i]);
	}

	// STD::vector like functions
	void PushBack(const T& item) { TCVectorManager->PushBack(Handle, &item); }
	TSize Size() const { return TCVectorManager->Size(Handle); }
	TSize Capacity() const { return TCVectorManager->Capacity(Handle); }
	T* Data() { return reinterpret_cast<T*>(Handle); }
	T& operator[](TSize index) { return reinterpret_cast<T*>(Handle)[index]; }
	const T& operator[](TSize index) const { return reinterpret_cast<const T*>(Handle)[index]; }
	void Resize(TSize new_size) { TCVectorManager->Resize(Handle, new_size); }
	void Erase(TSize index) { TCVectorManager->Erase(Handle, index); }
};

} // namespace TCore
#endif