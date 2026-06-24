#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE
TCORE_PLUGIN_DEFINE(TCAllocator, "tcAllocator", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

//        ALLOCATOR API
/////////////////////////////////////
//  This API is used to provide memory allocator functionality as simple as possible
//  SuperMemoryBlock: A very big address space possibly for a system
//   Allocators allocates for a system by using a SuperMemoryBlock pointer
TCORE_DEFINE_HANDLE(TCSuperBlock);

typedef struct ITCBuffer
{
	TCSuperBlockHandle (*CreateSuperBlock)(TSize block_size, const char* name);
	void (*DestroySuperBlock)(TCSuperBlockHandle super_mem_block);

	void* (*Malloc)(TCSuperBlockHandle memory_block, TSize size, const char* name);
	// @param allocationPtr: This should be the same pointer as returned by malloc()
	void (*Free)(void* allocationPtr);
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
	TSize (*Size)(const TCVectorHandle vector);
	TSize (*Capacity)(const TCVectorHandle vector);
	// @param src: Source data to copy from
	// @param copyFunc: Used to copy the element
	TCResult (*PushBack)(TCVectorHandle vector, const void* src);
	// @return 0 if there is no such object, 1 if succeeds
	TCResult (*Erase)(TCVectorHandle vector, TSize element_index);
	// @param constructor: if new items will be added, this is used to initialize
	// @param destructor: if old items will be destroyed, this is used to destroy
	TCResult (*Resize)(TCVectorHandle vector, TSize new_item_count);
	void (*Destroy)(TCVectorHandle vector);
} ITCVector;

typedef struct ITCAllocator
{
	const ITCVector* VectorManager;
	// Allocates each memory block directly after each other
	const ITCBuffer* StandardAllocator;
	// Allocates extra empty pages at start and end of the allocated memory block to prevent out of space accesses
	// Use for debugging (out of space accesses will be crashes etc.)
	// Allocation layout: Page0 [ Block A Info | Empty Space ], Page1 Empty, Page2 [ Data A ], Page3 Empty,
	//						Page4 [ Block B Info | Empty Space ], ...
	const ITCBuffer* EndOfPageAllocator;
} ITCAllocator;

#define TCVectorManager TCAllocator->VectorManager
#define TCStdAllocator TCAllocator->StandardAllocator
#define TCEopAllocator TCAllocator->EndOfPageAllocator

TCORE_END_C_LINKAGE

// C++ wrapper
#define TCORE_USE_CPP_WRAPPER
#if defined(TCORE_CPP_20) & defined(TCORE_USE_CPP_WRAPPER)
#include <cstring>

namespace TCore
{

extern TCSuperBlockHandle GSuperMemoryBlock;
#define TCORE_PLUGIN_RESERVE_ADDRESS_SPACE(size) TCAllocator->AllocateSuperMemoryBlock(size, TCORE_ACTIVE_PLUGIN_NAME)

// Write a C++ wrapper for the vector allocator
template <typename T, bool EnableTrivialCopy>
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
		if constexpr (!EnableTrivialCopy)
			for (TSize i = 0; i < other.Size(); i++)
				new ((*this)[i]) T(other[i]);
		else
			std::memcpy(Data(), other.Data(), sizeof(T) * other.Size());
	}

	// STD::vector like functions
	void PushBack(const T& item)
	{
		if (TCVectorManager->PushBack(Handle, &item) != TC_RESULT_SUCCESS)
		{
			printf("Vector push back failed!\n");
			return;
		}

		if constexpr (!EnableTrivialCopy)
			new ((*this)[Size() - 1]) T(item);
		else
			std::memcpy((*this)[Size() - 1], &item, sizeof(T));
	}
	TSize Size() const { return TCVectorManager->Size(Handle); }
	TSize Capacity() const { return TCVectorManager->Capacity(Handle); }
	T* Data() { return reinterpret_cast<T*>(Handle); }
	T& operator[](TSize index) { return reinterpret_cast<T*>(Handle)[index]; }
	const T& operator[](TSize index) const { return reinterpret_cast<const T*>(Handle)[index]; }
	void Resize(TSize new_size)
	{
		auto capacity = TCVectorManager->Capacity(Handle);
		auto size = TCVectorManager->Size(Handle);

		if (new_size > capacity)
		{
			auto newHnd = TCVectorManager->Create(GSuperMemoryBlock, sizeof(T), new_size);

			if constexpr (!EnableTrivialCopy)
				for (TSize i = 0; i < size; i++)
					new ((*newHnd)[i]) T((*this)[i]);
			else
				std::memcpy(newHnd, Handle, sizeof(T) * size);

			TCVectorManager->Destroy(Handle);
			Handle = newHnd;
		}
		else if (capacity > new_size && new_size > size)
		{
			auto diff = new_size - size;
			for (TSize i = 0; i < diff; i++)
				PushBack(T());
		}
		else
		{
			if constexpr (!EnableTrivialCopy)
				for (TSize i = new_size; i < size; i++)
					(*this)[i].~T();
			else
				std::memset((*this) + new_size, 0, sizeof(T) * (size - new_size));
		}

		TCVectorManager->Resize(Handle, new_size);
	}
	void Erase(TSize index) { TCVectorManager->Erase(Handle, index); }
	T* Begin() { return Data(); }
	T* End() { return Data() + Size() - 1; }

private:
	TCVectorHandle Handle;
};

} // namespace TCore
#endif