#define TCORE_USE_CPP_WRAPPER
#include "Allocator.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <algorithm>

#include "VirtualMemory.h"
#include "UnitTestSystem.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCAllocator)
TCORE_PLUGIN_INIT(TCVirtualMemory)
TCORE_PLUGIN_INIT(TCUnitTest)
TCORE_PLUGIN_MEMORY_BLOCK_INIT();

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCAllocator)
TCORE_PLUGIN_HARD_DEPENDENCY(TCVirtualMemory, TCVirtualMemory_PLUGIN_VERSION)
TCORE_PLUGIN_ENTRY_POINT_END()

namespace TCore
{
namespace Allocator
{

static constexpr TU8 kMaxBlockName = 230; // You should use null terminator

// These can be gathered after reload too
static TU8 GPageSizeBitCount = 0;
static TU8 GHandleConstantSuffix = 0;

static inline TU8 PagesToBytes(TU8 pageCount)
{
	return pageCount << GPageSizeBitCount;
}

void BindVectorAndAllocatorServices(ITCAllocator* allocator);
struct TCAllocatorContext* GContext = nullptr;
struct SuperBlock;
struct TCAllocatorContext
{
	TCore::Vector<SuperBlock*, true> SuperBlocks;

	static void Initialize();
	SuperBlock* FindCollidingSuperBlock(void* ptr);
};

using MallocFunc = decltype(ITCAllocator::Malloc);
using FreeFunc = decltype(ITCAllocator::Free);

MallocFunc GetMallocFunc(TCSuperBlockAllocationStrategy strategy);
FreeFunc GetFreeFunc(TCSuperBlockAllocationStrategy strategy);

struct SuperBlock
{
#ifndef NDEBUG
	// Next page pointer
	// For validation
	void* Ptr = nullptr;
#endif

	char Name[kMaxBlockName + 1];
	bool IsFreed = true;
	TU8 PageCount;
	TCSuperBlockAllocationStrategy Strategy;

	// Validate if this super block is valid
	static bool IsValid(TCSuperBlock hnd)
	{
		TU8 handleSuffix = uintptr_t(hnd) << (64 - GPageSizeBitCount);
		if (handleSuffix != GHandleConstantSuffix)
			return false;
		auto block = reinterpret_cast<SuperBlock*>(uintptr_t(hnd) - handleSuffix);
		if (block->Ptr != reinterpret_cast<void*>(uintptr_t(block) + PagesToBytes(1)))
			return false;
		return true;
	}

	bool IsValid()
	{
		auto expectedPtr = reinterpret_cast<void*>(uintptr_t(this) + PagesToBytes(1));
		if (Ptr != expectedPtr)
			return false;
		return true;
	}

	TCSuperBlock GetHnd()
	{
		TCORE_SOFT_CHECK(IsValid(), "Invalid super block");
		TU8 address = uintptr_t(this);
		address += GHandleConstantSuffix;
		return (TCSuperBlock)address;
	}

	static SuperBlock* GetFromHnd(TCSuperBlock hnd)
	{
		TCORE_SOFT_CHECK(IsValid(hnd), "Invalid super block handle");
		auto block = reinterpret_cast<SuperBlock*>(uintptr_t(hnd) - GHandleConstantSuffix);
		return block;
	}

	template <bool UseContext>
	static TCSuperBlock Create(TU8 block_size, const char* name, TCSuperBlockAllocationStrategy strategy)
	{
		// Search for a freed super memory block
		TU8 namelen = std::min(TU8(strlen(name)), kMaxBlockName);
		if constexpr (UseContext)
		{
			for (uint32_t i = 0; i < GContext->SuperBlocks.Size(); i++)
			{
				SuperBlock* super = GContext->SuperBlocks[i];
				if (!super->IsFreed)
					continue;

				// If requested allocation is less than %75 of this block, skip this
				if (PagesToBytes(super->PageCount) < block_size ||
					PagesToBytes(super->PageCount) * 3ull > block_size * 4ull)
					continue;

				TCVirtualMemory->Commit(super, PagesToBytes(1));
				TCVirtualMemory->Commit(super->Ptr, PagesToBytes(1));
				super->IsFreed = false;
				memcpy(super->Name, name, namelen * sizeof(char));
				super->Strategy = strategy;
				return (TCSuperBlock)super;
			}
		}

		// There is no available super memory block, so allocate new one
		TU8 roundedSize = block_size + (PagesToBytes(1) - (block_size % PagesToBytes(1)));
		TU8 blockSize = roundedSize + PagesToBytes(1); // One extra page for super memory block struct
		SuperBlock* block = (SuperBlock*)TCVirtualMemory->Reserve(blockSize);
		if (!block)
		{
			printf("AllocateSuperMemoryBlock() failed because virtual memory reserve failed\n");
			return nullptr;
		}

		TCVirtualMemory->Commit(block, PagesToBytes(1));
		block->PageCount = block_size / PagesToBytes(1);
		block->Strategy = strategy;
		block->IsFreed = false;
		memcpy(block->Name, name, namelen * sizeof(char));
		block->Ptr = reinterpret_cast<void*>(uintptr_t(block) + PagesToBytes(1));
		TCVirtualMemory->Commit(block->Ptr, PagesToBytes(1));
		if constexpr (UseContext)
			GContext->SuperBlocks.PushBack(block);

		return block->GetHnd();
	}
	void Destroy()
	{
		TCVirtualMemory->Decommit(Ptr, PageCount);
		IsFreed = true;
	}
};
SuperBlock* TCAllocatorContext::FindCollidingSuperBlock(void* ptr)
{
	auto target = reinterpret_cast<uintptr_t>(ptr);
	for (TU8 i = 0; i < SuperBlocks.Size(); i++)
	{
		auto& superBlock = SuperBlocks[i];
		auto superBlockStart = reinterpret_cast<uintptr_t>(superBlock->Ptr);
		auto superBlockEnd = superBlockStart + PagesToBytes(superBlock->PageCount);
		if (target >= superBlockStart && target < superBlockEnd)
			return superBlock;
	}
	return nullptr;
}

static ITCAllocator a;
void TCAllocatorContext::Initialize()
{
	ITCAllocator* services = &a;
	BindVectorAndAllocatorServices(services);
	TCAllocator = services;

	TU8 pageSizeBytes = TCVirtualMemory->GetPageSize();
	GPageSizeBitCount = 0;
	while ((TU8(1) << GPageSizeBitCount) < pageSizeBytes)
		++GPageSizeBitCount;
	TCORE_SOFT_CHECK((TU8(1) << GPageSizeBitCount) == pageSizeBytes, "Page size is not a power of two");

	static constexpr TU8 kInitialAllocatorSize = 1ull << 20ull;
	auto reservedMemory = TCVirtualMemory->Reserve(kInitialAllocatorSize);
	TCVirtualMemory->Commit(reservedMemory, PagesToBytes(1));
	TCore::GSuperMemoryBlock = SuperBlock::Create<false>(
		kInitialAllocatorSize, TCORE_ACTIVE_PLUGIN_NAME, TCORE_SUPERBLOCKALLOCATIONSTRATEGY_EOP);

	GContext = new (reservedMemory) TCAllocatorContext;
}

struct MemoryBlock
{
	TU8 Size;
	bool IsFree;
	SuperBlock* ParentSuperBlock;
	char Name[kMaxBlockName];
	void* Ptr = nullptr;

	bool IsValid()
	{
		if (!ParentSuperBlock)
			return false;
		if (!ParentSuperBlock->IsValid())
			return false;
		if (!Size || Size == UINT64_MAX)
			return false;
		return true;
	}
};

// Mostly for debug purposes
struct EndOfPageAllocatorServices
{
	static TCSuperBlock CreateSuperBlock(TU8 blockSize, const char* superMemBlockName)
	{
		return SuperBlock::Create<true>(blockSize, superMemBlockName, TCORE_SUPERBLOCKALLOCATIONSTRATEGY_EOP);
	}
	static void DestroySuperBlock(TCSuperBlock super_memory_block)
	{
		auto super = SuperBlock::GetFromHnd(super_memory_block);
		TCORE_SOFT_CHECK(super, "Super block handle invalid");
		return super->Destroy();
	}

	inline static TU8 CalculateFinalAllocSize(TU8 requestedAllocSize)
	{
		TU8 roundedUpSize = requestedAllocSize + (PagesToBytes(1) - (requestedAllocSize % PagesToBytes(1)));
		TU8 totalAlloc = PagesToBytes(3) + roundedUpSize;
		return totalAlloc;
	}
	inline static MemoryBlock* FindNextBlock(MemoryBlock* block)
	{
		return reinterpret_cast<MemoryBlock*>(uintptr_t(block) + CalculateFinalAllocSize(block->Size));
	}

	// Based on The Machinery's end of page allocator
	static MemoryBlock* GetMemoryBlockFromAllocationPointer(void* alloc)
	{
		auto block = reinterpret_cast<MemoryBlock*>(uintptr_t(alloc) - PagesToBytes(2));
		TCORE_SOFT_CHECK(block->IsValid(), "Invalid block");
		TCORE_SOFT_CHECK(block->ParentSuperBlock->Strategy == TCORE_SUPERBLOCKALLOCATIONSTRATEGY_EOP,
						 "Allocator mismatch");
		return block;
	}
	static void* Malloc(TCSuperBlock super_block, TU8 size, const char* name)
	{
		auto totalAlloc = CalculateFinalAllocSize(size);

		auto superBlock = SuperBlock::GetFromHnd(super_block);
		TCORE_SOFT_CHECK(superBlock->Strategy == TCORE_SUPERBLOCKALLOCATIONSTRATEGY_EOP, "Allocator mismatch");

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
			foundMemBlock->Ptr = reinterpret_cast<void*>(uintptr_t(foundMemBlock) + PagesToBytes(2));
			foundMemBlock->ParentSuperBlock = superBlock;
			foundMemBlock->Size = size;
		}
		foundMemBlock->IsFree = false;
		TU4 nameLen = std::min(TU8(std::strlen(name)), kMaxBlockName);
		std::memcpy(foundMemBlock->Name, name, nameLen);

		TCVirtualMemory->Commit(foundMemBlock->Ptr, size);

		// Commit and zero initialize next block
		MemoryBlock* nextBlock = FindNextBlock(foundMemBlock);
		TCVirtualMemory->Commit(nextBlock, PagesToBytes(1));
		*nextBlock = {};
		return foundMemBlock->Ptr;
	}

	static void Free(void* allocation)
	{
		auto memBlock = GetMemoryBlockFromAllocationPointer(allocation);
		if (!memBlock)
			return;
		TCVirtualMemory->Decommit(allocation, memBlock->Size);
		memBlock->IsFree = true;
	}
};

// Pretty standard allocator, just allocates the sizeof(MemoryBlock) + requested size
struct StandardAllocatorServices
{
	static TCSuperBlock CreateSuperBlock(TU8 blockSize, const char* superMemBlockName)
	{
		return SuperBlock::Create<true>(blockSize, superMemBlockName, TCORE_SUPERBLOCKALLOCATIONSTRATEGY_STANDARD);
	}
	static void DestroySuperBlock(TCSuperBlock super_memory_block)
	{
		auto super = SuperBlock::GetFromHnd(super_memory_block);
		TCORE_SOFT_CHECK(super, "Super block handle invalid");
		return super->Destroy();
	}

	inline static MemoryBlock* FindNextBlock(MemoryBlock* block)
	{
		return reinterpret_cast<MemoryBlock*>(uintptr_t(block) + sizeof(MemoryBlock) + block->Size);
	}
	static MemoryBlock* GetMemoryBlockFromAllocationPointer(void* allocation)
	{
		MemoryBlock* memBlock = (MemoryBlock*)allocation - 1;
		TCORE_SOFT_CHECK(memBlock->IsValid(), "Invalid memory allocation");
		TCORE_SOFT_CHECK(memBlock->ParentSuperBlock->Strategy == TCORE_SUPERBLOCKALLOCATIONSTRATEGY_STANDARD,
						 "Allocator mismatch");
		return memBlock;
	}
	static void* Malloc(TCSuperBlock super_block, TU8 size, const char* name)
	{
		const TU8 requiredAllocSize = size + sizeof(MemoryBlock);
		auto superBlock = SuperBlock::GetFromHnd(super_block);

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
			activeMemBlock = FindNextBlock(activeMemBlock);
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
		TU4 nameLen = std::min(TU8(std::strlen(name)), kMaxBlockName);
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
		if (!memBlock)
			return;
		// TCVirtualMemory->Decommit(allocation, memBlock->Size);
	}
};

struct Vector
{
	TU4 ElementSize = 0;
	TU8 Count = 0;
	TU8 Capacity = 0;
	TU8 AllocationSize = 0;
	SuperBlock* ParentSuperBlock = nullptr;
	void* Data() { return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(this) + sizeof(Vector)); }
};

struct VectorServices
{
#define GetVector(data) reinterpret_cast<Vector*>(reinterpret_cast<uintptr_t>(data) - sizeof(Vector))
	static void DestroyElements(void* data, TU8 destroyCount)
	{
		Vector* hnd = GetVector(data);
		if (!hnd || destroyCount == 0)
			return;

		const TU8 byteCount = hnd->ElementSize * destroyCount;
		memset(reinterpret_cast<unsigned char*>(data) + (hnd->ElementSize * (hnd->Count - destroyCount)), 0, byteCount);
	}
	static void AddElements(void* data, TU8 elementCount)
	{
		Vector* hnd = GetVector(data);
		if (!hnd || elementCount == 0)
			return;

		uintptr_t d = reinterpret_cast<uintptr_t>(data);
		TCVirtualMemory->Commit(reinterpret_cast<void*>(d + (hnd->ElementSize * hnd->Count)),
								elementCount * hnd->ElementSize);
		hnd->Count += elementCount;
	}
	static TCVector Create(TCSuperBlock super_block, TU4 element_size, TU8 initial_size, TU8 max_size)
	{
		auto superBlock = SuperBlock::GetFromHnd(super_block);
		if (!superBlock)
			return nullptr;

		const TU8 capacity = std::max<TU8>(initial_size, max_size ? max_size : 1);
		const TU8 reserveSize = std::max<TU8>(sizeof(Vector) + (element_size * capacity), PagesToBytes(1));
		Vector* vector =
			reinterpret_cast<Vector*>(GetMallocFunc(superBlock->Strategy)(superBlock->GetHnd(), reserveSize, "Vector"));
		if (!vector)
			return nullptr;

		TCVirtualMemory->Commit(vector, reserveSize);
		vector->Count = initial_size;
		vector->ElementSize = element_size;
		vector->Capacity = capacity;
		vector->AllocationSize = reserveSize;
		vector->ParentSuperBlock = superBlock;
		return reinterpret_cast<TCVector>(vector->Data());
	}
	static TU8 Size(const TCVector data)
	{
		Vector* hnd = GetVector(data);
		return hnd ? hnd->Count : 0;
	}
	static TU8 Capacity(const TCVector data)
	{
		Vector* hnd = GetVector(data);
		return hnd ? hnd->Capacity : 0;
	}
	static TCResult PushBack(TCVector data, const void* src)
	{
		Vector* hnd = GetVector(data);
		if (!hnd)
			return {TC_RESULTSTATE_INVALID_ARGUMENT, 0};
		if (hnd->Count >= hnd->Capacity)
			return {TC_RESULTSTATE_OUT_OF_MEMORY, 0};

		void* dstElement = reinterpret_cast<unsigned char*>(hnd->Data()) + (hnd->ElementSize * hnd->Count);
		TCVirtualMemory->Commit(dstElement, hnd->ElementSize);
		if (src)
			memcpy(dstElement, src, hnd->ElementSize);
		hnd->Count++;
		return {TC_RESULTSTATE_SUCCESS, 0};
	}
	static TCResult Erase(TCVector data, TU8 elementIndex)
	{
		Vector* hnd = GetVector(data);
		if (!hnd || elementIndex >= hnd->Count)
			return {TC_RESULTSTATE_INVALID_ARGUMENT, 0};

		memmove(reinterpret_cast<unsigned char*>(hnd->Data()) + (hnd->ElementSize * elementIndex),
				reinterpret_cast<unsigned char*>(hnd->Data()) + (hnd->ElementSize * (elementIndex + 1)),
				hnd->ElementSize * (hnd->Count - elementIndex - 1));

		hnd->Count--;
		return {TC_RESULTSTATE_SUCCESS, 0};
	}
	static void Destroy(TCVector v)
	{
		Vector* hnd = GetVector(v);
		if (!hnd)
			return;
		DestroyElements(hnd->Data(), hnd->Count);
	}
	static TCSuperBlock GetSuperBlock(TCVector v)
	{
		Vector* hnd = GetVector(v);
		if (!hnd)
			return nullptr;
		return hnd->ParentSuperBlock->GetHnd();
	}
};

void BindVectorAndAllocatorServices(ITCAllocator* services)
{
	// Vector Manager
	{
		ITCVector* f_vector = (ITCVector*)malloc(sizeof(ITCVector));
		f_vector->Create = VectorServices::Create;
		f_vector->Erase = VectorServices::Erase;
		f_vector->Capacity = VectorServices::Capacity;
		f_vector->Size = VectorServices::Size;
		f_vector->PushBack = VectorServices::PushBack;
		f_vector->Destroy = VectorServices::Destroy;
		f_vector->GetSuperBlock = VectorServices::GetSuperBlock;
		services->VectorManager = f_vector;
	}

	services->Malloc = [](TCSuperBlock memory_block, TU8 size, const char* name) -> void* {
		auto superBlock = SuperBlock::GetFromHnd(memory_block);
		return GetMallocFunc(superBlock->Strategy)(memory_block, size, name);
	};
	services->Free = [](void* allocationPtr) {
		auto superBlock = GContext->FindCollidingSuperBlock(allocationPtr);
		if (superBlock)
			return GetFreeFunc(superBlock->Strategy)(allocationPtr);
		printf("Failed to free %p. It is not allocated with TCAllocator\n", allocationPtr);
	};
	services->CreateSuperBlock =
		[](TU8 block_size, const char* name, TCSuperBlockAllocationStrategy strategy) -> TCSuperBlock {
		return SuperBlock::Create<true>(block_size, name, strategy);
	};
	services->DestroySuperBlock = [](TCSuperBlock super_memory_block) {
		SuperBlock::GetFromHnd(super_memory_block)->Destroy();
	};
}

MallocFunc GetMallocFunc(TCSuperBlockAllocationStrategy strategy)
{
	switch (strategy)
	{
	case TCORE_SUPERBLOCKALLOCATIONSTRATEGY_EOP: return EndOfPageAllocatorServices::Malloc;
	case TCORE_SUPERBLOCKALLOCATIONSTRATEGY_STANDARD: return StandardAllocatorServices ::Malloc;
	}
	return nullptr;
}
FreeFunc GetFreeFunc(TCSuperBlockAllocationStrategy strategy)
{
	switch (strategy)
	{
	case TCORE_SUPERBLOCKALLOCATIONSTRATEGY_EOP: return EndOfPageAllocatorServices::Free;
	case TCORE_SUPERBLOCKALLOCATIONSTRATEGY_STANDARD: return StandardAllocatorServices ::Free;
	}
	return nullptr;
}

struct TCAllocatorUnitTests
{
	static TCResult StandardAllocatorTest(TCReadBuffer inputData);
	static TCResult EndOfPageAllocatorTest(TCReadBuffer inputData);
	static TCResult VectorManagerTest(TCReadBuffer inputData);
	static TCResult PairedListTest(TCReadBuffer inputData);
	static void Register();
};

TCResult TCAllocatorUnitTests::StandardAllocatorTest(TCReadBuffer inputData)
{
	(void)inputData;
	if (!TCAllocator || !TCAllocator || !TCAllocator->CreateSuperBlock || !TCAllocator->Malloc || !TCAllocator->Free ||
		!TCAllocator->DestroySuperBlock)
		return {TC_RESULTSTATE_FAILURE, 0};

	TCSuperBlock superBlock =
		TCAllocator->CreateSuperBlock(4096, "TCAllocatorStdTest", TCORE_SUPERBLOCKALLOCATIONSTRATEGY_STANDARD);
	if (!superBlock)
		return {TC_RESULTSTATE_FAILURE, 0};
	void* allocation = TCAllocator->Malloc(superBlock, 128, "TCAllocatorStdTest");
	if (!allocation)
	{
		TCAllocator->DestroySuperBlock(superBlock);
		return {TC_RESULTSTATE_FAILURE, 0};
	}

	std::memset(allocation, 0x5A, 128);
	TCAllocator->Free(allocation);
	TCAllocator->DestroySuperBlock(superBlock);
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCAllocatorUnitTests::EndOfPageAllocatorTest(TCReadBuffer inputData)
{
	(void)inputData;
	if (!TCAllocator || !TCAllocator || !TCAllocator->CreateSuperBlock || !TCAllocator->Malloc || !TCAllocator->Free ||
		!TCAllocator->DestroySuperBlock)
		return {TC_RESULTSTATE_FAILURE, 0};

	TCSuperBlock superBlock =
		TCAllocator->CreateSuperBlock(1ull << 20ull, "TCAllocatorEopTest", TCORE_SUPERBLOCKALLOCATIONSTRATEGY_EOP);
	if (!superBlock)
		return {TC_RESULTSTATE_FAILURE, 0};
	void* allocation = TCAllocator->Malloc(superBlock, 64, "TCAllocatorEopTest");
	if (!allocation)
	{
		TCAllocator->DestroySuperBlock(superBlock);
		return {TC_RESULTSTATE_FAILURE, 0};
	}

	std::memset(allocation, 0x7C, 64);
	TCAllocator->Free(allocation);
	TCAllocator->DestroySuperBlock(superBlock);
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCAllocatorUnitTests::VectorManagerTest(TCReadBuffer inputData)
{
	(void)inputData;
	if (!TCAllocator || !TCVectorManager || !TCVectorManager->Create || !TCVectorManager->PushBack ||
		!TCVectorManager->Size || !TCVectorManager->Erase || !TCVectorManager->Destroy ||
		!TCVectorManager->GetSuperBlock)
		return {TC_RESULTSTATE_FAILURE, 0};

	TCSuperBlock superBlock =
		TCAllocator->CreateSuperBlock(8192, "TCAllocatorVectorTest", TCORE_SUPERBLOCKALLOCATIONSTRATEGY_STANDARD);
	if (!superBlock)
		return {TC_RESULTSTATE_FAILURE, 0};
	TCVector vector = TCVectorManager->Create(superBlock, sizeof(int), 0, 4);
	if (!vector)
	{
		TCAllocator->DestroySuperBlock(superBlock);
		return {TC_RESULTSTATE_FAILURE, 0};
	}

	int values[3] = {10, 20, 30};
	for (int i = 0; i < 3; ++i)
	{
		if (TCVectorManager->PushBack(vector, &values[i]) != TC_RESULTSTATE_SUCCESS)
		{
			TCVectorManager->Destroy(vector);
			TCAllocator->DestroySuperBlock(superBlock);
			return {TC_RESULTSTATE_FAILURE, 0};
		}
	}

	if (TCVectorManager->Size(vector) != 3)
	{
		TCVectorManager->Destroy(vector);
		TCAllocator->DestroySuperBlock(superBlock);
		return {TC_RESULTSTATE_FAILURE, 0};
	}

	if (TCVectorManager->Erase(vector, 1) != TC_RESULTSTATE_SUCCESS)
	{
		TCVectorManager->Destroy(vector);
		TCAllocator->DestroySuperBlock(superBlock);
		return {TC_RESULTSTATE_FAILURE, 0};
	}

	if (TCVectorManager->Size(vector) != 2)
	{
		TCVectorManager->Destroy(vector);
		TCAllocator->DestroySuperBlock(superBlock);
		return {TC_RESULTSTATE_FAILURE, 0};
	}

	int* data = reinterpret_cast<int*>(vector);
	if (data[0] != 10 || data[1] != 30)
	{
		TCVectorManager->Destroy(vector);
		TCAllocator->DestroySuperBlock(superBlock);
		return {TC_RESULTSTATE_FAILURE, 0};
	}

	TCVectorManager->Destroy(vector);
	TCAllocator->DestroySuperBlock(superBlock);
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCORE_DEFINE_HANDLE(TGfxHnd);
TCResult TCAllocatorUnitTests::PairedListTest(TCReadBuffer inputData)
{
	(void)inputData;
	if (!TCAllocator || !TCVectorManager || !TCVectorManager->Create || !TCVectorManager->PushBack ||
		!TCVectorManager->Size || !TCVectorManager->Erase || !TCVectorManager->Destroy)
		return {TC_RESULTSTATE_FAILURE, 0};

	TCSuperBlock superBlock =
		TCAllocator->CreateSuperBlock(8192, "TCAllocatorPairedListTest", TCORE_SUPERBLOCKALLOCATIONSTRATEGY_EOP);
	if (!superBlock)
		return {TC_RESULTSTATE_FAILURE, 0};

	PairedList<TU8, TGfxHnd> list;
	list.Insert(7, (TGfxHnd)superBlock);

	TCAllocator->DestroySuperBlock(superBlock);
	return {TC_RESULTSTATE_SUCCESS, 0};
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

	desc.Name = "TCAllocator_UnitTest_PairedList";
	desc.Test = PairedListTest;
	TCUnitTest->RegisterTest(&desc);
}

} // namespace Allocator
} // namespace TCore

TCResult TCAllocator_Initialize(const void** outPluginAPI)
{
	TCore::Allocator::TCAllocatorContext::Initialize();
	if (TC->GetPlugin(TCUnitTest_PLUGIN_NAME, TCUnitTest_PLUGIN_VERSION, nullptr, (const void**)&TCUnitTest) ==
		TC_RESULTSTATE_SUCCESS)
		TCore::Allocator::TCAllocatorUnitTests::Register();
	*outPluginAPI = TCAllocator;
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCAllocator_OnPreShutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCAllocator_Shutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

void TCAllocator_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded) {}
