#pragma once
#include "vk_predefinitions.h"

namespace TGFX
{
namespace Vulkan
{
struct ExtensionManager
{
	GPU_VKOBJ* m_GPU;
	IVkExt** m_exts;
	const char** m_activeDevExtNames;
	uint32_t m_devExtCount = 0;

public:
	static void createExtManager(GPU_VKOBJ* gpu);
	void Describe_SupportedExtensions(GPU_VKOBJ* VKGPU);

	// Find features GPU supports and limitations (properties)
	// Then each extension inspects GPU itself
	void inspect();
	const char** getEnabledExtensionNames(uint32_t* count);
};

// Create a header for the extension you want to support
// Struct that stores device data (props, features etc) should be derived from this interface
// Then add an enum to vkext_types, then add it to m_exts list in extManager_vkDevice
struct IVkExt
{
	// Extension structures should contain this as their first variable to identify which extension
	// the struct belongs to.
	// Then as a second variable, struct can store struct's type.
	// vkext_interface's store this to idenfity their extensions
	enum Types : uint16_t
	{
		DepthStencilExtension,
		DescriptorIndexingExtension,
		TimelineSemaphoresExtension,
		DynamicRenderingExtension,
		DynamicStatesExtension,
		InvalidExtension
	};

	IVkExt(GPU_VKOBJ* gpu, void* propsStruct, void* featuresStruct);
	virtual void Inspect() = 0;
	// If functionality is TGFX Extension, then handle it in this function
	virtual void Manage(VkStructureType structType, void* structPtr, unsigned int extCount, TGfxExtension* exts) = 0;
	Types Type = InvalidExtension; // Derived classes should change this
	GPU_VKOBJ* GPU = nullptr;
};
} // namespace Vulkan
} // namespace TGFX