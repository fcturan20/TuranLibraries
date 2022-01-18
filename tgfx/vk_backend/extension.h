#include "predefinitions_vk.h"



struct depthstencilslot_vk;
struct extension_manager {
private:
    //Swapchain
    bool SwapchainDisplay = false;

    //Seperated Depth Stencil
    bool SeperatedDepthStencilLayouts = false;

    //If GPU supports descriptor indexing, it is activated!
    bool isDescriptorIndexingSupported = false;
    //These limits are maximum count of defined resources in material linking (including global buffers and textures)
    //That means; at specified shader input type-> all global shader inputs + general shader inputs + per instance shader inputs shouldn't exceed the related limit
    unsigned int MaxDesc_SampledTexture = 0, MaxDesc_ImageTexture = 0, MaxDesc_UniformBuffer = 0, MaxDesc_StorageBuffer = 0, MaxDesc_ALL = 0, MaxAccessibleDesc_PerStage = 0;
public:
    extension_manager();
    void (*Fill_DepthAttachmentReference)(VkAttachmentReference& Ref, unsigned int index,
        texture_channels_tgfx channels, operationtype_tgfx DEPTHOPTYPE, operationtype_tgfx STENCILOPTYPE);
    void (*Fill_DepthAttachmentDescription)(VkAttachmentDescription& Desc, depthstencilslot_vk* DepthSlot);

    //GETTER-SETTERS
    inline bool ISSUPPORTED_DESCINDEXING() { return isDescriptorIndexingSupported; }
    inline unsigned int& GETMAXDESC(desctype_vk TYPE) { switch (TYPE) { case desctype_vk::IMAGE: return MaxDesc_ImageTexture; case desctype_vk::SAMPLER: return MaxDesc_SampledTexture; case desctype_vk::SBUFFER: return MaxDesc_StorageBuffer; case desctype_vk::UBUFFER: return MaxDesc_UniformBuffer; } }
    inline unsigned int& GETMAXDESCALL() { return MaxDesc_ALL; }
    inline unsigned int& GETMAXDESCPERSTAGE() { return MaxAccessibleDesc_PerStage; }
    inline bool ISSUPPORTED_SEPERATEDDEPTHSTENCILLAYOUTS() { return SeperatedDepthStencilLayouts; }
    inline bool ISSUPPORTED_SWAPCHAINDISPLAY() { return SwapchainDisplay; }

    //SUPPORTERS : Because all variables are private, these are to set them as true. All extensions are set as false by default.

    void SUPPORT_DESCINDEXING();
    void SUPPORT_SWAWPCHAINDISPLAY();
    void SUPPORT_SEPERATEDDEPTHSTENCILLAYOUTS();

    //ACTIVATORS : These are the supported extensions that doesn't only relaxes the rules but also adds new rules and usages.

};