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
	TCSuperBlockHnd (*CreateSuperBlock)(TSize block_size, const char* name);
	void (*DestroySuperBlock)(TCSuperBlockHnd super_mem_block);

	void* (*Malloc)(TCSuperBlockHnd memory_block, TSize size, const char* name);
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
	TCVectorHnd (*Create)(TCSuperBlockHnd memory_block, TUint element_size, TSize initial_size, TSize max_size);
	TSize (*Size)(const TCVectorHnd vector);
	TSize (*Capacity)(const TCVectorHnd vector);
	// @param src: Source data to copy from
	// @param copyFunc: Used to copy the element
	TCResult (*PushBack)(TCVectorHnd vector, const void* src);
	// @return 0 if there is no such object, 1 if succeeds
	TCResult (*Erase)(TCVectorHnd vector, TSize element_index);
	void (*Destroy)(TCVectorHnd vector);
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
#if defined(TCORE_CPP_20) & defined(TCORE_USE_CPP_WRAPPER)
#include <cstring>
#include <type_traits>

namespace TCore
{

// Use macros at the end of the header
extern TCSuperBlockHnd GSuperMemoryBlock;

// Write a C++ wrapper for the vector allocator
template <typename T, bool EnableTrivialCopy = std::is_trivially_copyable<T>::value>
class Vector
{
public:
	Vector(TCSuperBlockHnd mem_block = GSuperMemoryBlock,
		   unsigned int initial_size = 0,
		   unsigned int initial_capacity = 0)
	{
		Handle = TCVectorManager->Create(mem_block, sizeof(T), initial_size, initial_capacity);
		if constexpr (!EnableTrivialCopy)
			for (size_t i = 0; i < initial_size; i++)
				new ((*this)[i]) T();
	}

	Vector(const Vector& other)
	{
		Handle = TCVectorManager->Create(GSuperMemoryBlock, sizeof(T), other.Size(), other.Capacity());
		if constexpr (!EnableTrivialCopy)
			for (TSize i = 0; i < other.Size(); i++)
				new ((*this)[i]) T(other[i]);
	}

	// STD::vector like functions
	void PushBack(const T& item)
	{
		Resize(Size() + 1);
		if (TCVectorManager->PushBack(Handle, &item) != TC_RESULT_SUCCESS)
		{
			printf("Vector push back failed!\n");
			return;
		}

		if constexpr (!EnableTrivialCopy)
			new ((*this)[Size() - 1]) T(item);
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
			auto newHnd = TCVectorManager->Create(GSuperMemoryBlock, sizeof(T), new_size, new_size);

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
				std::memset((this) + new_size, 0, sizeof(T) * (new_size - size));
		}
	}
	void Erase(TSize index) { TCVectorManager->Erase(Handle, index); }
	T* Begin() { return Data(); }
	T* End() { return Data() + Size() - 1; }

private:
	TCVectorHnd Handle;
};

} // namespace TCore

#define TCORE_PLUGIN_MEMORY_BLOCK_INIT() TCSuperBlockHnd TCore::GSuperMemoryBlock = nullptr;
// Call this inside the entry point
#define TCORE_PLUGIN_RESERVE_ADDRESS_SPACE(size)                                                                       \
	TCore::GSuperMemoryBlock = TCStdAllocator->CreateSuperBlock(size, TCORE_ACTIVE_PLUGIN_NAME)

#endif