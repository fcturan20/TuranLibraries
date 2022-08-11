#pragma once
#include "vk_predefinitions.h"
#include <float.h>


struct depthstencilslot_vk;
struct extension_manager {
private:
    //Swapchain
    bool SwapchainDisplay = false;

    //Seperated Depth Stencil
    bool SeperatedDepthStencilLayouts = false;

    // VK_KHR_descriptor_indexing
    // This extension'll be used to support variable descriptor count and NonUniformDescIndexing
    bool isVariableDescCountSupported = false;

    // VK_KHR_buffer_device_address
    // This extension'll be used to implement pointer support for buffers
    // This extension is better than all DX12. But if gpu doesn't support, VK is worse than DX12 so i still couldn't figure out the best possible way.
    bool isBufferDeviceAddressSupported = false;

    // VK_KHR_push_descriptor
    // This extension'll be used to implement CallBufferDescriptors for now
    // But in future, maybe this can be used to move all descriptor updating CPU cost to GPU with an TGFX extension
    // Because this is what DX12 does, updating descriptors in command lists (so in GPU)
    bool isPushDescriptorSupported = false;

    // VK_EXT_inline_uniform_block
    // This is VK only extension, which has extreme use cases on certain AMD GPUs (at the time of writing: March '22)
    bool isInlineUniformBlockSupported = false;

    //These limits are maximum count of defined resources in material linking (including global buffers and textures)
    //That means; at specified shader input type-> all global shader inputs + general shader inputs + per instance shader inputs shouldn't exceed the related limit
    unsigned int MaxDescCounts[VKCONST_DYNAMICDESCRIPTORTYPESCOUNT] = {}, MaxDesc_ALL = 0, MaxAccessibleDesc_PerStage = 0, MaxBoundDescSet = 0;
public:
    extension_manager();
    void (*Fill_DepthAttachmentReference)(VkAttachmentReference& Ref, unsigned int index,
        textureChannels_tgfx channels, operationtype_tgfx DEPTHOPTYPE, operationtype_tgfx STENCILOPTYPE);
    void (*Fill_DepthAttachmentDescription)(VkAttachmentDescription& Desc, depthstencilslot_vk* DepthSlot);

    //GETTER-SETTERS
    inline bool ISSUPPORTED_DESCINDEXING() { return isVariableDescCountSupported; }
    inline unsigned int GETMAXDESCCOUNT_ALLALLOCATIONs(unsigned char DESCSETID) { return MaxDescCounts[DESCSETID]; }
    inline unsigned int GETMAXDESCALL() { return MaxDesc_ALL; }
    inline unsigned int GETMAXDESCPERSTAGE() { return MaxAccessibleDesc_PerStage; }
    inline unsigned int GETMAXBOUNDDESCSETCOUNT() { return MaxBoundDescSet; }
    inline bool ISSUPPORTED_SEPERATEDDEPTHSTENCILLAYOUTS() { return SeperatedDepthStencilLayouts; }
    inline bool ISSUPPORTED_SWAPCHAINDISPLAY() { return SwapchainDisplay; }
    inline bool ISSUPPORTED_BUFFERDEVICEADRESS() { return isBufferDeviceAddressSupported; }

    //SUPPORTERS : Because all variables are private, these are to set them as true. All extensions are set as false by default.

    void SUPPORT_VARIABLEDESCCOUNT();
    void SUPPORT_SWAPCHAINDISPLAY();
    void SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS();
    void SUPPORT_BUFFERDEVICEADDRESS();

    //ACTIVATORS : These are the supported extensions that doesn't only relaxes the rules but also adds new rules and usages.
    void CheckDeviceExtensionProperties(GPU_VKOBJ* VKGPU);
};

//Each extension structure should contain its type as its first variable!
enum VKEXT_TYPES {
    VKEXT_ERROR = 0,
    VKEXT_DEPTHBOUNDS
};

struct VKEXT_DEPTHBOUNDS_STRUCT {
    VKEXT_TYPES type;
    VkBool32 DepthBoundsEnable = VK_FALSE; float DepthBoundsMin = FLT_MIN, DepthBoundsMax = FLT_MAX;
};