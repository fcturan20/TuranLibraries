#pragma once
#include "tgfx_forwarddeclarations.h"
#include "tgfx_structs.h"


typedef struct renderer_tgfx {
    //RenderGraph Functions

    //Returns Swapchain's Handle
    void (*Start_RenderGraphConstruction)();
    //If reconstruction fails, old RG is used
    unsigned char (*Finish_RenderGraphConstruction)(subdrawpass_tgfx_handle IMGUI_Subpass);
    //When you call this function, you have to create your material types and instances from scratch
    void (*Destroy_RenderGraph)();

    //SubDrawPassIDs is used to return created SubDrawPasses IDs and DrawPass ID
    //SubDrawPassIDs argument array's order is in the order of passed SubDrawPass_Description list
    //There is no passwaitdescription, because each subpass defines one
    result_tgfx(*Create_DrawPass)(subdrawpassdescription_tgfx_listhandle SubDrawPasses,
        rtslotset_tgfx_handle RTSLOTSET_ID, subdrawpass_tgfx_listhandle SubDrawPassHandles, drawpass_tgfx_handle* DPHandle);
    /*
    * There are 2 types of TransferPasses and each type only support its type of GFXContentManager command: Barrier, Copy
    * Barrier means passes waits for the given resources (also image usage changes may happen that may change layout of the image)
    And Barrier commands are also useful for creating resources that don't depend on CPU side data and GPU operations creates the data
    That means you generally should use Barrier TP to create RTs
    * Copy cmd copies resources CPU<->GPU, GPU<->GPU
    */
    result_tgfx (*Create_TransferPass)(subtransferpassdescription_tgfx_listhandle subTPdescs, subtransferpass_tgfx_listhandle subTPHandles, transferpass_tgfx_handle* TPHandle);
    result_tgfx (*Create_ComputePass)(subcomputepassdescription_tgfx_listhandle subCPdescs, subcomputepass_tgfx_listhandle subCPHandles, computepass_tgfx_handle * CPHandle);
    result_tgfx (*Create_WindowPass)(passwaitdescription_tgfx_listhandle WaitDescriptions, windowpass_tgfx_handle* WindowPassHandle);

    //Rendering Functions

    //If VertexBuffer_ID is nullptr, you have to fetch vertex data in shader (with uniform/storage buffers)
    //If IndexBuffer_ID is nullptr, draw is a NonIndexedDraw. If IndexBuffer_ID isn't nullptr, this is an IndexedDraw.
    //For IndexedDraws, Count is IndexCount to render. For NonIndexedDraws, Count is VertexCount to render.
    //You can set Count as 0. For IndexedDraws, this means all of index buffer will be used. For NonIndexedDraws, all of the vertex buffer will be used.
    //For IndexedDraws, VertexOffset is the added to the value gotten from index buffer. For NonIndexedDraws, it is the place to start reading from Vertex Buffer.
    //For IndexedDraws, IndexOffset is the place to start reading from Index Buffer
    void (*DrawDirect)(buffer_tgfx_handle VertexBuffer_ID, buffer_tgfx_handle IndexBuffer_ID, unsigned int Count, unsigned int VertexOffset,
        unsigned int FirstIndex, unsigned int InstanceCount, unsigned int FirstInstance, rasterpipelineinstance_tgfx_handle MaterialInstance_ID, 
        unsigned int DrawCallIndex, void* CallBufferSourceData, unsigned char CallBufferSourceCopySize, unsigned char CallBufferTargetOffset, subdrawpass_tgfx_handle SubDrawPass_ID);
    void (*SwapBuffers)(window_tgfx_handle WindowHandle, windowpass_tgfx_handle WindowPassHandle);
    void (*CopyBuffer_toBuffer)(subtransferpass_tgfx_handle TransferPassHandle, buffer_tgfx_handle SourceBuffer_Handle,
        buffer_tgfx_handle TargetBuffer_Handle, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size);
    //If TargetTexture_CopyXXX is 0, it's converted to size of the texture in that dimension
    //If your texture is a cubemap, you should use TargetCubeMapFace to specify which face you're copying to
    void (*CopyBuffer_toImage)(subtransferpass_tgfx_handle TransferPassHandle, buffer_tgfx_handle SourceBuffer_Handle, texture_tgfx_handle TextureHandle,
        unsigned int SourceBuffer_offset, boxregion_tgfx TargetTextureRegion, unsigned int TargetMipLevel,
        cubeface_tgfx TargetCubeMapFace);
    //This function copies CopySize amount of data from Source to Target
    //Which means texture channels and layouts should match for a bugless copy
    //If either (or both) of your texture is a cubemap, you should use xxxCubeMapFace argument to specify the face.
    void (*CopyImage_toImage)(subtransferpass_tgfx_handle TransferPassHandle, texture_tgfx_handle SourceTextureHandle, texture_tgfx_handle TargetTextureHandle,
        uvec3_tgfx SourceTextureOffset, uvec3_tgfx CopySize, uvec3_tgfx TargetTextureOffset, unsigned int SourceMipLevel, unsigned int TargetMipLevel,
        cubeface_tgfx SourceCubeMapFace, cubeface_tgfx TargetCubeMapFace);
    //If you don't want to target to a specific mip level (you want all mips to transition), you have to specify TargetMipLevel as UINT32_MAX
    //If your texture is a cubemap, you should use TargetCubeMapFace to specify the texture you're gonna use barrier for. Otherwise it's ignored.
    void (*ImageBarrier)(subtransferpass_tgfx_handle BarrierTPHandle, texture_tgfx_handle TextureHandle, image_access_tgfx LAST_ACCESS,
        image_access_tgfx NEXT_ACCESS, unsigned int TargetMipLevel, cubeface_tgfx TargetCubeMapFace);
    void (*ChangeDrawPass_RTSlotSet)(drawpass_tgfx_handle DrawPassHandle, rtslotset_tgfx_handle RTSlotSetHandle);
    void (*Dispatch_Compute)(subcomputepass_tgfx_handle SubComputePassHandle, computeshaderinstance_tgfx_handle CSInstanceHandle, uvec3_tgfx DispatchSize);


    unsigned char (*GetCurrentFrameIndex)();
    //Everything you call after this, will be proccessed for next frame!
    void (*Run)();
} renderer_tgfx;