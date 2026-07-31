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
TCORE_DEFINE_HANDLE(TCVector);

typedef enum TCSuperBlockAllocationStrategy
{
	// Allocates each memory block directly after each other
	TCORE_SUPERBLOCKALLOCATIONSTRATEGY_STANDARD,
	// Allocates extra empty pages at start and end of the allocated memory block to prevent out of space accesses
	// Use for debugging (out of space accesses will be crashes etc.)
	// Allocation layout: Page0 [ Block A Info | Empty Space ], Page1 Empty, Page2 [ Data A ], Page3 Empty,
	//						Page4 [ Block B Info | Empty Space ], ...
	TCORE_SUPERBLOCKALLOCATIONSTRATEGY_EOP
} TCSuperBlockAllocationStrategy;

typedef struct ITCVector
{
	// Use maxSize as much as you can because vector struct and data will be right back each other.
	// So overall faster access etc.
	// @param elementSize: Size of a single element
	// @param memblock: Super memory block to allocate address from
	// @param initialSize: Elements to push at initialization
	// @param maxSize: Define an upper limit for element count
	// @return Array pointer, so cast it to your own type
	TCVector (*Create)(TCSuperBlock memory_block, TU4 element_size, TU8 initial_size, TU8 max_size);
	TU8 (*Size)(const TCVector vector);
	TU8 (*Capacity)(const TCVector vector);
	// @param src: Source data to copy from
	// @param copyFunc: Used to copy the element
	TCResult (*PushBack)(TCVector vector, const void* src);
	// @return 0 if there is no such object, 1 if succeeds
	TCResult (*Erase)(TCVector vector, TU8 element_index);
	TCSuperBlock (*GetSuperBlock)(const TCVector vector);
	void (*Destroy)(TCVector vector);
} ITCVector;

typedef struct ITCAllocator
{
	TCSuperBlock (*CreateSuperBlock)(TU8 block_size, const char* name, TCSuperBlockAllocationStrategy strategy);
	void (*DestroySuperBlock)(TCSuperBlock super_memory_block);
	void* (*Malloc)(TCSuperBlock memory_block, TU8 size, const char* name);
	// @param allocationPtr: This should be the same pointer as returned by malloc()
	void (*Free)(void* allocationPtr);

	const ITCVector* VectorManager;
} ITCAllocator;

#define TCVectorManager TCAllocator->VectorManager

TCORE_END_C_LINKAGE

// C++ wrapper
#if defined(TCORE_CPP_20) & defined(TCORE_USE_CPP_WRAPPER)
#include <cstring>
#include <type_traits>

namespace TCore
{

// Use macros at the end of the header
extern TCSuperBlock GSuperMemoryBlock;

template <typename T, bool EnableTrivialCopy = std::is_trivially_copyable<T>::value>
class Vector
{
public:
	Vector(TCSuperBlock mem_block = GSuperMemoryBlock, unsigned int initSize = 0, unsigned int initCapacity = 0)
	{
		Handle = TCVectorManager->Create(mem_block, sizeof(T), initSize, initCapacity);

		// Construct non-trivial elements
		if constexpr (!EnableTrivialCopy && std::is_default_constructible<T>::value)
		{
			T* data = reinterpret_cast<T*>(Handle);
			for (unsigned int i = 0; i < initSize; ++i)
				new (&data[i]) T();
		}
	}

	// Copy constructor
	Vector(const Vector& other)
	{
		TU8 otherSize = other.Size();
		Handle = TCVectorManager->Create(GSuperMemoryBlock, sizeof(T), otherSize, other.Capacity());

		if constexpr (!EnableTrivialCopy)
		{
			T* dst = reinterpret_cast<T*>(Handle);
			for (TU8 i = 0; i < otherSize; ++i)
				new (&dst[i]) T(other[i]);
		}
		else
		{
			std::memcpy(reinterpret_cast<void*>(Handle),
						reinterpret_cast<const void*>(other.Handle),
						sizeof(T) * static_cast<size_t>(otherSize));
		}
	}

	// Destructor — destroy elements and free the underlying vector handle.
	~Vector()
	{
		if (!Handle)
			return;

		TU8 size = Size();

		if constexpr (!EnableTrivialCopy)
		{
			T* data = reinterpret_cast<T*>(Handle);
			for (TU8 i = 0; i < size; ++i)
				data[i].~T();
		}

		TCVectorManager->Destroy(Handle);
		Handle = nullptr;
	}

	// STD::vector like functions
	void PushBack(const T& item)
	{
		TU8 idx = Size();
		Resize(idx + 1);

		// Construct last element for non-trivial types
		if constexpr (!EnableTrivialCopy)
			new (&reinterpret_cast<T*>(Handle)[idx]) T(item);
		else
			std::memcpy(reinterpret_cast<void*>(Data() + idx), reinterpret_cast<const void*>(&item), sizeof(T));
	}

	TU8 Size() const { return TCVectorManager->Size(Handle); }
	TU8 Capacity() const { return TCVectorManager->Capacity(Handle); }
	T* Data() { return reinterpret_cast<T*>(Handle); }
	const T* Data() const { return reinterpret_cast<const T*>(Handle); }
	T& operator[](TU8 index) { return reinterpret_cast<T*>(Handle)[index]; }
	const T& operator[](TU8 index) const { return reinterpret_cast<const T*>(Handle)[index]; }

	// Resize: if new_size > capacity, allocate a new underlying buffer and move/copy elements.
	// If new_size > size and new_size <= capacity, use PushBack() to increase size.
	// If new_size < size, call destructor for non-trivial elements then Erase() to reduce size.
	void Resize(TU8 new_size)
	{
		TU8 capacity = Capacity();
		TU8 size = Size();

		if (new_size > capacity)
		{
			TCVector newHnd =
				TCVectorManager->Create(TCVectorManager->GetSuperBlock(Handle), sizeof(T), new_size, new_size * 2);
			T* newData = reinterpret_cast<T*>(newHnd);
			T* oldData = reinterpret_cast<T*>(Handle);

			if (!EnableTrivialCopy)
			{
				for (TU8 i = 0; i < size; ++i)
					new (&newData[i]) T(oldData[i]);

				// Destroy old elements
				for (TU8 i = 0; i < size; ++i)
					oldData[i].~T();
			}
			else
				std::memcpy(reinterpret_cast<void*>(newData),
							reinterpret_cast<const void*>(oldData),
							sizeof(T) * static_cast<size_t>(size));

			TCVectorManager->Destroy(Handle);
			Handle = newHnd;

			// If expanding beyond previous size, default-construct / zero new elements
			if (new_size > size)
			{
				for (TU8 i = size; i < new_size; ++i)
				{
					if (!EnableTrivialCopy)
						new (&reinterpret_cast<T*>(Handle)[i]) T();
					else
						std::memset(reinterpret_cast<char*>(reinterpret_cast<T*>(Handle) + i), 0, sizeof(T));
				}
			}
		}
		else if (new_size > size && new_size <= capacity)
		{
			// Expand within existing capacity by pushing default-constructed elements.
			T diffVal{};
			TU8 diff = new_size - size;
			for (TU8 i = 0; i < diff; ++i)
			{
				TCVectorManager->PushBack(Handle, &diffVal);
				if constexpr (!EnableTrivialCopy)
					new (&reinterpret_cast<T*>(Handle)[i]) T();
			}
		}
		else if (new_size < size)
		{
			// Shrink: destroy and erase elements from end down to new_size
			for (TU8 i = size; i > new_size; --i)
			{
				TU8 idx = i - 1;
				if constexpr (!EnableTrivialCopy)
					reinterpret_cast<T*>(Handle)[idx].~T();
				TCVectorManager->Erase(Handle, idx);
			}
		}
		// if new_size == size: nothing to do
	}

	// inside class Vector<T,...> add these methods (place near PushBack)
	// rvalue overload to avoid an extra copy when user passes a temporary
	T& PushBack(T&& item)
	{
		TU8 idx = Size();
		Resize(idx + 1);

		// Construct last element for non-trivial types
		if constexpr (!EnableTrivialCopy)
			new (&reinterpret_cast<T*>(Handle)[idx]) T(item);
		else
			std::memcpy(reinterpret_cast<void*>(Data() + idx), reinterpret_cast<const void*>(&item), sizeof(T));
		return *(Data() + idx);
	}
	// emplace to construct the element using the target constructor
	template <typename... Args>
	T& EmplaceBack(Args&&... args)
	{
		T tmp(std::forward<Args>(args)...);
		return PushBack(std::move(tmp));
	}

	void Erase(TU8 index) { TCVectorManager->Erase(Handle, index); }
	T* Find(const T& obj)
	{
		for (TU8 i = 0; i < Size(); i++)
		{
			if ((Data()[i]) == obj)
				return &(*this)[i];
		}
		return End();
	}
	TU8 GetIndxByPointer(T* iter)
	{
		TCORE_SOFT_CHECK(iter >= Data() && iter < End() && ((reinterpret_cast<uintptr_t>(iter) % sizeof(T)) == 0),
						 "Invalid iterator");
		return (reinterpret_cast<uintptr_t>(iter) - reinterpret_cast<uintptr_t>(Data())) / sizeof(T);
	}
	T* Begin() { return Data(); }
	T* End() { return Data() + Size(); }

private:
	TCVector Handle = nullptr;
};

uint32_t ClosestPowerOf2(uint32_t x)
{
	if (x <= 1)
		return 1;

	unsigned long index;
	_BitScanReverse(&index, x);

	uint32_t lower = 1u << index;
	uint32_t upper = lower << 1;

	return (x - lower < upper - x) ? lower : upper;
}

template <typename K, typename V>
class PairedList
{
private:
	TCBuffer Data;
	TU8 ElementCount{};
	TCSuperBlock SuperBlock;
	static constexpr TU8 kElementSize = sizeof(K) + sizeof(V);
	TU8 GetBufferElementCapacity(const TCBuffer& buffer) const { return buffer.Size / kElementSize; }
	struct ListPointers
	{
	public:
		K* Keys;
		V* Values;
		TU8 Capacity;
	};
	ListPointers GetPointersFromBuffer(TCBuffer data) const
	{
		auto listSize = GetBufferElementCapacity(data);
		ListPointers r;
		r.Keys = (K*)data.Data;
		r.Values = (V*)(r.Keys + listSize);
		r.Capacity = listSize;
		return r;
	}
	void ExpandIfNecessary(TU8 newSize)
	{
		auto capacity = GetBufferElementCapacity(Data);
		if (capacity >= newSize)
			return;

		auto requiredCapacity = ClosestPowerOf2(newSize + 1);
		auto newAllocSize = kElementSize * requiredCapacity;
		auto recentVec = TCAllocator->Malloc(SuperBlock, newAllocSize, "PairList");
		TCBuffer recentBuf = {recentVec, newAllocSize};

		ListPointers prev = GetPointersFromBuffer(Data);
		ListPointers recent = GetPointersFromBuffer(recentBuf);
		for (TU8 i = 0; i < ElementCount; i++)
		{
			new (&recent.Keys[i]) K(prev.Keys[i]);
			prev.Keys[i].~K();
		}
		for (TU8 i = 0; i < ElementCount; i++)
		{
			new (&recent.Values[i]) V(prev.Values[i]);
			prev.Values[i].~V();
		}
		TCAllocator->Free(Data.Data);
		Data = recentBuf;
	}

public:
	PairedList(TCSuperBlock superBlock = GSuperMemoryBlock) : SuperBlock(superBlock)
	{
		Data = {TCAllocator->Malloc(superBlock, kElementSize, "PairList"), kElementSize};
	}
	V* Find(const K& k)
	{
		ListPointers cur = GetPointersFromBuffer(Data);
		for (TU8 i = 0; i < ElementCount; i++)
			if (cur.Keys[i] == k)
				return &cur.Values[i];
	}
	const V* Find(const K& k) const
	{
		ListPointers cur = GetPointersFromBuffer(Data);
		for (TU8 i = 0; i < ElementCount; i++)
			if (cur.Keys[i] == k)
				return &cur.Values[i];
	}
	void Insert(const K& k, const V& v)
	{
		if (auto e = Find(k))
		{
			*e = v;
			return;
		}
		ExpandIfNecessary(ElementCount + 1);
		// ByteVector pointer updated
		auto cur = GetPointersFromBuffer(Data);
		new ((void*)&cur.Keys[ElementCount]) K(k);
		new ((void*)&cur.Values[ElementCount]) V(v);
		ElementCount++;
	}
	V& operator[](const K& k)
	{
		if (auto r = Find(k))
			return *r;
		Insert(k, V());
		return *Find(k);
	}
	const V& operator[](const K& k) const
	{
		if (auto r = Find(k))
			return *r;
		TCORE_SOFT_CHECK(false, "Failed to find element on const access");
		return V();
	}
};

} // namespace TCore

#ifndef TCORE_DONT_OVERRIDE_NEW_DELETE
void* operator new(size_t sz)
{
	return TCAllocator->Malloc(TCore::GSuperMemoryBlock, sz, "Overriden default new()");
}

void operator delete(void* ptr)
{
	TCAllocator->Free(ptr);
}
#endif

#define TCORE_PLUGIN_MEMORY_BLOCK_INIT() TCSuperBlock TCore::GSuperMemoryBlock = nullptr;
// Call this inside the entry point
#define TCORE_PLUGIN_RESERVE_ADDRESS_SPACE(size)                                                                       \
	TCore::GSuperMemoryBlock =                                                                                         \
		TCAllocator->CreateSuperBlock(size, TCORE_ACTIVE_PLUGIN_NAME, TCORE_SUPERBLOCKALLOCATIONSTRATEGY_STANDARD)

#endif