#pragma once
#define TCORE_USE_CPP_WRAPPER
#include <TGfxCore.h>
#include <Allocator.h>
#include <Logger.h>

#ifndef NDEBUG
#define VK_VALIDATION_LAYER
#endif

#include <volk.h>

// <-------------------------------------------------------------------------------------->
//		GLOBALS
// <-------------------------------------------------------------------------------------->

extern VkInstance GVkInstance;
extern VkApplicationInfo GVkAppInfo;

// <------------------------------------------------------------------------------------>
//		CONSTANTS
// <------------------------------------------------------------------------------------>

#define VkConstU4 static constexpr uint32_t
#define VkConstF4 static constexpr float
#define VkConstHndType static constexpr VKHANDLETYPEs
#define VkConstU1 static constexpr uint8_t
// User can pass lots of descriptor sets in a list, this defines the max size
VkConstU4 kMaxDescSetPerList = 12;
VkConstU4 kMaxViewportCount = 16;
VkConstU4 kMaxGpuCount = 4;
VkConstU4 kMaxRtSlotCount = 16;
VkConstU4 kMaxQueueFamilyCountPerGpu = 5;
VkConstU4 kMaxSemaphoreCountPerSubmit = 16;
VkConstU4 kMaxSwapchainCountPerSubmit = 8; // Max count of swapchain count per submit
VkConstU4 kMaxSwapchainTextureCountPerSwapchain = 4;

// <-------------------------------------------------------------------------------------->
//		HELPERS
// <-------------------------------------------------------------------------------------->

TCResult vkPrint(TU4 returnCode, const char* extraDetails = nullptr)
{
	const char* message{};
	TGfx->GetResultStateByReturnCode(returnCode, &message);
	if (extraDetails)
		TCore::tl(TC_LOG_LEVEL_STATUS) << message << TCore::td() << extraDetails;
	else
		TCore::tl(TC_LOG_LEVEL_STATUS) << message;
}

void Append_pNext(void* targetStruct, void* attachStruct);

#define VK_PRIM_MIN(primType) std::numeric_limits<primType>::min()
#define VK_PRIM_MAX(primType) std::numeric_limits<primType>::max()

// <-------------------------------------------------------------------------------------->
//      C++ Generics
// <-------------------------------------------------------------------------------------->

// There can be only 65536 types of handles that users can access (which is enough)
enum class VKHANDLETYPEs : unsigned short
{
	UNDEFINED = 0,
	INTERNAL, // You can use INTERNAL for objects that won't be returned to user as handle but you
			  // want the vkobjectstructure for the data backend uses
	SAMPLER,
	BINDINGTABLEINST, // Returned as binding table
	VERTEXATTRIB,
	SHADERSOURCE,
	BUFFER,
	RTSLOTSET,
	IRTSLOTSET,
	TEXTURE,
	GPU,
	VIEWPORT,
	HEAP,
	GPUQUEUE,
	FENCE,
	CMDBUFFER,
	CMDBUNDLE,
	PIPELINE,
	SUBRASTERPASS,
	SWAPCHAIN
};

struct VKOBJHANDLE
{
	VKHANDLETYPEs Type : 16;
	uint16_t EXTRA_FLAGs : 16;	 // This is generally for padding (because I want this struct to match a
								 // 8byte pointer's size)
	uint32_t OBJ_memoffset : 32; // It's offset of the object from the start of the memory allocation
};
static_assert(sizeof(VKOBJHANDLE) == sizeof(TGfxGpu),
			  "Vulkan backend's opaque handle structure size mismatches TGfx's opaque handle size");

#include <vector>
template <typename T, typename TGFXHND, unsigned int max_object_count = 1 << 20>
class VK_LINEAR_OBJARRAY
{
	static_assert(T::HANDLETYPE != VKHANDLETYPEs::UNDEFINED, "VKOBJ's type shouldn't be UNDEFINED");
	const unsigned int elementCountPerPage() { return VKCONST_VIRMEMPAGESIZE / sizeof(T); }
	std::vector<T*> data;

public:
	VK_LINEAR_OBJARRAY() {}
	T* create_OBJ()
	{
		T* o = new T;
		data.push_back(o);
		return o;
	}
	void destroyObj(unsigned int objIndx)
	{
		T* o = data[objIndx];
		delete o;
		data.erase(data.begin() + objIndx);
	}
	bool isValid(T* obj)
	{
		for (T* o : data)
		{
			if (obj == o)
			{
				return true;
			}
		}
		return false;
	}
	unsigned int size() const { return data.size(); }
	T* getOBJbyINDEX(unsigned int i) { return (isValid(data[i])) ? (data[i]) : (NULL); }
	T* operator[](unsigned int index) { return getOBJbyINDEX(index); }
	uint32_t getINDEXbyOBJ(T* obj)
	{
		for (uint32_t i = 0; i < data.size(); i++)
		{
			if (data[i] == obj)
			{
				return i;
			}
		}
		return UINT32_MAX;
	}
};

template <typename T, typename TGFXHND>
class VK_ARRAY
{
	std::vector<T> data;

public:
	bool isValid(T* obj) const
	{
		for (uint32_t i = 0; i < data.size(); i++)
		{
			if (&data[i] == obj)
			{
				return true;
			}
		}
		return false;
	}
	void init(uint32_t size) { data.resize(size); }
	void init(T* i_data, uint32_t size) { data = std::vector<T>(i_data, i_data + size); }
	T* operator[](uint32_t i) { return &data[i]; }
	const T* operator[](uint32_t i) const { return &data[i]; }
	uint32_t size() const { return data.size(); }
};

//--------------------------------------------------------------------------------------
//	VKOBJECT TUTORIAL:
// VKOBJECTs have 2 states; INVALID (destroyed or not created) and VALID (created and not deleted
// yet) If isDELETED = true, VK_LINEAR_OBJARRAY places previous and next free element's indexes as
// next data in the structure If you create an object from LINEAR_OBJARRAY, you should change every
// variable of the structure
//	because the LINEAR_OBJARRAY may corrupt your data because of deletion process
//--------------------------------------------------------------------------------------
// This is an example of a vulkan object structure that a LINEAR_OBJARRAY can create handles from
// So VKOBJECT structure should have following funcs: assignment operator, static GET_EXTRAFLAGS(),
// deleted new() operator
// and should have following variables: VkConstHndType HANDLETYPE, std::atomic_bool
// isDELETED (should be first variable)

uint32_t GetMemOffset(void* object) {}

void* GetPointer(uint32_t memOffset) {}

// Use this right after VkObject class definition to automatically register the object to GetVkObject() and
// GetTGfxHandle() overloads
#define VK_DEFINE_TYPE_CONVERTERS(VkObjectTypeName)                                                                    \
	typedef decltype(VkObjectTypeName## ::GetTGfxOpaqueHandle(nullptr)) VkObjectTypeName##_TGfxHndType;                \
	VkObjectTypeName* GetVkObject(VkObjectTypeName##_TGfxHndType opaqueHandle)                                         \
	{                                                                                                                  \
		return VkObjectTypeName::GetVkObject(opaqueHandle);                                                            \
	}                                                                                                                  \
	VkObjectTypeName##_TGfxHndType GetTGfxHandle(VkObjectTypeName* object)                                             \
	{                                                                                                                  \
		return VkObjectTypeName::GetTGfxOpaqueHandle(object);                                                          \
	}

template <typename VkObject, typename TGfxHndType>
struct VkObjectBase
{
	bool IsAlive = true;
	static TGfxHndType GetTGfxOpaqueHandle(VkObject* object)
	{
		VKOBJHANDLE handle;
		handle.EXTRA_FLAGs = object->GetExtraFlags();
		handle.OBJ_memoffset = GetMemOffset(object);
		handle.Type = VkObject::HANDLETYPE;
		return *(TGfxHndType*)&handle;
	}

	static VkObject* GetVkObject(TGfxHndType opaqueHandle)
	{
		auto handle = *(VKOBJHANDLE*)&opaqueHandle;

		if (VkObject::HANDLETYPE != handle.Type)
			return nullptr;

		VkObject* object = (VkObject*)GetPointer(handle.OBJ_memoffset);
		if (object->GetExtraFlags() != handle.EXTRA_FLAGs)
			return nullptr;

		// Implement conversion from opaqueHandle to TObject* here.
		// Example placeholder (must be replaced with your real lookup):
		return object;
	}
};

// VkObject Example

struct VkObjectExample : public VkObjectBase<VkObjectExample, TGfxGpu>
{
	VkConstHndType HANDLETYPE = VKHANDLETYPEs::UNDEFINED;
	uint16_t GetExtraFlags() { return 0; }

	uint64_t normal_objdata;
};
VK_DEFINE_TYPE_CONVERTERS(VkObjectExample) // <- This allows us to directly use GetVkObject(object) without
										   // writing any VkObject type name