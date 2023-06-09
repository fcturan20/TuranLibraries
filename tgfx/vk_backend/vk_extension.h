#pragma once
#include <float.h>

#include <vector>

#include "vk_predefinitions.h"

struct vkext_interface;
extern vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_EXTS;
struct extManager_vkDevice {
  GPU_VKOBJ*        m_GPU;
  vkext_interface** m_exts;
  const char**      m_activeDevExtNames;
  uint32_t          m_devExtCount = 0;

 private:
  // Swapchain
  bool                               SwapchainDisplay = false;

 public:
  static void createExtManager(GPU_VKOBJ* gpu);
  void        Describe_SupportedExtensions(GPU_VKOBJ* VKGPU);

  // Find features GPU supports and limitations (properties)
  // Then each extension inspects GPU itself
  void inspect();
  const char** getEnabledExtensionNames(uint32_t* count);
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
    dynamicRendering_vkExtEnum,
    dynamicStates_vkExtEnum,
    vkext_count
  };

  vkext_interface(GPU_VKOBJ* gpu, void* propsStruct, void* featuresStruct);
  virtual void inspect() = 0;
  // If functionality is TGFX Extension, then handle it in this function
  virtual void manage(VkStructureType structType, void* structPtr, unsigned int extCount,
                      struct tgfx_extension* const* exts) = 0;
  vkext_types  m_type = vkext_count; // Derived classes should change this
  GPU_VKOBJ*   m_gpu  = nullptr;
};
