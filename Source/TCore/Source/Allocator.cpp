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

static constexpr TSize kMaxSuperBlockName = 254; // You should use null terminator
static constexpr TSize kMaxAddressSpaceSizePerSuperBlockInternals =
	1 << 24; // Reserved address space size for each super block's internal usage
static constexpr TSize kMaxSuperBlockCount = 1 << 10;
static constexpr TSize kMaxAddressSpaceForAllSuperBlocks =
	kMaxAddressSpaceSizePerSuperBlockInternals * kMaxSuperBlockCount;

// These can be gathered after reload too
static TSize GPageSize = 0;

void BindVectorAndAllocatorServices(ITCAllocator* allocator);
struct TCAllocatorContext* GContext = nullptr;
struct TCAllocatorContext
{
	uint32_t SuperBlockCount = 0;
	SuperBlock* SuperBlocks = nullptr;

	static TCSuperBlockHandle AllocateSuperMemoryBlock(TSize blockSize, const char* superMemBlockName)
	{
		void* alloc = TCVirtualMemory->Reserve(blockSize);
		if (!alloc)
		{
			printf("AllocateSuperMemoryBlock() failed because virtual memory reserve failed\n");
			return nullptr;
		}

		// Search for a freed super memory block
		uint32_t namelen = std::min(strlen(superMemBlockName), kMaxSuperBlockName);
		for (uint32_t i = 0; i < GContext->SuperBlockCount; i++)
		{
			SuperBlock* super = &GContext->SuperBlocks[i];
			if (!super->info.isFreed)
			{
				continue;
			}

			super->info.blockSize = blockSize;
			super->info.isFreed = false;
			super->info.ptr = alloc;
			memcpy(super->info.name, superMemBlockName, namelen * sizeof(char));
			return (TCSuperBlockHandle)super;
		}

		// There is no freed super memory block, so allocate new one
		SuperBlock* super = &GContext->SuperBlocks[GContext->SuperBlockCount++];
		TCVirtualMemory->Commit(super, GPageSize);
		super->info.blockSize = blockSize;
		super->info.isFreed = false;
		super->info.ptr = alloc;
		memcpy(super->info.name, superMemBlockName, namelen * sizeof(char));

		for (unsigned int i = 0; i < firstPageMemBlockCount; i++)
		{
			super->blocks[i] = MemoryBlock();
		}
		super->info.activeBlockCount = firstPageMemBlockCount;
		// Committed memory will be 0 initialized, so memBlocks will be 0 initialized too
		return (TCSuperBlockHandle)super;
	}
	static void FreeSuperMemoryBlock(TCSuperBlockHandle super_memory_block)
	{
		if (uintptr_t(super_memory_block) - uintptr_t(GContext->SuperBlocks) % sizeof(SuperBlock))
		{
			printf("FreeSuperMemoryBlock() failed because super memory block pointer is invalid!");
			return;
		}
		auto superMemBlock = (SuperBlock*)super_memory_block;
		// Free all memory that superMemBlock externally has
		TCVirtualMemory->Free(superMemBlock->info.ptr, superMemBlock->info.blockSize);
		// Decommit superMemBlock's internal memory block infos except first page
		TCVirtualMemory->Decommit((void*)(uintptr_t(superMemBlock) + GPageSize), sizeof(SuperBlock) - GPageSize);
		// Default initialize first page's memory block infos
		for (unsigned int i = 0; i < firstPageMemBlockCount; i++)
		{
			superMemBlock->blocks[i] = MemoryBlock();
		}
		// Info: Set as freed, memset name as 0
		superMemBlock->info.activeBlockCount = firstPageMemBlockCount;
		superMemBlock->info.blockSize = 0;
		superMemBlock->info.isFreed = true;
		memset(superMemBlock->info.name, 0, kMaxSuperBlockName);
		superMemBlock->info.ptr = nullptr;
	}
	SuperBlock* GetSuperBlockFromHandle(TCSuperBlockHandle handle)
	{
		if (uintptr_t(handle) - uintptr_t(GContext->SuperBlocks) % sizeof(SuperBlock))
		{
			printf("GetSuperBlockFromHandle() failed because super memory block pointer is invalid!");
			return nullptr;
		}
		return (SuperBlock*)handle;
	}

	TCAllocatorContext()
	{
		GContext = this;
		GPageSize = TCVirtualMemory->GetPageSize();
		firstPageMemBlockCount = (GPageSize - sizeof(SuperBlock::SuperBlockInfo)) / sizeof(MemoryBlock);
		memBlockCountPerPage = GPageSize / sizeof(MemoryBlock);

		ITCAllocator* services = new ITCAllocator;
		services->AllocateSuperMemoryBlock = TCAllocatorContext::AllocateSuperMemoryBlock;
		services->FreeSuperMemoryBlock = TCAllocatorContext::FreeSuperMemoryBlock;

		BindVectorAndAllocatorServices(services);

		TCAllocator = services;
	}
};

struct SuperBlock;
struct MemoryBlock
{
	TSize Size = 0;
	bool IsFree = true;
	SuperBlock* ParentSuperBlock = nullptr;
	// For validation
	void* Ptr = nullptr;
};

struct SuperBlock
{
	struct SuperBlockInfo
	{
		char name[kMaxSuperBlockName];
		bool isFreed = true;
		unsigned long long blockSize;
		unsigned int activeBlockCount = 0;
		void* ptr = nullptr;
	};
	SuperBlockInfo info;
	static constexpr uint32_t kMaxBlockCountPerSuperBlock =
		(kMaxAddressSpaceSizePerSuperBlockInternals - sizeof(SuperBlockInfo)) / sizeof(MemoryBlock);
	MemoryBlock blocks[kMaxBlockCountPerSuperBlock];
};

static uint32_t firstPageMemBlockCount;
static uint32_t memBlockCountPerPage;

void* AllocateFromSuperMemoryBlock(SuperBlock* superMemBlock, TSize blockSize)
{
	if ((uintptr_t(superMemBlock) - uintptr_t(GContext->SuperBlocks)) % sizeof(SuperBlock))
	{
		printf("AllocateFromSuperMemoryBlock() failed. Super memory block pointer is invalid!\n");
		return nullptr;
	}
	uintptr_t lastVirmemAddress = reinterpret_cast<uintptr_t>(superMemBlock->info.ptr);
	uint32_t memBlockIndex = UINT32_MAX;
	for (uint32_t i = 0; i < superMemBlock->info.activeBlockCount; i++)
	{
		MemoryBlock& curBlock = superMemBlock->blocks[i];
		// This is an unused block info
		if (curBlock.size == 0)
		{
			memBlockIndex = i;
			break;
		}
		if (curBlock.isFree && curBlock.size >= blockSize)
		{
			curBlock.isFree = false;
			return reinterpret_cast<void*>(lastVirmemAddress);
		}
		lastVirmemAddress += curBlock.size;
	}
	// If requested block can't fit (remaining space - last page), fail
	// Last page better be unused
	if (lastVirmemAddress + blockSize - reinterpret_cast<uintptr_t>(superMemBlock->info.ptr) >=
		superMemBlock->info.blockSize - GPageSize)
	{
		printf("Request exceed super memory block\n");
		return nullptr;
	}
	// If all active memory blocks does allocate memory, create new block info with commiting a page
	if (memBlockIndex == UINT32_MAX)
	{
		memBlockIndex = superMemBlock->info.activeBlockCount++;
		TCVirtualMemory->Commit(superMemBlock->blocks + memBlockIndex, GPageSize);
		for (uint32_t i = 1; i < memBlockCountPerPage; i++)
		{
			superMemBlock->blocks[i + memBlockIndex] = MemoryBlock();
		}
		superMemBlock->info.activeBlockCount += memBlockCountPerPage;
	}
	MemoryBlock& finalBlock = superMemBlock->blocks[memBlockIndex];
	finalBlock.isFree = false;
	finalBlock.size = blockSize;
	return reinterpret_cast<void*>(lastVirmemAddress);
}
void FreeFromSuperMemoryBlock(SuperBlock* superMemBlock, void* allocation)
{
	if (uintptr_t(superMemBlock) - uintptr_t(GContext->SuperBlocks) % sizeof(SuperBlock))
	{
		printf("FreeFromSuperMemoryBlock() failed. Invalid super memory block\n");
		return;
	}
	uintptr_t lastVirmemAddress = reinterpret_cast<uintptr_t>(superMemBlock->info.ptr);
	for (uint32_t i = 0; i < superMemBlock->info.activeBlockCount; i++)
	{
		MemoryBlock& curBlock = superMemBlock->blocks[i];
		if (lastVirmemAddress == reinterpret_cast<uintptr_t>(allocation))
		{
			curBlock.IsFree = true;
			TCVirtualMemory->Decommit(allocation, curBlock.size);
			return;
		}
		if (lastVirmemAddress >= reinterpret_cast<uintptr_t>(allocation) || !curBlock.size)
		{
			printf("FreeFromSuperMemoryBlock() failed. Invalid allocation pointer\n");
			return;
		}
		lastVirmemAddress += curBlock.size;
	}
}

// Mostly for debug purposes
struct EndOfPageAllocatorServices
{
	// Based on The Machinery's end of page allocator
	static void* Malloc(TCSuperBlockHandle super_block, TSize size, const char* name)
	{
		const TSize RequiredAllocSize = size + sizeof(MemoryBlock);
		// Requested size is rounded up to GPageSize (these pages will be committed)
		//  Then adds a new page (this page won't be committed)
		const TSize eopSize = RequiredAllocSize + (GPageSize - (RequiredAllocSize % GPageSize)) + GPageSize;

		void* base = AllocateFromSuperMemoryBlock((SuperBlock*)super_block, eopSize);
		TCVirtualMemory->Commit(base, eopSize - GPageSize);
		// Memory Layout: offsettedSpace (less than a page) - sizeVariable (unsigned int) - data
		// (requestedSize) - reserved (non-committed) last page (page size)
		MemoryBlock* memBlock = reinterpret_cast<MemoryBlock*>(reinterpret_cast<uintptr_t>(base) +
															   (GPageSize - (RequiredAllocSize % GPageSize)));

		memBlock->IsFree = false;
		memBlock->ParentSuperBlock = (SuperBlock*)super_block;
		memBlock->Ptr = memBlock + 1;
		memBlock->Size = RequiredAllocSize;
	}
	static void Free(void* allocation)
	{
		MemoryBlock* memBlock = (MemoryBlock*)allocation - 1;
		const TSize requiredAllocSize = memBlock->Size;
		if (!requiredAllocSize || requiredAllocSize == UINT64_MAX)
		{
			printf("End of Page allocator failed to free the allocation!");
			return;
		}
#ifndef NDEBUG
		if (memBlock->Ptr != allocation)
		{
			printf("End of Page allocator failed to free the allocation! Invalid pointer\n");
			return;
		}
		if (memBlock->ParentSuperBlock->info.ptr != memBlock->ParentSuperBlock)
		{
			printf("End of Page allocator failed to free the allocation! Invalid super block pointer\n");
			return;
		}
#endif
		const TSize eopSize = requiredAllocSize + (GPageSize - (requiredAllocSize % GPageSize)) + GPageSize;
		void* base = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(allocation) -
											 (GPageSize - (requiredAllocSize % GPageSize)));
		FreeFromSuperMemoryBlock(memBlock->ParentSuperBlock, base);
	}
};

// Pretty standard allocator, just allocates the requested size + sizeof(MemoryBlock)
struct StandardAllocatorServices
{
	static void* Malloc(TCSuperBlockHandle memBlock, TSize size, const char* name)
	{
		const TSize requiredAllocSize = size + sizeof(MemoryBlock);

		void* base = AllocateFromSuperMemoryBlock((SuperBlock*)memBlock, requiredAllocSize);
		TCVirtualMemory->Commit(base, requiredAllocSize);
		return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(base) + sizeof(MemoryBlock));
	}
	static void Free(void* allocation)
	{
		MemoryBlock* memBlock = (MemoryBlock*)allocation - 1;
		const TSize requiredAllocSize = memBlock->Size;
		if (!requiredAllocSize || requiredAllocSize == UINT64_MAX)
		{
			printf("Standard allocator failed to free the allocation!");
			return;
		}
#ifndef NDEBUG
		if (memBlock->Ptr != allocation)
		{
			printf("Standard allocator failed to free the allocation! Invalid pointer\n");
			return;
		}
		if (memBlock->ParentSuperBlock->info.ptr != memBlock->ParentSuperBlock)
		{
			printf("Standard allocator failed to free the allocation! Invalid super block pointer\n");
			return;
		}
#endif
		FreeFromSuperMemoryBlock(memBlock->ParentSuperBlock, memBlock);
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
		TSize reserveSize = std::max(sizeof(Vector) + (element_size * max_size), GPageSize);
		if (max_size == 0)
		{
			// Either allocate a page or list of pages based on element size
		}
		Vector* vector =
			(Vector*)AllocateFromSuperMemoryBlock(GContext->GetSuperBlockFromHandle(super_block), reserveSize);
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
		Allocator->Malloc = StandardAllocatorServices::Malloc;
		Allocator->Free = StandardAllocatorServices::Free;
		services->StandardAllocator = Allocator;
	}

	// End of Page Buffer Allocator
	{
		ITCBuffer* bufAllocator = (ITCBuffer*)malloc(sizeof(ITCBuffer));
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
	new TCore::Allocator::TCAllocatorContext();
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
