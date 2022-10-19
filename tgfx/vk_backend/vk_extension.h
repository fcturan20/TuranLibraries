#pragma once
#include <float.h>

#include <vector>

#include "vk_predefinitions.h"

struct vkext_interface;
extern vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_EXTS;
struct extManager_vkDevice {
  GPU_VKOBJ*                                         m_GPU;
  vkext_interface**                                  m_exts;
  VK_STATICVECTOR<const char*, const char*, 1 << 15> m_activeDevExtNames;

 private:
  // Swapchain
  bool SwapchainDisplay = false;

  // VK_KHR_descriptor_indexing
  // This extension'll be used to support variable descriptor count and NonUniformDescIndexing
  // bool isVariableDescCountSupported = false;

  // VK_KHR_buffer_device_address
  // This extension'll be used to implement pointer support for buffers
  // This extension is better than all DX12. But if gpu doesn't support, VK is worse than DX12 so i
  // still couldn't figure out the best possible way.
  // bool isBufferDeviceAddressSupported = false;

  // VK_KHR_push_descriptor
  // This extension'll be used to implement CallBufferDescriptors for now
  // But in future, maybe this can be used to move all descriptor updating CPU cost to GPU with an
  // TGFX extension Because this is what DX12 does, updating descriptors in command lists (so in
  // GPU)
  // bool isPushDescriptorSupported = false;

  // VK_EXT_inline_uniform_block
  // This is VK only extension, which has extreme use cases on certain AMD GPUs (at the time of
  // writing: March '22)
  // bool isInlineUniformBlockSupported = false;

  // These limits are maximum count of defined resources in material linking (including global
  // buffers and textures) That means; at specified shader input type-> all global shader inputs +
  // general shader inputs + per instance shader inputs shouldn't exceed the related limit
  // unsigned int MaxDescCounts[VKCONST_DYNAMICDESCRIPTORTYPESCOUNT] = {}, MaxDesc_ALL = 0,
  // MaxAccessibleDesc_PerStage = 0, MaxBoundDescSet = 0;

  // bool isDynamicStateVertexInputBindingSupported = false;

  // VK_KHR_timeline_semaphore
  // This feature is expected to be supported.
  // There'll be an emulation layer later for devices that don't support this extension
  // bool isTimelineSemaphoresSupported = false;

 public:
  static void createExtManager(GPU_VKOBJ* gpu);
  void        Describe_SupportedExtensions(GPU_VKOBJ* VKGPU);

  // Find features GPU supports and limitations (properties)
  // Then each extension inspects GPU itself
  void inspect();
};

// Create a header for the extension you want to support
// Struct that stores device data (props, features etc) should be derived from this interface
// Then add an enum to vkext_types, then add it to m_exts list in extManager_vkDevice
struct vkext_interface {
  // Extension structures should contain this as their first variable to identify which extension
  // the struct belongs to.
  // Then as a second variable, struct can store struct's type.
  // vkext_interface's store this to idenfity their extensions
  enum vkext_types : uint16_t {
    depthStencil_vkExtEnum,
    descIndexing_vkExtEnum,
    timelineSemaphores_vkExtEnum,
    vkext_count
  };

  vkext_interface(GPU_VKOBJ* gpu, void* propsStruct, void* featuresStruct);
  virtual void inspect() = 0;
  // If functionality is TGFX Extension, then handle it in this function
  virtual void manage(VkStructureType structType, void* structPtr,
                      extension_tgfx_handle extData) = 0;
  vkext_types  m_type = vkext_count; // Derived classes should change this
  GPU_VKOBJ*   m_gpu  = nullptr;
};
