/* There are 5 different branches for Renderer:
1) RG Description: Describes render passes then checks if RG is valid.
    If valid, creates Vulkan objects for the passes (RenderPass, Framebuffer etc.)
    Implemented in RG_Description.cpp
2) RG Static Linking: Creates optimized data structures for dynamic framegraph optimizations.
    Implemented in RG_StaticLinkage.cpp
3) RG Commander: Implements render commands API (Draws, Dispatches, Transfer calls etc.)
    Implemented in RG_Commander.cpp
4) RG Dynamic Linking: Optimizes RG for minimal queue and sync operations and submits recorded CBs.
    Implemented in RG_DynamicLinkage.cpp
5) RG CommandBuffer Recorder: Records command buffers according to the commands from
RG_DynamicLinkage.cpp Implemented in RG_CBRecording.cpp
//For the sake of simplicity; RG_CBRecording, RG_StaticLinkage and RG_DynamicLinkage is implemented
in RG_primitive.cpp for now
//But all of these branches are communicating through RG.h header, so don't include the header in
other systems
*/

#pragma once
#include <tgfx_forwarddeclarations.h>
#include <tgfx_structs.h>

#include <atomic>
#include <vector>

#include "vk_includes.h"

vk_uint32c VKCONST_MAXCMDBUNDLE_PERCALL = 128;

// Renderer data that other parts of the backend can access
struct renderer_funcs;
struct renderer_public {
 public:
};

struct FRAMEBUFFER_VKOBJ;
struct SUBRASTERPASS_VKOBJ;
struct QUEUEFAM_VK;
// Extension: QueueOwnershipTransfer
// Use o_ params with uint32_t queueFamList[VKCONST_MAXQUEUEFAMCOUNT] etc.
void VK_getQueueAndSharingInfos(gpuQueue_tgfxlsthnd i_queueList, extension_tgfxlsthnd i_exts,
                                uint32_t* o_famList, uint32_t* o_famListSize,
                                VkSharingMode* o_sharingMode);

void vk_getSecondaryCmdBuffers(commandBundle_tgfxlsthnd commandBundleList,
                               FRAMEBUFFER_VKOBJ* framebuffer, QUEUEFAM_VK* queueFam,
                               SUBRASTERPASS_VKOBJ* subpass, VkCommandBuffer* secondaryCmdBuffers,
                               uint32_t* cmdBufferCount);

#define getGPUfromQueueHnd(i_queue)                            \
  GPU_VKOBJ*   gpu   = QUEUE_VKOBJ::getGPUfromHandle(i_queue); \
  QUEUEFAM_VK* fam   = QUEUE_VKOBJ::getFAMfromHandle(i_queue); \
  QUEUE_VKOBJ* queue = fam->m_queues.getOBJfromHANDLE(i_queue)