// External
#include <algorithm>
#include <cstring>
#include <string>
#include "stdint.h"
#include "stdio.h"

// TCore
#define T_INCLUDE_PLATFORM_LIBS
#include "ECS.h"
#include "Allocator.h"

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCECS)
TCORE_PLUGIN_INIT(TCAllocator)

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCECS)
TCORE_PLUGIN_ENTRY_POINT_END()

namespace TCore
{
namespace ECS
{
#pragma region Constants
static constexpr TUint kMaxSystemCount = 1 << 20;
static constexpr TUint kMaxEntityTypeCount = 1 << 24;
static constexpr TUint kMaxEntityCount = 1 << 24;
static constexpr TUint kMaxCompTypeCount = 1 << 24;
static constexpr TUint kMaxCompTypePairCount = 1 << 26;
static constexpr TUint kMaxSystemChar = 32;				 // You should use null terminator too, so max 31 chars
static constexpr TUint kMaxCompTypeChar = 80;			 // You should use null terminator too, so max 79 chars
static constexpr TSize kVirmemAllocSize = 1ull << 36ull; // Allocation size for plugin info and dlls

#pragma region ECS INTERNAL STRUCTURES

struct SystemInfo
{
	char Name[kMaxSystemChar];
	TUint Version = UINT32_MAX;
	const void* Ptr = nullptr;
};

// This is main component type, so you shouldn't store (overriden) parent types with this
// Manager will return the (overriden) parent types with getComponentType anyway
struct ComponentType
{
	uint32_t TypeId = UINT32_MAX;
	char Name[kMaxCompTypeChar];
	void* MainTypeHandle;
	TCComponentManagerDescription Manager;
	unsigned int OverridenTypeCount = 0;
	TCComponentTypePair* OverridenTypePairs;
};

struct Entity
{
	TUint TypeId;
	TUint EntityId;
};

struct EntityType
{
	TUint TypeId = UINT32_MAX;
	TCore::Vector<TCComponentTypeHandle> ComponentTypes;
	TCore::Vector<Entity> Entities;
};

struct Context* GContext = nullptr;
struct Context
{
	static TCResult Initialize()
	{
		auto mainMemBlock = TCAllocator->AllocateSuperMemoryBlock(TCore::ECS::kVirmemAllocSize, TCECS_PLUGIN_NAME);

		// Initialize ECS
		GContext = (Context*)TCStdAllocator->Malloc(mainMemBlock, sizeof(Context), TCECS_PLUGIN_NAME);

		// Fill plugin API
		auto services = (ITCECS*)TCStdAllocator->Malloc(mainMemBlock, sizeof(ITCECS), TCECS_PLUGIN_NAME);
		services->GetSystem = GetSystem;
		services->RegisterSystem = RegisterSystem;
		services->DestroySystem = UnregisterSystem;
		services->AddComponentType = AddComponentType;
		services->AddEntityType = AddEntityType;
		services->CreateEntity = CreateEntity;
		services->FindEntityType = FindEntityType;
		services->SearchComponentType = SearchComponentType;
		services->GetComponent = GetComponent;
		TCECS = services;
	}

	// Systems
	//////////////////////
	TCore::Vector<SystemInfo> SystemInfos;

	// Components & Types
	////////////////////////

	// These types are main types
	// Only childs are stored, overriden parents aren't
	TCore::Vector<ComponentType>* MainComponentTypes;
	ComponentType* find_compType_byID(TCComponentTypeHandle id)
	{
		ecstapi_idOnlyPointer hndle = *reinterpret_cast<ecstapi_idOnlyPointer*>(&id);
		ComponentType* r = &MainComponentTypes[hndle.typeID];
#ifdef TURAN_DEBUGGING
		if (r->TypeId != hndle.typeID)
		{
			r = nullptr;
		}
#endif
		return r;
	}

	// Entities & Types
	// You can find an entity's type with its ID
	// Entity types store each component type as a pair of base-overriden component type pointers
	// This is the best for the performance and increases only a little bit of memory usage (otherwise
	// a recursive memory traversal is needed to find overriden component types)
	///////////////////////

	TCore::Vector<EntityType> EntityTypes;
	EntityType* FindEntityType(TCEntityTypeHandle hnd)
	{
		ecstapi_idOnlyPointer hndle = *reinterpret_cast<ecstapi_idOnlyPointer*>(&hnd);
		EntityType* r = &EntityTypes[hndle.typeID];
#ifdef TURAN_DEBUGGING
		if (r->TypeId != hndle.typeID)
		{
			r = nullptr;
		}
#endif
		return r;
	}

	// INTERNAL SYSTEM

	uint32_t FindOverridenComponentType(EntityType* eType, ComponentType* base, void** overriden)
	{
		for (uint32_t mainCompIndex = 0; mainCompIndex < eType->compCount; mainCompIndex++)
		{
			// If main type matches
			if (base == eType->compTypeHndlesList[mainCompIndex])
			{
				*overriden = GContext->find_compType_byID(base)->mainTypeHandle;
				return mainCompIndex;
			}
			ecs_compType* mainCompType = GContext->find_compType_byID(eType->compTypeHndlesList[mainCompIndex]);
			for (uint32_t pairIndx = 0; pairIndx < mainCompType->overridenTypeCount; pairIndx++)
			{
				tlComponentTypePair& pair = mainCompType->overridenTypePairs[pairIndx];
				if (pair.base == base)
				{
					*overriden = pair.overriden;
					return mainCompIndex;
				}
			}
		}
		return UINT32_MAX;
	}

#pragma region System Functions

	static TCResult RegisterSystem(const char* pluginName,
								   const char* name,
								   unsigned int version,
								   const void* system_ptr)
	{
		// Checks & Validation
		auto nameLen = std::strlen(name);
		{
			if (nameLen >= kMaxSystemChar)
			{
				printf("System name is too long, it should be less than %u characters!\n", kMaxSystemChar);
				return TC_RESULT_INVALID_ARGUMENT;
			}
			if (GetSystem(name))
			{
				printf("A system with the same name is already registered, so this one failed!\n");
				return TC_RESULT_ALREADY_EXISTS;
			}
		}

		// Add to the list
		SystemInfo sysInfo;
		unsigned int maxcharlen = std::min<unsigned int>(nameLen, kMaxSystemChar - 1);
		std::memcpy(sysInfo.Name, name, maxcharlen);
		sysInfo.Name[maxcharlen] = 0;
		sysInfo.Ptr = system_ptr;
		sysInfo.Version = version;
		for (uint32_t i = 0; i < kMaxSystemCount; i++)
		{
			SystemInfo& currentSys = GContext->SystemInfos[i];
			if (currentSys.Ptr == nullptr)
			{
				currentSys = sysInfo;
				return TC_RESULT_SUCCESS;
			}
		}
		return TC_RESULT_OUT_OF_MEMORY; // Max system count exceeded, this is very unlikely
	}

	static void UnregisterSystem(const void* systemPtr) { printf("Unregistering a system isn't supported yet!\n"); }

	static const void* GetSystem(const char* name)
	{
		for (uint32_t i = 0; i < kMaxSystemCount; i++)
		{
			if (GContext->SystemInfos[i].Ptr == nullptr)
				continue;
			if (!std::strcmp(GContext->SystemInfos[i].Name, name))
				return GContext->SystemInfos[i].Ptr;
		}
		return nullptr;
	}

#pragma region Component Functions

	static TCComponentTypeHandle AddComponentType(const char* name,
												  void* main_type,
												  const TCComponentManagerDescription* manager,
												  const TCComponentTypePair* pair_list,
												  unsigned int pair_list_size)
	{
		f_vector->pushBack(GContext->v_mainComponentTypes, GContext->v_mainComponentTypes);
		uint32_t indx = f_vector->size(GContext->v_mainComponentTypes) - 1;
		ecs_compType* type = &GContext->v_mainComponentTypes[indx];
		type->mainTypeHandle = mainType;
		type->manager = manager;
		type->typeID = indx;
		type->overridenTypeCount = pairListSize;
		type->overridenTypePairs =
			(tlComponentTypePair*)standard_alloc->malloc(mainMemBlock, sizeof(tlComponentTypePair) * pairListSize);
		std::memcpy(type->overridenTypePairs, pairList, sizeof(tlComponentTypePair) * pairListSize);
		uint32_t namelen = std::strlen(name) + 1;
		uint32_t copylen = std::min<uint32_t>(namelen + 1, MAX_COMPTYPECHAR - 1);
		std::memcpy(type->name, name, copylen);
		for (uint32_t i = copylen; i < MAX_COMPTYPECHAR; i++)
		{
			type->name[i] = '\0';
		}
		ecstapi_idOnlyPointer idOnlyPointer;
		idOnlyPointer.typeID = type->typeID;
		idOnlyPointer.padding_to_8byte = UINT32_MAX;
		return *reinterpret_cast<ComponentType**>(&idOnlyPointer);
	}

#pragma region Entity Functions

	static TCEntityTypeHandle AddEntityType(const TCComponentTypeHandle* comp_type_list, TSize list_size)
	{
		ecs_entityType type;
		type.compCount = list_size;
		type.typeID = f_vector->size(GContext->v_entityTypes);
		type.v_entityList = f_vector->create(mainMemBlock, list_size * sizeof(TCComponentHandle), 0, 1 << 20, 0);
		type.compTypeHndlesList =
			(ComponentType**)standard_alloc->malloc(mainMemBlock, sizeof(ComponentType) * list_size);
		std::memcpy(type.compTypeHndlesList, compTypeList, sizeof(ComponentType) * list_size);

		f_vector->pushBack(GContext->v_entityTypes, &type);
		ecstapi_idOnlyPointer hnd;
		hnd.padding_to_8byte = UINT32_MAX;
		hnd.typeID = type.typeID;
		return *reinterpret_cast<TCEntityTypeHandle*>(&hnd);
	}
	static TCEntityHandle CreateEntity(TCEntityTypeHandle typeHandle)
	{
		ecs_entityType* eType = GContext->FindEntityType(typeHandle);
		f_vector->pushBack(eType->v_entityList, nullptr);
		uint32_t index = f_vector->size(eType->v_entityList) - 1;
		TCComponentHandle* compHndsList = reinterpret_cast<TCComponentHandle*>(
			reinterpret_cast<uintptr_t>(eType->v_entityList) + (index * eType->compCount * sizeof(TCComponentHandle)));
		for (uint16_t compIndx = 0; compIndx < eType->compCount; compIndx++)
		{
			ecs_compType* compType = GContext->find_compType_byID(eType->compTypeHndlesList[compIndx]);
			compHndsList[compIndx] = compType->manager.createComponent();
		}
		Entity fin_hnd;
		fin_hnd.entityID = index;
		fin_hnd.typeID = eType->typeID;
		return *reinterpret_cast<TCEntityHandle*>(&fin_hnd);
	}
	static TCEntityTypeHandle FindEntityType(TCEntityHandle entityHnd)
	{
		ecstapi_idOnlyPointer hnd;
		Entity entity = *reinterpret_cast<TCEntityHandle>(&entityHnd);
#ifdef TURAN_DEBUGGING
		// If debugging, first access type
		// Then return ID of it
		// With this way, wrong accesses minimized
		ecs_entityType* type = &GContext->v_entityTypes[entity.typeID];
		hnd.typeID = type->typeID;
#else
		// If not debugging, just get type ID from entity handle
		hnd.typeID = entity.TypeId;
#endif
		hnd.padding_to_8byte = UINT32_MAX;
		return *reinterpret_cast<TCEntityTypeHandle*>(&hnd);
	}
	static TCResult SearchComponentType(TCEntityTypeHandle entity_type, TCComponentTypeHandle component_type)
	{
		EntityType* eType = GContext->FindEntityType(entity_type);
		void* empty = nullptr;
		uint32_t compTypeIndx = FindOverridenComponentType(eType, component_type, &empty);
		if (compTypeIndx != UINT32_MAX)
		{
			return 1;
		}
		return 0;
	}
	static TCComponentHandle GetComponent(TCEntityHandle entityHnd,
										  TCComponentTypeHandle component_type,
										  void** outComponentType)
	{
		TCEntityTypeHandle eTypeHnd = FindEntityType(entityHnd);
		EntityType* eType = GContext->FindEntityType(eTypeHnd);
		// Find overriden component type
		uint32_t compTypeIndx = FindOverridenComponentType(eType, component_type, outComponentType);
		uintptr_t eList = reinterpret_cast<uintptr_t>(eType->v_entityList);
		Entity entity = *reinterpret_cast<TCEntityHandle>(&entityHnd);
		return *reinterpret_cast<TCComponentHandle*>(
			eList + (((entity.entityID * eType->compCount) + compTypeIndx) * sizeof(TCComponentHandle)));
	}
};

} // namespace ECS
} // namespace TCore

#pragma region Plugin Entry Points
// This is the entry point of the engine
TCResult TCECS_Initialize(const void** outECSAPI)
{
	auto res = TCore::ECS::Context::Initialize();
	if (res != TC_RESULT_SUCCESS)
	{
		printf("Failed to initialize ECS context!\n");
		return TC_RESULT_FAILURE;
	}

	*outECSAPI = TCECS;
	return TC_RESULT_SUCCESS;
}

TCResult TCECS_OnPreShutdown(const TCPluginInfo* pluginInfo, bool isLoaded)
{
	return TC_RESULT_SUCCESS;
}

TCResult TCECS_Shutdown()
{
	return TC_RESULT_SUCCESS;
}

void TCECS_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, bool isLoaded) {}