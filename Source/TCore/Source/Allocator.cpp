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
TCORE_PLUGIN_HARD_DEPENDENCY(TCVirtualMemory, TCVirtualMemory_PLUGIN_VERSION)
TCORE_PLUGIN_ENTRY_POINT_END()

namespace TCore
{
namespace Allocator
{

static constexpr TSize kMaxBlockName = 230; // You should use null terminator

// These can be gathered after reload too
static TSize GPageSizeBitCount = 0;
static TSize GHandleConstantSuffix = 0;

void BindVectorAndAllocatorServices(ITCAllocator* allocator);
struct TCAllocatorContext* GContext = nullptr;
struct SuperBlock;
struct TCAllocatorContext
{
	TCore::Vector<SuperBlock*, true> SuperBlocks;

	static void Initialize();
};

static inline TSize PagesToBytes(TSize pageCount)
{
	return pageCount << GPageSizeBitCount;
}

struct SuperBlock
{
#ifndef NDEBUG
	// Next page pointer
	// For validation
	void* Ptr = nullptr;
#endif

	char Name[kMaxBlockName + 1];
	bool IsFreed = true;
	TSize PageCount;
	const ITCBuffer* Allocator = nullptr;

	// Validate if this super block is valid
	static bool IsValid(TCSuperBlockHnd hnd)
	{
		TSize handleSuffix = uintptr_t(hnd) << (64 - GPageSizeBitCount);
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

	TCSuperBlockHnd GetHnd()
	{
		TCORE_SOFT_CHECK(IsValid(), "Invalid super block");
		TSize address = uintptr_t(this);
		address += GHandleConstantSuffix;
		return (TCSuperBlockHnd)address;
	}

	static SuperBlock* GetFromHnd(TCSuperBlockHnd hnd)
	{
		TCORE_SOFT_CHECK(IsValid(hnd), "Invalid super block handle");
		auto block = reinterpret_cast<SuperBlock*>(uintptr_t(hnd) - GHandleConstantSuffix);
		return block;
	}

	template <bool UseContext>
	static TCSuperBlockHnd Create(TSize block_size, const char* name, const ITCBuffer* allocator)
	{
		// Search for a freed super memory block
		uint32_t namelen = std::min(strlen(name), kMaxBlockName);
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
				super->Allocator = allocator;
				return (TCSuperBlockHnd)super;
			}
		}

		// There is no available super memory block, so allocate new one
		TSize roundedSize = block_size + (PagesToBytes(1) - (block_size % PagesToBytes(1)));
		TSize blockSize = roundedSize + PagesToBytes(1); // One extra page for super memory block struct
		SuperBlock* block = (SuperBlock*)TCVirtualMemory->Reserve(blockSize);
		if (!block)
		{
			printf("AllocateSuperMemoryBlock() failed because virtual memory reserve failed\n");
			return nullptr;
		}

		TCVirtualMemory->Commit(block, PagesToBytes(1));
		block->PageCount = block_size / PagesToBytes(1);
		block->Allocator = allocator;
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

void TCAllocatorContext::Initialize()
{
	ITCAllocator* services = new ITCAllocator;
	BindVectorAndAllocatorServices(services);
	TCAllocator = services;

	TSize pageSizeBytes = TCVirtualMemory->GetPageSize();
	GPageSizeBitCount = 0;
	while ((TSize(1) << GPageSizeBitCount) < pageSizeBytes)
		++GPageSizeBitCount;
	TCORE_SOFT_CHECK((TSize(1) << GPageSizeBitCount) == pageSizeBytes, "Page size is not a power of two");

	static constexpr TSize kInitialAllocatorSize = 1ull << 20ull;
	auto reservedMemory = TCVirtualMemory->Reserve(kInitialAllocatorSize);
	TCVirtualMemory->Commit(reservedMemory, PagesToBytes(1));
	TCore::GSuperMemoryBlock =
		SuperBlock::Create<false>(kInitialAllocatorSize, TCORE_ACTIVE_PLUGIN_NAME, TCStdAllocator);

	GContext = new (reservedMemory) TCAllocatorContext;
}

struct MemoryBlock
{
	TSize Size;
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
	static TCSuperBlockHnd CreateSuperBlock(TSize blockSize, const char* superMemBlockName)
	{
		return SuperBlock::Create<true>(blockSize, superMemBlockName, TCEopAllocator);
	}
	static void DestroySuperBlock(TCSuperBlockHnd super_memory_block)
	{
		auto super = SuperBlock::GetFromHnd(super_memory_block);
		TCORE_SOFT_CHECK(super, "Super block handle invalid");
		return super->Destroy();
	}

	inline static TSize CalculateFinalAllocSize(TSize requestedAllocSize)
	{
		TSize roundedUpSize = requestedAllocSize + (PagesToBytes(1) - (requestedAllocSize % PagesToBytes(1)));
		TSize totalAlloc = PagesToBytes(3) + roundedUpSize;
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
		TCORE_SOFT_CHECK(block->ParentSuperBlock->Allocator == TCEopAllocator, "Allocator mismatch");
		return block;
	}
	static void* Malloc(TCSuperBlockHnd super_block, TSize size, const char* name)
	{
		auto totalAlloc = CalculateFinalAllocSize(size);

		auto superBlock = SuperBlock::GetFromHnd(super_block);
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
			foundMemBlock->Ptr = reinterpret_cast<void*>(uintptr_t(foundMemBlock) + PagesToBytes(2));
			foundMemBlock->ParentSuperBlock = superBlock;
			foundMemBlock->Size = size;
		}
		foundMemBlock->IsFree = false;
		TUint nameLen = std::min(std::strlen(name), kMaxBlockName);
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
		TCVirtualMemory->Decommit(allocation, memBlock->Size);
		memBlock->IsFree = true;
	}
};

// Pretty standard allocator, just allocates the sizeof(MemoryBlock) + requested size
struct StandardAllocatorServices
{
	static TCSuperBlockHnd CreateSuperBlock(TSize blockSize, const char* superMemBlockName)
	{
		return SuperBlock::Create<true>(blockSize, superMemBlockName, TCStdAllocator);
	}
	static void DestroySuperBlock(TCSuperBlockHnd super_memory_block)
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
		TCORE_SOFT_CHECK(memBlock->ParentSuperBlock->Allocator == TCStdAllocator, "Allocator mismatch");
		return memBlock;
	}
	static void* Malloc(TCSuperBlockHnd super_block, TSize size, const char* name)
	{
		const TSize requiredAllocSize = size + sizeof(MemoryBlock);
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
	static TCVectorHnd Create(TCSuperBlockHnd super_block, TUint element_size, TSize initial_size, TSize max_size)
	{
		auto superBlock = SuperBlock::GetFromHnd(super_block);
		if (!superBlock)
			return nullptr;

		const TSize capacity = std::max<TSize>(initial_size, max_size ? max_size : 1);
		const TSize reserveSize = std::max<TSize>(sizeof(Vector) + (element_size * capacity), PagesToBytes(1));
		Vector* vector =
			reinterpret_cast<Vector*>(superBlock->Allocator->Malloc(superBlock->GetHnd(), reserveSize, "Vector"));
		if (!vector)
			return nullptr;

		TCVirtualMemory->Commit(vector, reserveSize);
		vector->Count = 0;
		vector->ElementSize = element_size;
		vector->Capacity = capacity;
		vector->AllocationSize = reserveSize;
		vector->ParentSuperBlock = superBlock;
		return reinterpret_cast<TCVectorHnd>(vector->Data());
	}
	static TSize Size(const TCVectorHnd data)
	{
		Vector* hnd = GetVector(data);
		return hnd ? hnd->Count : 0;
	}
	static TSize Capacity(const TCVectorHnd data)
	{
		Vector* hnd = GetVector(data);
		return hnd ? hnd->Capacity : 0;
	}
	static TCResult PushBack(TCVectorHnd data, const void* src)
	{
		Vector* hnd = GetVector(data);
		if (!hnd)
			return TC_RESULT_INVALID_ARGUMENT;
		if (hnd->Count >= hnd->Capacity)
			return TC_RESULT_OUT_OF_MEMORY;

		void* dstElement = reinterpret_cast<unsigned char*>(hnd->Data()) + (hnd->ElementSize * hnd->Count);
		TCVirtualMemory->Commit(dstElement, hnd->ElementSize);
		if (src)
			memcpy(dstElement, src, hnd->ElementSize);
		hnd->Count++;
		return TC_RESULT_SUCCESS;
	}
	static TCResult Erase(TCVectorHnd data, TSize elementIndex)
	{
		Vector* hnd = GetVector(data);
		if (!hnd || elementIndex >= hnd->Count)
			return TC_RESULT_INVALID_ARGUMENT;

		memmove(reinterpret_cast<unsigned char*>(hnd->Data()) + (hnd->ElementSize * elementIndex),
				reinterpret_cast<unsigned char*>(hnd->Data()) + (hnd->ElementSize * (elementIndex + 1)),
				hnd->ElementSize * (hnd->Count - elementIndex - 1));

		hnd->Count--;
		return TC_RESULT_SUCCESS;
	}
	static void Destroy(TCVectorHnd v)
	{
		Vector* hnd = GetVector(v);
		if (!hnd)
			return;
		DestroyElements(hnd->Data(), hnd->Count);
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
	static TCResult StandardAllocatorTest(TCReadBuffer inputData);
	static TCResult EndOfPageAllocatorTest(TCReadBuffer inputData);
	static TCResult VectorManagerTest(TCReadBuffer inputData);
	static void Register();
};

TCResult TCAllocatorUnitTests::StandardAllocatorTest(TCReadBuffer inputData)
{
	(void)inputData;
	if (!TCAllocator || !TCAllocator->StandardAllocator || !TCAllocator->StandardAllocator->CreateSuperBlock ||
		!TCAllocator->StandardAllocator->Malloc || !TCAllocator->StandardAllocator->Free ||
		!TCAllocator->StandardAllocator->DestroySuperBlock)
		return TC_RESULT_FAILURE;

	TCSuperBlockHnd superBlock = TCAllocator->StandardAllocator->CreateSuperBlock(4096, "TCAllocatorStdTest");
	if (!superBlock)
		return TC_RESULT_FAILURE;
	void* allocation = TCAllocator->StandardAllocator->Malloc(superBlock, 128, "TCAllocatorStdTest");
	if (!allocation)
	{
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return TC_RESULT_FAILURE;
	}

	std::memset(allocation, 0x5A, 128);
	TCAllocator->StandardAllocator->Free(allocation);
	TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
	return TC_RESULT_SUCCESS;
}

TCResult TCAllocatorUnitTests::EndOfPageAllocatorTest(TCReadBuffer inputData)
{
	(void)inputData;
	if (!TCAllocator || !TCAllocator->EndOfPageAllocator || !TCAllocator->EndOfPageAllocator->CreateSuperBlock ||
		!TCAllocator->EndOfPageAllocator->Malloc || !TCAllocator->EndOfPageAllocator->Free ||
		!TCAllocator->EndOfPageAllocator->DestroySuperBlock)
		return TC_RESULT_FAILURE;

	TCSuperBlockHnd superBlock = TCAllocator->EndOfPageAllocator->CreateSuperBlock(1ull << 20ull, "TCAllocatorEopTest");
	if (!superBlock)
		return TC_RESULT_FAILURE;
	void* allocation = TCAllocator->EndOfPageAllocator->Malloc(superBlock, 64, "TCAllocatorEopTest");
	if (!allocation)
	{
		TCAllocator->EndOfPageAllocator->DestroySuperBlock(superBlock);
		return TC_RESULT_FAILURE;
	}

	std::memset(allocation, 0x7C, 64);
	TCAllocator->EndOfPageAllocator->Free(allocation);
	TCAllocator->EndOfPageAllocator->DestroySuperBlock(superBlock);
	return TC_RESULT_SUCCESS;
}

TCResult TCAllocatorUnitTests::VectorManagerTest(TCReadBuffer inputData)
{
	(void)inputData;
	if (!TCAllocator || !TCAllocator->VectorManager || !TCAllocator->VectorManager->Create ||
		!TCAllocator->VectorManager->PushBack || !TCAllocator->VectorManager->Size ||
		!TCAllocator->VectorManager->Erase || !TCAllocator->VectorManager->Destroy)
		return TC_RESULT_FAILURE;

	TCSuperBlockHnd superBlock = TCAllocator->StandardAllocator->CreateSuperBlock(8192, "TCAllocatorVectorTest");
	if (!superBlock)
		return TC_RESULT_FAILURE;
	TCVectorHnd vector = TCAllocator->VectorManager->Create(superBlock, sizeof(int), 0, 4);
	if (!vector)
	{
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return TC_RESULT_FAILURE;
	}

	int values[3] = {10, 20, 30};
	for (int i = 0; i < 3; ++i)
	{
		if (TCAllocator->VectorManager->PushBack(vector, &values[i]) != TC_RESULT_SUCCESS)
		{
			TCAllocator->VectorManager->Destroy(vector);
			TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
			return TC_RESULT_FAILURE;
		}
	}

	if (TCAllocator->VectorManager->Size(vector) != 3)
	{
		TCAllocator->VectorManager->Destroy(vector);
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return TC_RESULT_FAILURE;
	}

	if (TCAllocator->VectorManager->Erase(vector, 1) != TC_RESULT_SUCCESS)
	{
		TCAllocator->VectorManager->Destroy(vector);
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return TC_RESULT_FAILURE;
	}

	if (TCAllocator->VectorManager->Size(vector) != 2)
	{
		TCAllocator->VectorManager->Destroy(vector);
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return TC_RESULT_FAILURE;
	}

	int* data = reinterpret_cast<int*>(vector);
	if (data[0] != 10 || data[1] != 30)
	{
		TCAllocator->VectorManager->Destroy(vector);
		TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
		return TC_RESULT_FAILURE;
	}

	TCAllocator->VectorManager->Destroy(vector);
	TCAllocator->StandardAllocator->DestroySuperBlock(superBlock);
	return TC_RESULT_SUCCESS;
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
	if (TC->GetPlugin(TCUnitTest_PLUGIN_NAME, TCUnitTest_PLUGIN_VERSION, nullptr, (const void**)&TCUnitTest) ==
		TC_RESULT_SUCCESS)
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
