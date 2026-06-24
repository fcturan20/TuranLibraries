#define T_INCLUDE_PLATFORM_LIBS
#include "Allocator.h"

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
TCORE_PLUGIN_HARD_DEPENDENCY(TCVirtualMemory, TCVirtualMemory_PLUGIN_VERSION)
TCORE_PLUGIN_ENTRY_POINT_END()

namespace TCore
{
namespace Allocator
{

static constexpr TSize kMaxBlockName = 230; // You should use null terminator

// These can be gathered after reload too
static TSize GPageSize = 0;

void BindVectorAndAllocatorServices(ITCAllocator* allocator);
struct TCAllocatorContext* GContext = nullptr;
struct SuperBlock;
struct TCAllocatorContext
{
	TCore::Vector<SuperBlock*, true> SuperBlocks;

	void Initialize()
	{
		ITCAllocator* services = new ITCAllocator;
		BindVectorAndAllocatorServices(services);
		TCAllocator = services;

		GContext = new TCAllocatorContext;
		GPageSize = TCVirtualMemory->GetPageSize();
	}
};

struct SuperBlock
{
	char Name[kMaxBlockName + 1];
	bool IsFreed = true;
	TSize AllocationSize;
	const ITCBuffer* Allocator = nullptr;

	// Usable memory pointer for the user
	// Should be aligned to 8 bytes, so that MemoryBlock's alignment is not broken
	void* Ptr = nullptr;
	// Validate if this super block is valid
	bool IsValid()
	{
		for (size_t i = 0; i < GContext->SuperBlocks.Size(); i++)
		{
			if (GContext->SuperBlocks[i] == this)
			{
				if (Ptr == reinterpret_cast<void*>(uintptr_t(this) + GPageSize))
					return true;
				else
					return false;
			}
		}
		return false;
	}

	TCSuperBlockHandle GetHandle()
	{
		TCORE_SOFT_CHECK(IsValid(), "Invalid super block handle");
		return (TCSuperBlockHandle)this;
	}
	static SuperBlock* GetFromHandle(TCSuperBlockHandle hnd)
	{
		auto block = (SuperBlock*)hnd;
		TCORE_SOFT_CHECK(block->IsValid(), "Invalid super block");
		return block;
	}

	static TCSuperBlockHandle Create(TSize block_size, const char* name, const ITCBuffer* allocator)
	{
		// Search for a freed super memory block
		uint32_t namelen = std::min(strlen(name), kMaxBlockName);
		for (uint32_t i = 0; i < GContext->SuperBlocks.Size(); i++)
		{
			SuperBlock* super = GContext->SuperBlocks[i];
			if (!super->IsFreed)
				continue;

			// If requested allocation is less than %75 of this block, skip this
			if (super->AllocationSize < block_size || super->AllocationSize * 3ull > block_size * 4ull)
				continue;

			super->IsFreed = false;
			memcpy(super->Name, name, namelen * sizeof(char));
			return (TCSuperBlockHandle)super;
		}

		// There is no available super memory block, so allocate new one
		TSize roundedSize = block_size + (GPageSize - (block_size % GPageSize));
		TSize blockSize = roundedSize + GPageSize; // One extra page for super memory block struct
		SuperBlock* block = (SuperBlock*)TCVirtualMemory->Reserve(blockSize);
		if (!block)
		{
			printf("AllocateSuperMemoryBlock() failed because virtual memory reserve failed\n");
			return nullptr;
		}

		TCVirtualMemory->Commit(block, GPageSize);
		block->AllocationSize = block_size;
		block->Allocator = allocator;
		block->IsFreed = false;
		block->Ptr = reinterpret_cast<void*>(uintptr_t(block) + GPageSize);
		memcpy(block->Name, name, namelen * sizeof(char));
		GContext->SuperBlocks.PushBack(block);

		return block->GetHandle();
	}
	void Destroy()
	{
		TCVirtualMemory->Decommit(Ptr, AllocationSize);
		IsFreed = true;
	}
	static SuperBlock* GetSuperBlockFromHandle(TCSuperBlockHandle handle)
	{
		SuperBlock* block = (SuperBlock*)handle;
		TCORE_SOFT_CHECK(block->IsValid(), "Invalid super block handle");
		return block;
	}
};

struct MemoryBlock
{
	TSize Size;
	bool IsFree;
	SuperBlock* ParentSuperBlock;
	char Name[kMaxBlockName];
#ifndef NDEBUG
	// For validation
	void* Ptr = nullptr;
#endif

	bool IsValid()
	{
		if (!ParentSuperBlock->IsValid())
			return false;
		if (!Size || Size == UINT64_MAX)
			return false;
#ifndef NDEBUG
		if (uintptr_t(Ptr) != uintptr_t(this + 1))
			return false;
#endif // !NDEBUG
		return true;
	}
};

// Mostly for debug purposes
struct EndOfPageAllocatorServices
{
	static TCSuperBlockHandle CreateSuperBlock(TSize blockSize, const char* superMemBlockName)
	{
		return SuperBlock::Create(blockSize, superMemBlockName, TCEopAllocator);
	}
	static void DestroySuperBlock(TCSuperBlockHandle super_memory_block)
	{
		auto super = SuperBlock::GetFromHandle(super_memory_block);
		TCORE_SOFT_CHECK(super, "Super block handle invalid");
		return super->Destroy();
	}

	inline static TSize CalculateFinalAllocSize(TSize requestedAllocSize)
	{
		TSize roundedUpSize = requestedAllocSize + (GPageSize - (requestedAllocSize % GPageSize));
		TSize totalAlloc = GPageSize * 3 + roundedUpSize;
		return totalAlloc;
	}
	inline static MemoryBlock* FindNextBlock(MemoryBlock* block)
	{
		return reinterpret_cast<MemoryBlock*>(uintptr_t(block) + CalculateFinalAllocSize(block->Size));
	}

	// Based on The Machinery's end of page allocator
	static MemoryBlock* GetMemoryBlockFromAllocationPointer(void* alloc)
	{
		auto block = reinterpret_cast<MemoryBlock*>(uintptr_t(alloc) - 2 * GPageSize);
		TCORE_SOFT_CHECK(block->IsValid(), "Invalid block");
		TCORE_SOFT_CHECK(block->ParentSuperBlock->Allocator == TCEopAllocator, "Allocator mismatch");
		return block;
	}
	static void* Malloc(TCSuperBlockHandle super_block, TSize size, const char* name)
	{
		auto totalAlloc = CalculateFinalAllocSize(size);

		auto superBlock = SuperBlock::GetFromHandle(super_block);
		TCORE_SOFT_CHECK(superBlock->Allocator == TCEopAllocator, "Allocator mismatch");

		MemoryBlock* foundMemBlock = nullptr;
		auto activeMemBlock = (MemoryBlock*)superBlock->Ptr;
		while (activeMemBlock->IsValid())
		{
			if (activeMemBlock->IsFree)
				if (activeMemBlock->Size > size && activeMemBlock->Size * 3ull <= size * 4ull)
				{
					foundMemBlock = activeMemBlock;
					break;
				}
			activeMemBlock = FindNextBlock(activeMemBlock);
		}
		// None of the valid memory blocks are suitable for this request
		if (!foundMemBlock)
		{
			foundMemBlock = activeMemBlock;
			*foundMemBlock = {};
			foundMemBlock->Ptr = reinterpret_cast<void*>(uintptr_t(foundMemBlock) + 2 * GPageSize);
			foundMemBlock->ParentSuperBlock = superBlock;
			foundMemBlock->Size = size;
		}
		foundMemBlock->IsFree = false;
		TUint nameLen = std::min(std::strlen(name), kMaxBlockName);
		std::memcpy(foundMemBlock->Name, name, nameLen);

		TCVirtualMemory->Commit(foundMemBlock->Ptr, size);

		// Commit and zero initialize next block
		MemoryBlock* nextBlock = FindNextBlock(foundMemBlock);
		TCVirtualMemory->Commit(nextBlock, GPageSize);
		*nextBlock = {};
	}
	static void Free(void* allocation)
	{
		auto memBlock = GetMemoryBlockFromAllocationPointer(allocation);
		TCVirtualMemory->Decommit(allocation, memBlock->Size);
		memBlock->IsFree = true;
	}
};

// Pretty standard allocator, just allocates the sizeof(MemoryBlock) + requested size
struct StandardAllocatorServices
{
	static TCSuperBlockHandle CreateSuperBlock(TSize blockSize, const char* superMemBlockName)
	{
		return SuperBlock::Create(blockSize, superMemBlockName, TCStdAllocator);
	}
	static void DestroySuperBlock(TCSuperBlockHandle super_memory_block)
	{
		auto super = SuperBlock::GetFromHandle(super_memory_block);
		TCORE_SOFT_CHECK(super, "Super block handle invalid");
		return super->Destroy();
	}

	inline static MemoryBlock* FindNextBlock(MemoryBlock* block)
	{
		return reinterpret_cast<MemoryBlock*>(uintptr_t(block) + block->Size);
	}
	static MemoryBlock* GetMemoryBlockFromAllocationPointer(void* allocation)
	{
		MemoryBlock* memBlock = (MemoryBlock*)allocation - 1;
		TCORE_SOFT_CHECK(memBlock->IsValid(), "Invalid memory allocation");
		TCORE_SOFT_CHECK(memBlock->ParentSuperBlock->Allocator == TCStdAllocator, "Allocator mismatch");
		return memBlock;
	}
	static void* Malloc(TCSuperBlockHandle super_block, TSize size, const char* name)
	{
		const TSize requiredAllocSize = size + sizeof(MemoryBlock);
		auto superBlock = SuperBlock::GetSuperBlockFromHandle(super_block);

		MemoryBlock* foundMemBlock = nullptr;
		auto activeMemBlock = (MemoryBlock*)superBlock->Ptr;
		// Last memory block is always invalid
		while (activeMemBlock->IsValid())
		{
			if (activeMemBlock->IsFree)
				if (activeMemBlock->Size > size && activeMemBlock->Size * 3ull <= size * 4ull)
				{
					foundMemBlock = activeMemBlock;
					break;
				}
		}
		// None of the valid memory blocks are suitable for this request
		if (!foundMemBlock)
		{
			foundMemBlock = activeMemBlock;
			*foundMemBlock = {};
			foundMemBlock->Ptr = reinterpret_cast<void*>(uintptr_t(foundMemBlock) + sizeof(MemoryBlock));
			foundMemBlock->ParentSuperBlock = superBlock;
			foundMemBlock->Size = size;
		}
		foundMemBlock->IsFree = false;
		TUint nameLen = std::min(std::strlen(name), kMaxBlockName);
		std::memcpy(foundMemBlock->Name, name, nameLen);

		TCVirtualMemory->Commit(foundMemBlock->Ptr, size);

		// Commit and zero initialize next block
		MemoryBlock* nextBlock = FindNextBlock(foundMemBlock);
		TCVirtualMemory->Commit(nextBlock, sizeof(MemoryBlock));
		*nextBlock = {};
	}
	static void Free(void* allocation)
	{
		auto memBlock = GetMemoryBlockFromAllocationPointer(allocation);
		TCVirtualMemory->Decommit(allocation, memBlock->Size);
		memBlock->IsFree = true;
	}
};

struct Vector
{
	TUint ElementSize;
	TSize Count;
	void* Data() { return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(this) + sizeof(Vector)); }
};
struct VectorServices
{
#define GetVector(data) reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(data))
	static void DestroyElements(void* data, TSize destroyCount)
	{
		uintptr_t loc = reinterpret_cast<uintptr_t>(data) - sizeof(Vector);
		Vector* hnd = GetVector(data);
		// Set memory to zero then decommit it
		memset((unsigned char*)data + (hnd->ElementSize * (hnd->Count - destroyCount)),
			   0,
			   hnd->ElementSize * destroyCount);

		TCVirtualMemory->Decommit((unsigned char*)data + (hnd->ElementSize * (hnd->Count - destroyCount)),
								  hnd->ElementSize * destroyCount);
	}
	static void AddElements(void* data, TSize elementCount)
	{
		Vector* hnd = GetVector(data);
		uintptr_t d = reinterpret_cast<uintptr_t>(data);
		TCVirtualMemory->Commit(reinterpret_cast<void*>(d + (hnd->ElementSize * hnd->Count)),
								elementCount * hnd->ElementSize);
		hnd->Count += elementCount;
	}
	static TCResult Resize(TCVectorHandle data, TUint newItemCount)
	{
		Vector* hnd = GetVector(data);
		if (newItemCount > hnd->Capacity)
		{
			// Reserve a new space
		}
	}
	static TCVectorHandle Create(TCSuperBlockHandle super_block, TUint element_size, TSize initial_size, TSize max_size)
	{
		auto superBlock = GContext->GetSuperBlockFromHandle(super_block);
		if (!superBlock)
			return nullptr;
		TSize reserveSize = std::max(sizeof(Vector) + (element_size * max_size), GPageSize);
		if (max_size == 0)
		{
			// Either allocate a page or list of pages based on element size
		}
		Vector* vector = (Vector*)superBlock->Allocate(reserveSize);
		TCVirtualMemory->Commit(vector, sizeof(Vector));

		// Fill vector struct
		vector->Count = 0;
		vector->ElementSize = element_size;
		return (TCVectorHandle)vector;
	}
	static TSize Size(const TCVectorHandle data)
	{
		Vector* hnd = GetVector(data);
		return hnd->Count;
	}
	static TSize Capacity(const TCVectorHandle data)
	{
		Vector* hnd = GetVector(data);
		return hnd->Capacity;
	}
	static TCResult PushBack(TCVectorHandle data, const void* src)
	{
		Vector* hnd = GetVector(data);
		void* dstElement = (unsigned char*)data + (hnd->ElementSize * hnd->Count++);
		TCVirtualMemory->Commit(dstElement, hnd->ElementSize);
		if (src)
			memcpy(dstElement, src, hnd->ElementSize);
		return TC_RESULT_SUCCESS;
	}
	static TCResult Erase(TCVectorHandle data, TSize elementIndex)
	{
		Vector* hnd = GetVector(data);
		if (elementIndex >= hnd->Count)
			return TC_RESULT_INVALID_ARGUMENT;

		memmove((unsigned char*)data + (hnd->ElementSize * elementIndex),
				(unsigned char*)data + (hnd->ElementSize * (elementIndex + 1)),
				hnd->ElementSize * (hnd->Count - elementIndex - 1));

		if (((hnd->ElementSize * hnd->Count) % GPageSize) == 0)
			TCVirtualMemory->Decommit((unsigned char*)data + hnd->ElementSize * hnd->Count, GPageSize);

		return TC_RESULT_SUCCESS;
	}
	static void Destroy(TCVectorHandle v)
	{
		Vector* hnd = GetVector(v);
		DestroyElements(hnd->Data(), hnd->Count);
		FreeFromSuperMemoryBlock(hnd);
	}
};

void BindVectorAndAllocatorServices(ITCAllocator* services)
{
	// Standard Allocator
	{
		ITCBuffer* Allocator = (ITCBuffer*)malloc(sizeof(ITCBuffer));
		Allocator->CreateSuperBlock = StandardAllocatorServices::CreateSuperBlock;
		Allocator->DestroySuperBlock = StandardAllocatorServices::DestroySuperBlock;
		Allocator->Malloc = StandardAllocatorServices::Malloc;
		Allocator->Free = StandardAllocatorServices::Free;
		services->StandardAllocator = Allocator;
	}

	// End of Page Buffer Allocator
	{
		ITCBuffer* bufAllocator = (ITCBuffer*)malloc(sizeof(ITCBuffer));
		bufAllocator->CreateSuperBlock = EndOfPageAllocatorServices::CreateSuperBlock;
		bufAllocator->DestroySuperBlock = EndOfPageAllocatorServices::DestroySuperBlock;
		bufAllocator->Malloc = EndOfPageAllocatorServices::Malloc;
		bufAllocator->Free = EndOfPageAllocatorServices::Free;
		services->EndOfPageAllocator = bufAllocator;
	}

	// Vector Allocator
	{
		ITCVector* f_vector = (ITCVector*)malloc(sizeof(ITCVector));
		f_vector->Create = VectorServices::Create;
		f_vector->Erase = VectorServices::Erase;
		f_vector->Capacity = VectorServices::Capacity;
		f_vector->Size = VectorServices::Size;
		f_vector->PushBack = VectorServices::PushBack;
		f_vector->Resize = VectorServices::Resize;
		f_vector->Destroy = VectorServices::Destroy;
		services->VectorManager = f_vector;
	}
}

} // namespace Allocator
} // namespace TCore
TCResult TCAllocator_Initialize(const void** outPluginAPI)
{
	TCore::Allocator::TCAllocatorContext::Initialize();
	*outPluginAPI = TCAllocator;
	return TC_RESULT_SUCCESS;
}

TCResult TCAllocator_OnPreShutdown()
{
	return TC_RESULT_SUCCESS;
}

TCResult TCAllocator_Shutdown()
{
	return TC_RESULT_SUCCESS;
}

void TCAllocator_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded) {}
