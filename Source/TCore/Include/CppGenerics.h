#include "TCoreMacros.h"

// Generics

#ifdef TCORE_USE_CPP_WRAPPER

#pragma pack(push, 1)
template <typename TcPluginHandleTypes>
struct TCHandleLayout
{
	TcPluginHandleTypes Type : 16;
	unsigned short ExtraFlags : 16; // This is generally for padding (because I want this struct to match a
									// 8byte pointer's size)
	unsigned int MemoryOffset : 32; // It's offset of the object from the start of the memory allocation
};

// Use this right after a type class definition to automatically register the object to GetOpaqueHandle() and
// GetFromOpaqueHandle() overloads
#define TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(TcObjectTypeName, SystemPrefix)                                            \
	typedef decltype(TcObjectTypeName## ::GetOpaqueHandle(nullptr)) TcObjectTypeName##_TCOpaqueHandleType;             \
	TcObjectTypeName* Get##SystemPrefix##Object(TcObjectTypeName##_TCOpaqueHandleType opaqueHandle)                    \
	{                                                                                                                  \
		return TcObjectTypeName::Get##SystemPrefix##Object(opaqueHandle);                                              \
	}                                                                                                                  \
	TcObjectTypeName##_TCOpaqueHandleType GetOpaqueHandle(TcObjectTypeName* object)                                    \
	{                                                                                                                  \
		return TcObjectTypeName::GetOpaqueHandle(object);                                                              \
	}

#define TCORE_DEFINE_OPAQUE_HANDLE_SYSTEM(SystemPrefix, OpaqueHandleTypesEnum)                                         \
	template <typename PluginObjectType, typename TCOpaqueHandleType, OpaqueHandleTypesEnum TypeEnum>                  \
	struct SystemPrefix##ObjectBase                                                                                    \
	{                                                                                                                  \
		bool IsAlive = true;                                                                                           \
		static constexpr OpaqueHandleTypesEnum HANDLETYPE = TypeEnum;                                                  \
		static TCOpaqueHandleType GetOpaqueHandle(PluginObjectType* object)                                            \
		{                                                                                                              \
			TCHandleLayout<OpaqueHandleTypesEnum> handle;                                                              \
			handle.ExtraFlags = object->GetExtraFlags();                                                               \
			handle.MemoryOffset = GetMemOffset(object);                                                                \
			handle.Type = PluginObjectType::HANDLETYPE;                                                                \
			return *(TCOpaqueHandleType*)&handle;                                                                      \
		}                                                                                                              \
                                                                                                                       \
		static PluginObjectType* Get##SystemPrefix##Object(TCOpaqueHandleType opaqueHandle)                            \
		{                                                                                                              \
			auto handle = *(TCHandleLayout<OpaqueHandleTypesEnum>*)&opaqueHandle;                                      \
                                                                                                                       \
			if (PluginObjectType::HANDLETYPE != handle.Type)                                                           \
				return nullptr;                                                                                        \
                                                                                                                       \
			PluginObjectType* object = (PluginObjectType*)GetPointer(handle.MemoryOffset);                             \
			if (object->GetExtraFlags() != handle.ExtraFlags)                                                          \
				return nullptr;                                                                                        \
                                                                                                                       \
			return object;                                                                                             \
		}                                                                                                              \
	};                                                                                                                 \
	static_assert(sizeof(TCHandleLayout<OpaqueHandleTypesEnum>) == sizeof(void*),                                      \
				  "Size of opaque handle layout isn't equal to pointer size of the system");

// Object Handle System Example
namespace TCore
{
namespace Plugin1
{

// public.h extern "C"
TCORE_DEFINE_HANDLE(P1Random1);

// plugin only includes.h
enum class P1HandleTypes : unsigned short
{
	RANDOMOBJECTTYPE1,
	RANDOMOBJECTTYPE2
};
unsigned long long GetMemOffset(const void* P1Obj)
{
	return 0;
}
void* GetPointer(unsigned long long memOffset)
{
	return nullptr;
}
TCORE_DEFINE_OPAQUE_HANDLE_SYSTEM(P1, P1HandleTypes)

struct P1R1Object : P1ObjectBase<P1R1Object, P1Random1, P1HandleTypes::RANDOMOBJECTTYPE1>
{
	// Object data that's not visible to user
	unsigned short GetExtraFlags() { return 0; }
};
TCORE_DEFINE_HANDLE_TYPE_CONVERTERS(P1R1Object, P1)

} // namespace Plugin1
} // namespace TCore
#endif