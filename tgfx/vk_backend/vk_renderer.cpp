/*
  This file is responsible for managing TGFX Command Bundle.
  TGFX Command Bundle = Secondary Command Buffers.
*/
#include "vk_renderer.h"

#include <algorithm>
#include <numeric>
#include <utility>

#include "tgfx_renderer.h"
#include "vk_contentmanager.h"
#include "vk_core.h"
#include "vk_predefinitions.h"
#include "vk_queue.h"
#include "vkext_timelineSemaphore.h"

struct vk_renderer_private {};

#define vkEnumType_cmdType() uint32_t
enum class vk_cmdType : vkEnumType_cmdType(){
  error = VK_PRIM_MIN(vkEnumType_cmdType()), // cmdType neither should be 0 nor 255
  bindBindingTable,
  bindVertexBuffer,
  bindIndexBuffer,
  bindViewport,
  drawNonIndexedDirect,
  drawIndexedDirect,
  drawNonIndexedIndirect,
  drawIndexedIndirect,
  barrierTexture,
  barrierBuffer,
  error_2 = VK_PRIM_MAX(vkEnumType_cmdType())};
// Template struct for new cmd structs
// Then specify the struct in vkCmdStructsLists
struct vkCmdStruct_example {
  static constexpr vk_cmdType type = vk_cmdType::error;
  vkCmdStruct_example()            = default;
};

struct vkCmdStruct_barrierTexture {
  static constexpr vk_cmdType type = vk_cmdType::barrierTexture;

  VkImageMemoryBarrier im_bar = {};
};

struct vk_cmd {
  vk_cmdType type = vk_cmdType::error_2;
  uint64_t   key  = UINT64_MAX;

  // From https://stackoverflow.com/a/46408751
  template <typename... T>
  static constexpr size_t max_sizeof() {
    return std::max({sizeof(T)...});
  }

#define vkCmdStructsLists vkCmdStruct_example, vkCmdStruct_barrierTexture
  static constexpr uint32_t maxCmdStructSize = max_sizeof<vkCmdStructsLists>();
  uint8_t                   data[maxCmdStructSize];
  vk_cmd() : key(0), type(vk_cmdType::error) {}
};
static constexpr uint32_t sdf = sizeof(vk_cmd);

template <typename T>
T* vk_createCmdStruct(vk_cmd* cmd) {
  static_assert(T::type != vk_cmdType::error,
                "You forgot to specify command type as \"type\" variable in command struct");
  cmd->type = T::type;
  static_assert(vk_cmd::maxCmdStructSize >= sizeof(T),
                "You forgot to specify the struct in vk_cmd::maxCmdStructSize!");
  *( T* )cmd->data = T();
  return ( T* )cmd->data;
}

struct CMDBUNDLE_VKOBJ {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::CMDBUNDLE;
  VkCommandBuffer vk_cb      = nullptr;
  QUEUEFAM_VK*    queueFam   = nullptr;
};

// Synchronization Functions

void vk_createFences(gpu_tgfxhnd gpu, unsigned int fenceCount, uint32_t isSignaled,
                     fence_tgfxlsthnd fenceList) {
  GPU_VKOBJ* GPU = core_vk->getGPUs().getOBJfromHANDLE(gpu);
  for (uint32_t i = 0; i < fenceCount; i++) {
    fenceList[i] = vk_createTGFXFence(GPU, isSignaled);
  }
}

// Command Bundle Functions
////////////////////////////

commandBundle_tgfxhnd vk_beginCommandBundle(gpuQueue_tgfxhnd      i_queue,
                                            renderSubPass_tgfxhnd subpassHandle,
                                            extension_tgfxlsthnd  exts) {
  getGPUfromQueueHnd(i_queue);

  VkCommandBuffer vk_cmdBuffer = VK_NULL_HANDLE;
  vk_allocateCmdBuffer(fam, VK_COMMAND_BUFFER_LEVEL_SECONDARY, &vk_cmdBuffer, 1);
  vk_cmd c;
  auto*  tpr = vk_createCmdStruct<vkCmdStruct_barrierTexture>(&c);

  return nullptr;
}
void vk_finishCommandBundle(commandBundle_tgfxhnd bundle, rendererKeySortFunc_tgfx sortFunc,
                            extension_tgfxlsthnd exts) {}
void vk_cmdBindBindingTable(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                            bindingTable_tgfxhnd bindingtable) {}
void vk_cmdBindVertexBuffer(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                            buffer_tgfxhnd buffer, unsigned long long offset,
                            unsigned long long dataSize) {
  // BUFFER_VKOBJ* vb = contentmanager->GETBUFFER_ARRAY().getOBJfromHANDLE(buffer);
  //  vkCmdBindVertexBuffers();
}
void vk_cmdBindIndexBuffers(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                            buffer_tgfxhnd buffer, unsigned long long offset,
                            unsigned char IndexTypeSize) {}
void vk_cmdDrawNonIndexedDirect(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                                unsigned int vertexCount, unsigned int instanceCount,
                                unsigned int firstVertex, unsigned int firstInstance) {}
void vk_cmdDrawIndexedDirect(commandBundle_tgfxhnd bundle, unsigned long long sortKey,
                             unsigned int indexCount, unsigned int instanceCount,
                             unsigned int firstIndex, int vertexOffset,
                             unsigned int firstInstance) {}
void vk_cmdDrawNonIndexedIndirect(commandBundle_tgfxhnd, unsigned long long sortKey,
                                  buffer_tgfxhnd       drawDataBuffer,
                                  unsigned long long   drawDataBufferOffset,
                                  buffer_tgfxhnd       drawCountBuffer,
                                  unsigned long long   drawCountBufferOffset,
                                  extension_tgfxlsthnd exts) {}
void vk_cmdDrawIndexedIndirect(commandBundle_tgfxhnd, unsigned long long sortKey,
                               buffer_tgfxhnd       drawCountBuffer,
                               unsigned long long   drawCountBufferOffset,
                               extension_tgfxlsthnd exts) {}
void vk_destroyCommandBundle(commandBundle_tgfxhnd hnd) {}

void set_VkRenderer_funcPtrs() {
  core_tgfx_main->renderer->cmdBindBindingTable       = vk_cmdBindBindingTable;
  core_tgfx_main->renderer->cmdBindIndexBuffers       = vk_cmdBindIndexBuffers;
  core_tgfx_main->renderer->cmdBindVertexBuffer       = vk_cmdBindVertexBuffer;
  core_tgfx_main->renderer->cmdDrawIndexedDirect      = vk_cmdDrawIndexedDirect;
  core_tgfx_main->renderer->cmdDrawIndexedIndirect    = vk_cmdDrawIndexedIndirect;
  core_tgfx_main->renderer->cmdDrawNonIndexedDirect   = vk_cmdDrawNonIndexedDirect;
  core_tgfx_main->renderer->cmdDrawNonIndexedIndirect = vk_cmdDrawNonIndexedIndirect;
  core_tgfx_main->renderer->beginCommandBundle        = vk_beginCommandBundle;
  core_tgfx_main->renderer->finishCommandBundle       = vk_finishCommandBundle;
  core_tgfx_main->renderer->createFences              = vk_createFences;
  core_tgfx_main->renderer->destroyCommandBundle      = vk_destroyCommandBundle;
  core_tgfx_main->renderer->getFenceValue             = vk_getFenceValue;
  core_tgfx_main->renderer->setFence                  = vk_setFenceValue;
}

void vk_initRenderer() { set_VkRenderer_funcPtrs(); }

// Helper functions

void VK_getQueueAndSharingInfos(gpuQueue_tgfxlsthnd i_queueList, extension_tgfxlsthnd i_exts,
                                uint32_t* o_famList, uint32_t* o_famListSize,
                                VkSharingMode* o_sharingMode) {
  TGFXLISTCOUNT(core_tgfx_main, i_queueList, i_listSize);
  uint32_t   validQueueFamCount = 0;
  GPU_VKOBJ* theGPU             = nullptr;
  for (uint32_t listIndx = 0; listIndx < i_listSize; listIndx++) {
    getGPUfromQueueHnd(i_queueList[listIndx]);
    if (!theGPU) {
      theGPU = gpu;
    }
    assert(theGPU == gpu && "Queues from different devices!");
    bool isPreviouslyAdded = false;
    for (uint32_t validQueueIndx = 0; validQueueIndx < validQueueFamCount; validQueueIndx++) {
      if (o_famList[validQueueIndx] == queue->vk_queueFamIndex) {
        isPreviouslyAdded = true;
        break;
      }
    }
    if (!isPreviouslyAdded) {
      o_famList[validQueueFamCount++] = queue->vk_queueFamIndex;
    }
  }
  if (validQueueFamCount > 1) {
    *o_sharingMode = VK_SHARING_MODE_CONCURRENT;
  } else {
    *o_sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }
  *o_famListSize = validQueueFamCount;
  for (uint32_t i = *o_famListSize; i < VKCONST_MAXQUEUEFAMCOUNT_PERGPU; i++) {
    o_famList[i] = UINT32_MAX;
  }
}

void vk_GetSecondaryCmdBuffers(commandBundle_tgfxlsthnd commandBundleList,
                               VkCommandBuffer* secondaryCmdBuffers, uint32_t* cmdBufferCount) {}