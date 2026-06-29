#define TCORE_USE_CPP_WRAPPER
#include "Allocator.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <cstring>

#include "VirtualMemory.h"
#include "UnitTestSystem.h"
#include "UnitTestSystem.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCAllocator)
TCORE_PLUGIN_INIT(TCVirtualMemory)
TCORE_PLUGIN_INIT(TCUnitTest)
TCORE_PLUGIN_MEMORY_BLOCK_INIT();

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCAllocator)
TCORE_PLUGIN_RESERVE_ADDRESS_SPACE(1 << 20);
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

	static void Initialize()
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
	TSize AllocationUsed = 0;
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

	void* Allocate(TSize size)
	{
		void* result = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(Ptr) + AllocationUsed);
		AllocationUsed += size;
		return result;
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
		return foundMemBlock->Ptr;
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
		return foundMemBlock->Ptr;
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
	TUint ElementSize = 0;
	TSize Count = 0;
	TSize Capacity = 0;
	TSize AllocationSize = 0;
	SuperBlock* ParentSuperBlock = nullptr;
	void* Data() { return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(this) + sizeof(Vector)); }
};
struct VectorServices
{
#define GetVector(data) reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(data) - sizeof(Vector))
	static void DestroyElements(void* data, TSize destroyCount)
	{
		Vector* hnd = GetVector(data);
		if (!hnd || destroyCount == 0)
			return;

		const TSize byteCount = hnd->ElementSize * destroyCount;
		memset(reinterpret_cast<unsigned char*>(data) + (hnd->ElementSize * (hnd->Count - destroyCount)), 0, byteCount);
		TCVirtualMemory->Decommit(
			reinterpret_cast<unsigned char*>(data) + (hnd->ElementSize * (hnd->Count - destroyCount)), byteCount);
	}
	static void AddElements(void* data, TSize elementCount)
	{
		Vector* hnd = GetVector(data);
		if (!hnd || elementCount == 0)
			return;

		uintptr_t d = reinterpret_cast<uintptr_t>(data);
		TCVirtualMemory->Commit(reinterpret_cast<void*>(d + (hnd->ElementSize * hnd->Count)),
								elementCount * hnd->ElementSize);
		hnd->Count += elementCount;
	}
	static TCResult Resize(TCVectorHandle data, TUint newItemCount)
	{
		Vector* hnd = GetVector(data);
		if (!hnd || newItemCount <= hnd->Capacity)
			return TC_RESULT_SUCCESS;

		const TSize newCapacity = std::max<TSize>(newItemCount, hnd->Capacity ? hnd->Capacity * 2 : 8);
		if (newCapacity <= hnd->Capacity)
		{
			hnd->Count = newItemCount;
			return TC_RESULT_SUCCESS;
		}

		const TSize newAllocationSize = sizeof(Vector) + (newCapacity * hnd->ElementSize);
		Vector* newVector = reinterpret_cast<Vector*>(hnd->ParentSuperBlock->Allocate(newAllocationSize));
		if (!newVector)
			return TC_RESULT_OUT_OF_MEMORY;

		TCVirtualMemory->Commit(newVector, newAllocationSize);
		newVector->ElementSize = hnd->ElementSize;
		newVector->Count = hnd->Count;
		newVector->Capacity = newCapacity;
		newVector->AllocationSize = newAllocationSize;
		newVector->ParentSuperBlock = hnd->ParentSuperBlock;
		std::memcpy(newVector->Data(), hnd->Data(), hnd->Count * hnd->ElementSize);
		TCVirtualMemory->Decommit(hnd, hnd->AllocationSize);
		return TC_RESULT_SUCCESS;
	}
	static TCVectorHandle Create(TCSuperBlockHandle super_block, TUint element_size, TSize initial_size, TSize max_size)
	{
		auto superBlock = SuperBlock::GetSuperBlockFromHandle(super_block);
		if (!superBlock)
			return nullptr;

		const TSize capacity = std::max<TSize>(initial_size, max_size ? max_size : 1);
		const TSize reserveSize = std::max<TSize>(sizeof(Vector) + (element_size * capacity), GPageSize);
		Vector* vector = reinterpret_cast<Vector*>(superBlock->Allocate(reserveSize));
		if (!vector)
			return nullptr;

		TCVirtualMemory->Commit(vector, reserveSize);
		vector->Count = 0;
		vector->ElementSize = element_size;
		vector->Capacity = capacity;
		vector->AllocationSize = reserveSize;
		vector->ParentSuperBlock = superBlock;
		return reinterpret_cast<TCVectorHandle>(vector->Data());
	}
	static TSize Size(const TCVectorHandle data)
	{
		Vector* hnd = GetVector(data);
		return hnd ? hnd->Count : 0;
	}
	static TSize Capacity(const TCVectorHandle data)
	{
		Vector* hnd = GetVector(data);
		return hnd ? hnd->Capacity : 0;
	}
	static TCResult PushBack(TCVectorHandle data, const void* src)
	{
		Vector* hnd = GetVector(data);
		if (!hnd)
			return TC_RESULT_INVALID_ARGUMENT;
		if (hnd->Count >= hnd->Capacity)
		{
			TCResult result = Resize(data, hnd->Count + 1);
			if (result != TC_RESULT_SUCCESS)
				return result;
			hnd = GetVector(data);
		}

		void* dstElement = reinterpret_cast<unsigned char*>(hnd->Data()) + (hnd->ElementSize * hnd->Count);
		TCVirtualMemory->Commit(dstElement, hnd->ElementSize);
		if (src)
			memcpy(dstElement, src, hnd->ElementSize);
		hnd->Count++;
		return TC_RESULT_SUCCESS;
	}
	static TCResult Erase(TCVectorHandle data, TSize elementIndex)
	{
		Vector* hnd = GetVector(data);
		if (!hnd || elementIndex >= hnd->Count)
			return TC_RESULT_INVALID_ARGUMENT;

		memmove(reinterpret_cast<unsigned char*>(hnd->Data()) + (hnd->ElementSize * elementIndex),
				reinterpret_cast<unsigned char*>(hnd->Data()) + (hnd->ElementSize * (elementIndex + 1)),
				hnd->ElementSize * (hnd->Count - elementIndex - 1));

		if (((hnd->ElementSize * hnd->Count) % GPageSize) == 0)
			TCVirtualMemory->Decommit(reinterpret_cast<unsigned char*>(hnd->Data()) + hnd->ElementSize * hnd->Count,
									  GPageSize);

		hnd->Count--;
		return TC_RESULT_SUCCESS;
	}
	static void Destroy(TCVectorHandle v)
	{
		Vector* hnd = GetVector(v);
		if (!hnd)
			return;
		DestroyElements(hnd->Data(), hnd->Count);
		TCVirtualMemory->Decommit(hnd, hnd->AllocationSize);
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

namespace TCore
{
namespace Allocator
{

struct TCAllocatorUnitTests
{
	static unsigned int StandardAllocatorTest(TCReadBuffer inputData);
	static unsigned int EndOfPageAllocatorTest(TCReadBuffer inputData);
	static unsigned int VectorManagerTest(TCReadBuffer inputData);
	static void Register();
};

unsigned int TCAllocatorUnitTests::StandardAllocatorTest(TCReadBuffer inputData)
{
	(void)inputData;
	if (!TCAllocator || !TCAllocator->StandardAllocator || !TCAllocator->StandardAllocator->CreateSuperBlock ||
		!TCAllocator->StandardAllocator->Malloc || !TCAllocator->StandardAllocator->Free ||
		!TCAllocator->StandardAllocator->DestroySuperBlock)
		return 1;

	TCSuperBlockHandle superBlock = TCAllocator->StandardAllocator->CreateSuperBlock(4096, "TCAllocatorStdTest");
	if (!superBlock)
		return 2;

	void* allocation = TCAllocator->StandardAllocator->Malloc(superBlock, 128, "TCAllocatorStdTest");
	if (!allocation)
	{
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return 3;
	}

	std::memset(allocation, 0x5A, 128);
	TCAllocator->StandardAllocator->Free(allocation);
	TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
	return 0;
}

unsigned int TCAllocatorUnitTests::EndOfPageAllocatorTest(TCReadBuffer inputData)
{
	(void)inputData;
	if (!TCAllocator || !TCAllocator->EndOfPageAllocator || !TCAllocator->EndOfPageAllocator->CreateSuperBlock ||
		!TCAllocator->EndOfPageAllocator->Malloc || !TCAllocator->EndOfPageAllocator->Free ||
		!TCAllocator->EndOfPageAllocator->DestroySuperBlock)
		return 1;

	TCSuperBlockHandle superBlock = TCAllocator->EndOfPageAllocator->CreateSuperBlock(4096, "TCAllocatorEopTest");
	if (!superBlock)
		return 2;

	void* allocation = TCAllocator->EndOfPageAllocator->Malloc(superBlock, 64, "TCAllocatorEopTest");
	if (!allocation)
	{
		TCAllocator->EndOfPageAllocator->DestroySuperBlock(superBlock);
		return 3;
	}

	std::memset(allocation, 0x7C, 64);
	TCAllocator->EndOfPageAllocator->Free(allocation);
	TCAllocator->EndOfPageAllocator->DestroySuperBlock(superBlock);
	return 0;
}

unsigned int TCAllocatorUnitTests::VectorManagerTest(TCReadBuffer inputData)
{
	(void)inputData;
	if (!TCAllocator || !TCAllocator->VectorManager || !TCAllocator->VectorManager->Create ||
		!TCAllocator->VectorManager->PushBack || !TCAllocator->VectorManager->Size ||
		!TCAllocator->VectorManager->Erase || !TCAllocator->VectorManager->Destroy)
		return 1;

	TCSuperBlockHandle superBlock = TCAllocator->StandardAllocator->CreateSuperBlock(8192, "TCAllocatorVectorTest");
	if (!superBlock)
		return 2;

	TCVectorHandle vector = TCAllocator->VectorManager->Create(superBlock, sizeof(int), 0, 4);
	if (!vector)
	{
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return 3;
	}

	int values[3] = {10, 20, 30};
	for (int i = 0; i < 3; ++i)
	{
		if (TCAllocator->VectorManager->PushBack(vector, &values[i]) != TC_RESULT_SUCCESS)
		{
			TCAllocator->VectorManager->Destroy(vector);
			TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
			return 4;
		}
	}

	if (TCAllocator->VectorManager->Size(vector) != 3)
	{
		TCAllocator->VectorManager->Destroy(vector);
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return 5;
	}

	if (TCAllocator->VectorManager->Erase(vector, 1) != TC_RESULT_SUCCESS)
	{
		TCAllocator->VectorManager->Destroy(vector);
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return 6;
	}

	if (TCAllocator->VectorManager->Size(vector) != 2)
	{
		TCAllocator->VectorManager->Destroy(vector);
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return 7;
	}

	int* data = reinterpret_cast<int*>(vector);
	if (data[0] != 10 || data[1] != 30)
	{
		TCAllocator->VectorManager->Destroy(vector);
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return 8;
	}

	TCAllocator->VectorManager->Destroy(vector);
	TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
	return 0;
}

void TCAllocatorUnitTests::Register()
{
	if (!TCUnitTest)
		return;

	TCUnitTestDescription desc{};
	desc.GlobalCategoryName = "TCore";
	desc.Data = {nullptr, 0};

	desc.Name = "TCAllocator_UnitTest_StandardAllocator";
	desc.Test = StandardAllocatorTest;
	TCUnitTest->RegisterTest(&desc);

	desc.Name = "TCAllocator_UnitTest_EndOfPageAllocator";
	desc.Test = EndOfPageAllocatorTest;
	TCUnitTest->RegisterTest(&desc);

	desc.Name = "TCAllocator_UnitTest_VectorManager";
	desc.Test = VectorManagerTest;
	TCUnitTest->RegisterTest(&desc);
}

} // namespace Allocator
} // namespace TCore
TCResult TCAllocator_Initialize(const void** outPluginAPI)
{
	TCore::Allocator::TCAllocatorContext::Initialize();
	if (TCUnitTest)
		TCore::Allocator::TCAllocatorUnitTests::Register();
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
