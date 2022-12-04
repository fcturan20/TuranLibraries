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
#include "vk_resource.h"
#include "vkext_timelineSemaphore.h"

vk_virmem::dynamicmem* VKGLOBAL_VIRMEM_RENDERER = nullptr;

struct vk_cmd;
struct CMDBUNDLE_VKOBJ {
  bool            isALIVE    = false;
  vk_handleType   HANDLETYPE = VKHANDLETYPEs::CMDBUNDLE;
  static uint16_t GET_EXTRAFLAGS(CMDBUNDLE_VKOBJ* obj) { return 0; }

  // Command Buffer States
  VkPipeline vk_activePipeline = {};
  // To check pipeline compatibility
  VkPipelineLayout      vk_activePipelineLayout                       = {};
  VkDescriptorSetLayout vk_activeDescSets[VKCONST_MAXDESCSET_PERLIST] = {};
  uint8_t               vk_callBuffer[128] = {}; // CallBuffer = Push Constants = 128 byte

  GPU_VKOBJ*          m_gpu             = {};
  vk_cmd*             m_cmds            = {};
  uint64_t            m_cmdCount        = 0;
  pipeline_tgfxhnd    m_defaultPipeline = {};
  VkPipelineBindPoint vk_bindPoint      = {};

  void createCmdBuffer(uint64_t cmdCount);
};

#define vkEnumType_cmdType() uint32_t
enum class vk_cmdType : vkEnumType_cmdType(){
  error = VK_PRIM_MIN(vkEnumType_cmdType()), // cmdType neither should be 0 nor 255
  bindBindingTables,
  bindVertexBuffers,
  bindIndexBuffer,
  setDepthBounds,
  setViewport,
  setScissor,
  drawNonIndexedDirect,
  drawIndexedDirect,
  executeIndirect,
  barrierTexture,
  barrierBuffer,
  bindPipeline,
  dispatch,
  copyBufferToTexture,
  error_2 = VK_PRIM_MAX(vkEnumType_cmdType())};
// Template struct for new cmd structs
// Then specify the struct in vkCmdStructsLists
struct vkCmdStruct_example {
  // Necessary template variables & functions should have "cmd_" prefix
  static constexpr vk_cmdType type = vk_cmdType::error;
  vkCmdStruct_example()            = default;
  void cmd_execute(CMDBUNDLE_VKOBJ* cmdBundle) {}
};

struct vkCmdStruct_barrierTexture {
  static constexpr vk_cmdType cmd_type = vk_cmdType::barrierTexture;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &m_imBar);
  }

  // Command specific variables should have "m_" prefix
  VkImageMemoryBarrier m_imBar = {};
};

struct vkCmdStruct_bindBindingTables {
  static constexpr vk_cmdType cmd_type = vk_cmdType::bindBindingTables;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdBindDescriptorSets(cb, vk_bindPoint, cmdBundle->vk_activePipelineLayout, m_firstSetIndx,
                            m_setCount, vk_sets, 0, nullptr);
    for (uint32_t i = m_firstSetIndx; i < VKCONST_MAXDESCSET_PERLIST; i++) {
      cmdBundle->vk_activeDescSets[i] = vk_setLayouts[i - m_firstSetIndx];
    }
  }
  VkPipelineBindPoint   vk_bindPoint                              = VK_PIPELINE_BIND_POINT_MAX_ENUM;
  VkDescriptorSet       vk_sets[VKCONST_MAXDESCSET_PERLIST]       = {};
  VkDescriptorSetLayout vk_setLayouts[VKCONST_MAXDESCSET_PERLIST] = {};
  uint32_t              m_setCount = 0, m_firstSetIndx = 0;
};

struct vkCmdStruct_bindPipeline {
  static constexpr vk_cmdType cmd_type = vk_cmdType::bindPipeline;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdBindPipeline(cb, vk_bindPoint, vk_pipeline);
    cmdBundle->vk_activePipeline       = vk_pipeline;
    cmdBundle->vk_activePipelineLayout = vk_pipelineLayout;
  }
  VkPipelineBindPoint vk_bindPoint      = VK_PIPELINE_BIND_POINT_MAX_ENUM;
  VkPipeline          vk_pipeline       = {};
  VkPipelineLayout    vk_pipelineLayout = {};
};

struct vkCmdStruct_dispatch {
  static constexpr vk_cmdType cmd_type = vk_cmdType::dispatch;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdDispatch(cb, m_dispatchSize.x, m_dispatchSize.y, m_dispatchSize.z);
  };
  uvec3_tgfx m_dispatchSize;
};

struct vkCmdStruct_setViewport {
  static constexpr vk_cmdType cmd_type = vk_cmdType::setViewport;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdSetViewport(cb, 0, 1, &vk_viewport);
  };

  VkViewport vk_viewport = {};
};

struct vkCmdStruct_setScissor {
  static constexpr vk_cmdType cmd_type = vk_cmdType::setScissor;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdSetScissor(cb, 0, 1, &vk_rect);
  };
  VkRect2D vk_rect = {};
};

struct vkCmdStruct_drawNonIndexedDirect {
  static constexpr vk_cmdType cmd_type = vk_cmdType::drawNonIndexedDirect;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdDraw(cb, vertexCount, instanceCount, firstVertex, firstInstance);
  };
  uint32_t vertexCount = {}, instanceCount = {}, firstVertex = {}, firstInstance = {};
};

struct vkCmdStruct_drawIndexedDirect {
  static constexpr vk_cmdType cmd_type = vk_cmdType::drawIndexedDirect;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdDrawIndexed(cb, indxCount, instanceCount, firstIndx, vertexOffset, firstInstance);
  };
  uint32_t indxCount = {}, instanceCount = {}, firstIndx = {}, firstInstance = {};
  int32_t  vertexOffset = {};
};

struct vkCmdStruct_bindIndexBuffer {
  static constexpr vk_cmdType cmd_type = vk_cmdType::bindIndexBuffer;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdBindIndexBuffer(cb, vk_buffer, vk_offset, vk_indexType);
  };
  VkBuffer     vk_buffer;
  VkDeviceSize vk_offset;
  VkIndexType  vk_indexType;
};

struct vkCmdStruct_setDepthBounds {
  static constexpr vk_cmdType cmd_type = vk_cmdType::setDepthBounds;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdSetDepthBounds(cb, min, max);
  };
  float min = 0.0f, max = 1.0f;
};

struct vkCmdStruct_copyBufferToTexture {
  static constexpr vk_cmdType cmd_type = vk_cmdType::copyBufferToTexture;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdCopyBufferToImage(cb, vk_src, vk_dst, vk_dstImageLayout, 1, &vk_copy);
  };
  VkBuffer          vk_src;
  VkImage           vk_dst;
  VkImageLayout     vk_dstImageLayout;
  VkBufferImageCopy vk_copy;
};

struct vkCmdStruct_bindVertexBuffers {
  static constexpr vk_cmdType cmd_type = vk_cmdType::bindVertexBuffers;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    vkCmdBindVertexBuffers(cb, firstBinding, bindingCount, vk_buffers, vk_bufferOffsets);
  };
  VkBuffer     vk_buffers[VKCONST_MAXVERTEXBINDINGCOUNT];
  VkDeviceSize vk_bufferOffsets[VKCONST_MAXVERTEXBINDINGCOUNT];
  uint32_t     firstBinding, bindingCount;
};

VkDeviceSize vk_findIndirectOperationDataSize(indirectOperationType_tgfx opType) {
  switch (opType) {
    case indirectOperationType_tgfx_DRAWNONINDEXED: return sizeof(VkDrawIndirectCommand);
    case indirectOperationType_tgfx_DRAWINDEXED: return sizeof(VkDrawIndexedIndirectCommand);
    case indirectOperationType_tgfx_DISPATCH: return sizeof(VkDispatchIndirectCommand);
  }
  return UINT64_MAX;
}
vk_uint32c VKCONST_MAXINDIRECTOPERATIONSTATECOUNT = 128;
struct vkCmdStruct_executeIndirect {
  static constexpr vk_cmdType cmd_type = vk_cmdType::executeIndirect;

  void cmd_execute(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* cmdBundle) {
    VkDeviceSize activeOffset = vk_bufferOffset;
    for (uint32_t stateIndx = 0; stateIndx < VKCONST_MAXINDIRECTOPERATIONSTATECOUNT &&
                                 opStates[stateIndx].opType != indirectOperationType_tgfx_UNDEF;
         stateIndx++) {
      uint64_t                   loopCount = 1, drawCount = opStates[stateIndx].opCount;
      indirectOperationType_tgfx opType = opStates[stateIndx].opType;
      uint64_t                   indirectArgumentDataSize =
        vk_findIndirectOperationDataSize(opStates[stateIndx].opType);
      // If GPU doesn't support multiDrawIndirect or execute type is compute, call same VkCmd*
      // multiple times with incrementing offsets
      if (!cmdBundle->m_gpu->vk_featuresDev.features.multiDrawIndirect ||
          opType == indirectOperationType_tgfx_DISPATCH) {
        loopCount = opStates[stateIndx].opCount;
        drawCount = 1;
      }
      for (uint32_t loopIndx = 0; loopIndx < loopCount; loopIndx++) {
        switch (opType) {
          case indirectOperationType_tgfx_DRAWNONINDEXED:
            vkCmdDrawIndirect(cb, vk_buffer, activeOffset, drawCount, indirectArgumentDataSize);
            break;
          case indirectOperationType_tgfx_DRAWINDEXED:
            vkCmdDrawIndexedIndirect(cb, vk_buffer, activeOffset, drawCount,
                                     indirectArgumentDataSize);
            break;
          case indirectOperationType_tgfx_DISPATCH:
            vkCmdDispatchIndirect(cb, vk_buffer, activeOffset);
            break;
          default:
            printer(result_tgfx_NOTCODED, "Indirect Operation Type isn't supported!");
            return;
        }
        activeOffset += indirectArgumentDataSize * drawCount;
      }
    }
  };
  struct vk_indirectOperationState {
    uint32_t                   opCount;
    indirectOperationType_tgfx opType;
  };
  vk_indirectOperationState opStates[VKCONST_MAXINDIRECTOPERATIONSTATECOUNT] = {};
  VkBuffer                  vk_buffer;
  VkDeviceSize              vk_bufferOffset;
};

struct vk_cmd {
  vk_cmdType cmd_type = vk_cmdType::error_2;

  // From https://stackoverflow.com/a/46408751
  template <typename... T>
  static constexpr size_t max_sizeof() {
    return std::max({sizeof(T)...});
  }

#define vkCmdStructsLists                                                          \
  vkCmdStruct_example, vkCmdStruct_barrierTexture, vkCmdStruct_bindBindingTables,  \
    vkCmdStruct_bindPipeline, vkCmdStruct_dispatch, vkCmdStruct_bindVertexBuffers, \
    vkCmdStruct_executeIndirect
  static constexpr uint32_t maxCmdStructSize = max_sizeof<vkCmdStructsLists>();
  uint8_t                   cmd_data[maxCmdStructSize];
  vk_cmd() : cmd_type(vk_cmdType::error) {}
};

template <typename T>
T* vk_createCmdStruct(vk_cmd* cmd) {
  static_assert(T::cmd_type != vk_cmdType::error,
                "You forgot to specify command type as \"cmd_type\" variable in command struct");
  cmd->cmd_type = T::cmd_type;
  static_assert(vk_cmd::maxCmdStructSize >= sizeof(T),
                "You forgot to specify the struct in vk_cmd::maxCmdStructSize!");
  *( T* )cmd->cmd_data = T();
  return ( T* )cmd->cmd_data;
}

void vk_executeCmd(VkCommandBuffer cb, CMDBUNDLE_VKOBJ* bundle, const vk_cmd& cmd) {
  switch (cmd.cmd_type) {
    case vk_cmdType::barrierTexture:
      (( vkCmdStruct_barrierTexture* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::bindBindingTables:
      (( vkCmdStruct_bindBindingTables* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::bindPipeline:
      (( vkCmdStruct_bindPipeline* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::dispatch:
      (( vkCmdStruct_dispatch* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::setScissor:
      (( vkCmdStruct_setScissor* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::setViewport:
      (( vkCmdStruct_setViewport* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::drawNonIndexedDirect:
      (( vkCmdStruct_drawNonIndexedDirect* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::setDepthBounds:
      (( vkCmdStruct_setDepthBounds* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::copyBufferToTexture:
      (( vkCmdStruct_copyBufferToTexture* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::bindVertexBuffers:
      (( vkCmdStruct_bindVertexBuffers* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::bindIndexBuffer:
      (( vkCmdStruct_bindIndexBuffer* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::drawIndexedDirect:
      (( vkCmdStruct_drawIndexedDirect* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::executeIndirect:
      (( vkCmdStruct_executeIndirect* )cmd.cmd_data)->cmd_execute(cb, bundle);
      break;
    case vk_cmdType::error:
    case vk_cmdType::error_2: printf("One of the cmds is not used!"); break;
    default: assert_vk(0 && "Don't forget to specify command execution in vk_executeCmd()!");
  }
}

void CMDBUNDLE_VKOBJ::createCmdBuffer(uint64_t cmdCount) {
  vk_virmem::dynamicmem* cmdVirMem = vk_virmem::allocate_dynamicmem(sizeof(vk_cmd) * cmdCount);
  // I don't want to bother with commit operations for now, commit all cmds
  m_cmds     = new (cmdVirMem) vk_cmd[cmdCount];
  m_cmdCount = cmdCount;
}

struct vk_renderer_private {
  VK_STATICVECTOR<CMDBUNDLE_VKOBJ, commandBundle_tgfxhnd, UINT16_MAX> m_cmdBundles;
};
vk_renderer_private* hiddenRenderer = nullptr;

// Synchronization Functions

void vk_createFences(gpu_tgfxhnd gpu, unsigned int fenceCount, uint32_t isSignaled,
                     fence_tgfxlsthnd fenceList) {
  GPU_VKOBJ* GPU = core_vk->getGPUs().getOBJfromHANDLE(gpu);
  for (uint32_t i = 0; i < fenceCount; i++) {
    fenceList[i] = vk_createTGFXFence(GPU, isSignaled);
  }
}
void vk_destroyFence(fence_tgfxhnd fence) {
  for (uint8_t gpuIndx = 0; gpuIndx < core_vk->getGPUs().size(); gpuIndx++) {
    vkext_timelineSemaphore* ext = ( vkext_timelineSemaphore* )core_vk->getGPUs()[gpuIndx]
                                     ->ext()
                                     ->m_exts[vkext_interface::timelineSemaphores_vkExtEnum];
    FENCE_VKOBJ* vkFence = ext->fences.getOBJfromHANDLE(fence);
    if (vkFence) {
      vkDestroySemaphore(core_vk->getGPUs()[gpuIndx]->vk_logical, vkFence->vk_timelineSemaphore,
                         nullptr);
      vkFence->isALIVE = false;
    }
  }
}

// Command Bundle Functions
////////////////////////////

commandBundle_tgfxhnd vk_beginCommandBundle(gpu_tgfxhnd gpu, unsigned long long maxCmdCount,
                                            pipeline_tgfxhnd     defaultPipeline,
                                            extension_tgfxlsthnd exts) {
  VkCommandBuffer vk_cmdBuffer = VK_NULL_HANDLE;
  cmdPool_vk*     cmdPool;
  GPU_VKOBJ*      GPU = core_vk->getGPUs().getOBJfromHANDLE(gpu);
  if (!GPU) {
    return nullptr;
  }
  CMDBUNDLE_VKOBJ* cmdBundle = hiddenRenderer->m_cmdBundles.add();

  cmdBundle->m_gpu = GPU;
  cmdBundle->createCmdBuffer(maxCmdCount);
  cmdBundle->m_defaultPipeline = defaultPipeline;
  if (defaultPipeline) {
    PIPELINE_VKOBJ* pipe    = contentmanager->GETPIPELINE_ARRAY().getOBJfromHANDLE(defaultPipeline);
    cmdBundle->vk_bindPoint = pipe->vk_type;
  } else {
    cmdBundle->vk_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
  }

  return hiddenRenderer->m_cmdBundles.returnHANDLEfromOBJ(cmdBundle);
}
void vk_finishCommandBundle(commandBundle_tgfxhnd i_bundle, extension_tgfxlsthnd exts) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);

  // MOVE THIS RECORDING STAGE BECAUSE FRAMEBUFFER WILL RECORD ALREADY!
}
void vk_destroyCommandBundle(commandBundle_tgfxhnd hnd) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(hnd);
  /*
  auto&            queueFams = bundle->m_gpu->manager()->m_queueFams;
  for (uint32_t queueFamIndx = 0; queueFamIndx < queueFams.size(); queueFamIndx++) {
    // Check if queue family supports this bundle
    if ((bundle->vk_bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS &&
         !queueFams[queueFamIndx]->m_supportFlag.is_GRAPHICSsupported) ||
        (bundle->vk_bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE &&
         !queueFams[queueFamIndx]->m_supportFlag.is_COMPUTEsupported)) {
      continue;
    }
    for (uint32_t threadIndx = 0; threadIndx < threadcount; threadIndx++) {
      cmdBundleRef_vk& cmdBundleRef = queueFams[queueFamIndx]->m_cmdBundleRefs[threadIndx];
      if (cmdBundleRef.m_cmdBundle == hnd) {
        vkFreeCommandBuffers(bundle->m_gpu->vk_logical,
                             cmdBundleRef.m_cmdPool, 1,
                             &cmdBundleRef.vk_cmdBuffer);
      }
    }
  }*/
  hiddenRenderer->m_cmdBundles.erase(hiddenRenderer->m_cmdBundles.getINDEX_byOBJ(bundle));
}

void vk_cmdBindBindingTables(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey,
                             bindingTable_tgfxlsthnd bindingTables, unsigned int firstSetIndx,
                             pipelineType_tgfx pipelineType) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto* cmd = vk_createCmdStruct<vkCmdStruct_bindBindingTables>(&bundle->m_cmds[sortKey]);

  {
    TGFXLISTCOUNT(core_tgfx_main, bindingTables, bindingTableListSize);
    for (uint32_t i = 0; i < bindingTableListSize; i++) {
      BINDINGTABLEINST_VKOBJ* bindingTable =
        contentmanager->GETBINDINGTABLE_ARRAY().getOBJfromHANDLE(bindingTables[i]);
      if (bindingTable) {
        cmd->vk_sets[cmd->m_setCount] = bindingTable->vk_set;
        BINDINGTABLETYPE_VKOBJ* type =
          contentmanager->GETBINDINGTABLETYPE_ARRAY().getOBJfromHANDLE(bindingTable->m_type);
        cmd->vk_setLayouts[cmd->m_setCount++] = type->vk_layout;
      }
    }
  }

  cmd->vk_bindPoint   = vk_findPipelineBindPoint(pipelineType);
  cmd->m_firstSetIndx = firstSetIndx;
}
void vk_cmdBindPipeline(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey,
                        pipeline_tgfxhnd pipeline) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto*            cmd    = vk_createCmdStruct<vkCmdStruct_bindPipeline>(&bundle->m_cmds[sortKey]);
  PIPELINE_VKOBJ*  pipe   = contentmanager->GETPIPELINE_ARRAY().getOBJfromHANDLE(pipeline);

  assert_vk(pipe->vk_type == bundle->vk_bindPoint &&
            "You can't call this type of operation in this command bundle!");
  cmd->vk_bindPoint      = pipe->vk_type;
  cmd->vk_pipeline       = pipe->vk_object;
  cmd->vk_pipelineLayout = pipe->vk_layout;
}
void vk_cmdSetViewport(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey,
                       viewportInfo_tgfx viewport) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto*            cmd    = vk_createCmdStruct<vkCmdStruct_setViewport>(&bundle->m_cmds[sortKey]);

  cmd->vk_viewport.x        = viewport.topLeftCorner.x;
  cmd->vk_viewport.y        = viewport.topLeftCorner.y;
  cmd->vk_viewport.width    = viewport.size.x;
  cmd->vk_viewport.height   = viewport.size.y;
  cmd->vk_viewport.minDepth = viewport.depthMinMax.x;
  cmd->vk_viewport.maxDepth = viewport.depthMinMax.y;
}
void vk_cmdSetScissor(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey, ivec2_tgfx offset,
                      uvec2_tgfx size) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto*            cmd    = vk_createCmdStruct<vkCmdStruct_setScissor>(&bundle->m_cmds[sortKey]);

  cmd->vk_rect.offset.x      = offset.x;
  cmd->vk_rect.offset.y      = offset.x;
  cmd->vk_rect.extent.width  = size.x;
  cmd->vk_rect.extent.height = size.y;
}
void vk_cmdSetDepthBounds(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey, float min,
                          float max) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto*            cmd = vk_createCmdStruct<vkCmdStruct_setDepthBounds>(&bundle->m_cmds[sortKey]);

  cmd->min = min;
  cmd->max = max;
};
void vk_cmdBindVertexBuffers(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey,
                             unsigned int firstBinding, unsigned int bindingCount,
                             const buffer_tgfxhnd* buffers, const unsigned long long* offsets) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto* cmd = vk_createCmdStruct<vkCmdStruct_bindVertexBuffers>(&bundle->m_cmds[sortKey]);

  cmd->firstBinding = firstBinding;
  cmd->bindingCount = bindingCount;
  for (uint32_t i = 0; i < bindingCount; i++) {
    cmd->vk_bufferOffsets[i] = offsets[i];
    cmd->vk_buffers[i] = contentmanager->GETBUFFER_ARRAY().getOBJfromHANDLE(buffers[i])->vk_buffer;
  }
}
void vk_cmdBindIndexBuffer(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey,
                           buffer_tgfxhnd buffer, unsigned long long offset,
                           unsigned char IndexTypeSize) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto*            cmd = vk_createCmdStruct<vkCmdStruct_bindIndexBuffer>(&bundle->m_cmds[sortKey]);

  cmd->vk_buffer = contentmanager->GETBUFFER_ARRAY().getOBJfromHANDLE(buffer)->vk_buffer;
  switch (IndexTypeSize) {
    case 1: cmd->vk_indexType = VK_INDEX_TYPE_UINT8_EXT;
    case 2: cmd->vk_indexType = VK_INDEX_TYPE_UINT16; break;
    case 4: cmd->vk_indexType = VK_INDEX_TYPE_UINT32; break;
  }
  cmd->vk_offset = offset;
}
void vk_cmdDrawNonIndexedDirect(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey,
                                unsigned int vertexCount, unsigned int instanceCount,
                                unsigned int firstVertex, unsigned int firstInstance) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto* cmd = vk_createCmdStruct<vkCmdStruct_drawNonIndexedDirect>(&bundle->m_cmds[sortKey]);

  cmd->firstInstance = firstInstance;
  cmd->firstVertex   = firstVertex;
  cmd->vertexCount   = vertexCount;
  cmd->instanceCount = instanceCount;
}
void vk_cmdDrawIndexedDirect(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey,
                             unsigned int indexCount, unsigned int instanceCount,
                             unsigned int firstIndex, int vertexOffset,
                             unsigned int firstInstance) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto* cmd = vk_createCmdStruct<vkCmdStruct_drawIndexedDirect>(&bundle->m_cmds[sortKey]);

  cmd->firstIndx     = firstIndex;
  cmd->firstInstance = firstInstance;
  cmd->indxCount     = indexCount;
  cmd->instanceCount = instanceCount;
  cmd->vertexOffset  = vertexOffset;
}
void vk_cmdExecuteIndirect(commandBundle_tgfxhnd i_bundle, unsigned long long sortKey,
                           unsigned int                      operationCount,
                           const indirectOperationType_tgfx* operationTypes,
                           buffer_tgfxhnd dataBffr, unsigned long long drawDataBufferOffset,
                           extension_tgfxlsthnd exts) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(i_bundle);
  auto*            cmd = vk_createCmdStruct<vkCmdStruct_executeIndirect>(&bundle->m_cmds[sortKey]);

  if (operationCount > VKCONST_MAXINDIRECTOPERATIONSTATECOUNT) {
    printer(
      result_tgfx_FAIL,
      "Exceeding Vulkan backend's limit of operationCount in a single cmdExecuteIndirect call!");
    return;
  }
  uint32_t stateIndx = 0;
  for (uint32_t i = 0; i < operationCount && stateIndx < VKCONST_MAXINDIRECTOPERATIONSTATECOUNT;) {
    indirectOperationType_tgfx opType = operationTypes[i];
    cmd->opStates[stateIndx].opType   = opType;
    cmd->opStates[stateIndx].opCount  = 0;
    for (; i < operationCount && opType == operationTypes[i];
         i++, cmd->opStates[stateIndx].opCount++) {
    }
    stateIndx++;
  }
  if (stateIndx == VKCONST_MAXINDIRECTOPERATIONSTATECOUNT) {
    printer(result_tgfx_FAIL,
            "Exceeding Vulkan backend's limit of operationState (operation type change point "
            "count) in a single cmdExecuteIndirect call!");
    cmd->opStates[0].opType = indirectOperationType_tgfx_UNDEF;
    return;
  }
  cmd->vk_buffer       = contentmanager->GETBUFFER_ARRAY().getOBJfromHANDLE(dataBffr)->vk_buffer;
  cmd->vk_bufferOffset = drawDataBufferOffset;
}
void vk_cmdBarrierTexture(commandBundle_tgfxhnd bndl, unsigned long long key,
                          texture_tgfxhnd i_texture, image_access_tgfx lastAccess,
                          image_access_tgfx nextAccess, textureUsageMask_tgfxflag lastUsage,
                          textureUsageMask_tgfxflag nextUsage, extension_tgfxlsthnd exts) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(bndl);
  auto*            cmdBar = vk_createCmdStruct<vkCmdStruct_barrierTexture>(&bundle->m_cmds[key]);
  cmdBar->m_imBar.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  cmdBar->m_imBar.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  cmdBar->m_imBar.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  cmdBar->m_imBar.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  cmdBar->m_imBar.subresourceRange.baseArrayLayer = 0;
  cmdBar->m_imBar.subresourceRange.baseMipLevel   = 0;
  cmdBar->m_imBar.subresourceRange.layerCount     = 1;
  cmdBar->m_imBar.subresourceRange.levelCount     = 1;
  TEXTURE_VKOBJ* texture = contentmanager->GETTEXTURES_ARRAY().getOBJfromHANDLE(i_texture);
  cmdBar->m_imBar.image  = texture->vk_image;
  vk_findImageAccessPattern(lastAccess, cmdBar->m_imBar.srcAccessMask, cmdBar->m_imBar.oldLayout);
  vk_findImageAccessPattern(nextAccess, cmdBar->m_imBar.dstAccessMask, cmdBar->m_imBar.newLayout);
  cmdBar->m_imBar.pNext = nullptr;
}
void vk_cmdDispatch(commandBundle_tgfxhnd bndl, unsigned long long key, uvec3_tgfx dispatchSize) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(bndl);
  auto*            cmd    = vk_createCmdStruct<vkCmdStruct_dispatch>(&bundle->m_cmds[key]);

  cmd->m_dispatchSize = dispatchSize;
}
void vk_cmdCopyBufferToTexture(commandBundle_tgfxhnd bndl, unsigned long long key,
                               buffer_tgfxhnd srcBuffer, unsigned long long bufferOffset,
                               texture_tgfxhnd dstTexture, image_access_tgfx lastAccess,
                               extension_tgfxlsthnd exts) {
  CMDBUNDLE_VKOBJ* bundle = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(bndl);
  auto*            cmd = vk_createCmdStruct<vkCmdStruct_copyBufferToTexture>(&bundle->m_cmds[key]);

  BUFFER_VKOBJ*  buffer  = contentmanager->GETBUFFER_ARRAY().getOBJfromHANDLE(srcBuffer);
  TEXTURE_VKOBJ* texture = contentmanager->GETTEXTURES_ARRAY().getOBJfromHANDLE(dstTexture);

  cmd->vk_src = buffer->vk_buffer;
  cmd->vk_dst = texture->vk_image;
  VkAccessFlags flag;
  vk_findImageAccessPattern(lastAccess, flag, cmd->vk_dstImageLayout);
  cmd->vk_copy.imageOffset        = {};
  cmd->vk_copy.imageExtent.width  = texture->m_width;
  cmd->vk_copy.imageExtent.height = texture->m_height;
  cmd->vk_copy.imageExtent.depth  = 1;
  cmd->vk_copy.bufferImageHeight  = 0;
  cmd->vk_copy.bufferOffset       = bufferOffset;
  cmd->vk_copy.bufferRowLength    = 0;
  if (texture->m_channels == texture_channels_tgfx_D32) {
    cmd->vk_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  } else if (texture->m_channels == texture_channels_tgfx_D24S8) {
    cmd->vk_copy.imageSubresource.aspectMask =
      VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  } else {
    cmd->vk_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }
  cmd->vk_copy.imageSubresource.baseArrayLayer = 0;
  cmd->vk_copy.imageSubresource.layerCount     = 1;
  cmd->vk_copy.imageSubresource.mipLevel       = 0;
}

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
    assert_vk(theGPU == gpu && "Queues from different devices!");
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

void vk_getSecondaryCmdBuffers(commandBundle_tgfxlsthnd commandBundleList, QUEUEFAM_VK* queueFam,
                               VkCommandBuffer* secondaryCmdBuffers, uint32_t* cmdBufferCount) {
  uint32_t bundleCount = 0;
  TGFXLISTCOUNT(core_tgfx_main, commandBundleList, cmdListCount);
  for (uint32_t bundleListIndx = 0; bundleListIndx < cmdListCount; bundleListIndx++) {
    if (!commandBundleList[bundleListIndx]) {
      continue;
    }
    commandBundle_tgfxhnd bundleHnd = commandBundleList[bundleListIndx];
    CMDBUNDLE_VKOBJ*      bundle    = hiddenRenderer->m_cmdBundles.getOBJfromHANDLE(bundleHnd);

    if (!bundle || !bundle->isALIVE || bundleCount >= VKCONST_MAXCMDBUNDLE_PERCALL) {
      continue;
    }

    bool isFound = false;
    for (uint32_t i = 0; i < queueFam->m_cmdBundleCount && !isFound; i++) {
      if (queueFam->m_cmdBundleRefs[i].m_cmdBundle == commandBundleList[bundleListIndx]) {
        secondaryCmdBuffers[bundleCount++] = queueFam->m_cmdBundleRefs[i].vk_cmdBuffer;
        isFound                            = true;
      }
    }

    // There is room to create a new cmdBundle reference, so create it
    if (!isFound) {
      cmdPool_vk*     cmdPool   = {};
      VkCommandBuffer cmdBuffer = {};
      vk_allocateCmdBuffer(queueFam, VK_COMMAND_BUFFER_LEVEL_SECONDARY, cmdPool, &cmdBuffer, 1);

      VkCommandBufferInheritanceRenderingInfo rInfo = {};

      VkCommandBufferInheritanceInfo secInfo = {};
      secInfo.sType                          = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
      if (bundle->vk_bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS) {
        rInfo.sType    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
        rInfo.viewMask = 0;
        PIPELINE_VKOBJ* defaultPipe =
          contentmanager->GETPIPELINE_ARRAY().getOBJfromHANDLE(bundle->m_defaultPipeline);
        rInfo.pColorAttachmentFormats = defaultPipe->vk_colorAttachmentFormats;
        while (rInfo.colorAttachmentCount < TGFX_RASTERSUPPORT_MAXCOLORRT_SLOTCOUNT &&
               rInfo.pColorAttachmentFormats[rInfo.colorAttachmentCount] != VK_FORMAT_UNDEFINED) {
          rInfo.colorAttachmentCount++;
        }
        rInfo.depthAttachmentFormat = defaultPipe->vk_depthAttachmentFormat;
        rInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
        secInfo.pNext               = &rInfo;
      }

      VkCommandBufferBeginInfo bi = {};
      bi.flags                    = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT |
                 ((bundle->vk_bindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
                    ? VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
                    : 0);
      bi.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      bi.pInheritanceInfo = &secInfo;

      assert_vk(!ThrowIfFailed(vkBeginCommandBuffer(cmdBuffer, &bi),
                               "vkBeginCommandBuffer() shouldn't fail!"));
      for (uint64_t cmdIndx = 0; cmdIndx < bundle->m_cmdCount; cmdIndx++) {
        vk_executeCmd(cmdBuffer, bundle, bundle->m_cmds[cmdIndx]);
      }
      assert_vk(
        !ThrowIfFailed(vkEndCommandBuffer(cmdBuffer), "vkEndCommandBuffer() shouldn't fail!"));

      queueFam->m_cmdBundleRefs[queueFam->m_cmdBundleCount].m_cmdBundle    = bundleHnd;
      queueFam->m_cmdBundleRefs[queueFam->m_cmdBundleCount].m_cmdPool      = cmdPool;
      queueFam->m_cmdBundleRefs[queueFam->m_cmdBundleCount++].vk_cmdBuffer = cmdBuffer;
      secondaryCmdBuffers[bundleCount++]                                   = cmdBuffer;
    }
  }
  *cmdBufferCount = bundleCount;
}

void set_VkRenderer_funcPtrs() {
  core_tgfx_main->renderer->cmdBindBindingTables    = vk_cmdBindBindingTables;
  core_tgfx_main->renderer->cmdBindIndexBuffer      = vk_cmdBindIndexBuffer;
  core_tgfx_main->renderer->cmdBindVertexBuffers    = vk_cmdBindVertexBuffers;
  core_tgfx_main->renderer->cmdDrawIndexedDirect    = vk_cmdDrawIndexedDirect;
  core_tgfx_main->renderer->cmdExecuteIndirect      = vk_cmdExecuteIndirect;
  core_tgfx_main->renderer->cmdDrawNonIndexedDirect = vk_cmdDrawNonIndexedDirect;
  core_tgfx_main->renderer->cmdBarrierTexture       = vk_cmdBarrierTexture;
  core_tgfx_main->renderer->cmdBindPipeline         = vk_cmdBindPipeline;
  core_tgfx_main->renderer->cmdDispatch             = vk_cmdDispatch;
  core_tgfx_main->renderer->cmdSetViewport          = vk_cmdSetViewport;
  core_tgfx_main->renderer->cmdSetScissor           = vk_cmdSetScissor;
  core_tgfx_main->renderer->cmdSetDepthBounds       = vk_cmdSetDepthBounds;
  core_tgfx_main->renderer->cmdCopyBufferToTexture  = vk_cmdCopyBufferToTexture;

  core_tgfx_main->renderer->beginCommandBundle   = vk_beginCommandBundle;
  core_tgfx_main->renderer->finishCommandBundle  = vk_finishCommandBundle;
  core_tgfx_main->renderer->createFences         = vk_createFences;
  core_tgfx_main->renderer->destroyCommandBundle = vk_destroyCommandBundle;
  core_tgfx_main->renderer->getFenceValue        = vk_getFenceValue;
  core_tgfx_main->renderer->setFence             = vk_setFenceValue;
  core_tgfx_main->renderer->destroyFence         = vk_destroyFence;
}

void vk_initRenderer() {
  set_VkRenderer_funcPtrs();
  VKGLOBAL_VIRMEM_RENDERER = vk_virmem::allocate_dynamicmem(sizeof(vk_renderer_private));
  hiddenRenderer           = new (VKGLOBAL_VIRMEM_RENDERER) vk_renderer_private;
}
