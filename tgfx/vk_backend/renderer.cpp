#include "renderer.h"
#include "predefinitions_vk.h"
#include <tgfx_structs.h>



typedef struct renderer_private{
	VkDescriptorPool IMGUIPOOL = VK_NULL_HANDLE;
} renderer_private;
static renderer_private* hidden = nullptr;

//RenderGraph operations
static unsigned char GetCurrentFrameIndex();
static void Start_RenderGraphConstruction();
static unsigned char Finish_RenderGraphConstruction(subdrawpass_tgfx_handle IMGUI_Subpass);
//This function is defined in the FGAlgorithm.cpp
static void Destroy_RenderGraph();
static result_tgfx Create_DrawPass(subdrawpassdescription_tgfx_listhandle SubDrawPasses, rtslotset_tgfx_handle RTSLOTSET_ID, passwaitdescription_tgfx_listhandle DrawPassWAITs,
    const char* NAME, subdrawpassaccess_tgfx* SubDrawPassHandles, drawpass_tgfx_handle* DPHandle);
static result_tgfx Create_TransferPass(passwaitdescription_tgfx_listhandle WaitDescriptions, transferpasstype_tgfx TP_TYPE, const char* NAME,
    transferpass_tgfx_handle* TPHandle);
static result_tgfx Create_ComputePass(passwaitdescription_tgfx_listhandle WaitDescriptions, unsigned int SubComputePassCount, const char* NAME, 
    computepass_tgfx_handle* CPHandle);
static result_tgfx Create_WindowPass(passwaitdescription_tgfx_listhandle WaitDescriptions, const char* NAME, windowpass_tgfx_handle* WindowPassHandle);
//RenderGraph has draw-compute-transfer calls to execute and display calls to display on windows
//So this function handles everything related to these calls
//If returns false, this means nothing is rendered this frame
static bool Execute_RenderGraph();

//Rendering operations
static void Run();
static void DrawDirect(buffer_tgfx_handle VertexBuffer_ID, buffer_tgfx_handle IndexBuffer_ID, unsigned int Count, unsigned int VertexOffset,
    unsigned int FirstIndex, unsigned int InstanceCount, unsigned int FirstInstance, rasterpipelineinstance_tgfx_handle MaterialInstance_ID, subdrawpass_tgfx_handle SubDrawPass_ID);
static void SwapBuffers(window_tgfx_handle WindowHandle, windowpass_tgfx_handle WindowPassHandle);
//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
//Target Buffer should be created with DEVICELOCAL
static void CopyBuffer_toBuffer(transferpass_tgfx_handle TransferPassHandle, buffer_tgfx_handle SourceBuffer_Handle, buffertype_tgfx SourceBufferTYPE,
    buffer_tgfx_handle TargetBuffer_Handle, buffertype_tgfx TargetBufferTYPE, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size);
//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
static void CopyBuffer_toImage(transferpass_tgfx_handle TransferPassHandle, buffer_tgfx_handle SourceBuffer_Handle, texture_tgfx_handle TextureHandle,
    unsigned int SourceBuffer_offset, boxregion_tgfx TargetTextureRegion, buffertype_tgfx SourceBufferTYPE, unsigned int TargetMipLevel,
    cubeface_tgfx TargetCubeMapFace);
static void CopyImage_toImage(transferpass_tgfx_handle TransferPassHandle, texture_tgfx_handle SourceTextureHandle, texture_tgfx_handle TargetTextureHandle,
    uvec3_tgfx SourceTextureOffset, uvec3_tgfx CopySize, uvec3_tgfx TargetTextureOffset, unsigned int SourceMipLevel, unsigned int TargetMipLevel,
    cubeface_tgfx SourceCubeMapFace, cubeface_tgfx TargetCubeMapFace);
static void ImageBarrier(transferpass_tgfx_handle BarrierTPHandle, texture_tgfx_handle TextureHandle, image_access_tgfx LAST_ACCESS,
    image_access_tgfx NEXT_ACCESS, unsigned int TargetMipLevel, cubeface_tgfx TargetCubeMapFace);
static void Dispatch_Compute(computepass_tgfx_handle ComputePassHandle, computeshaderinstance_tgfx_handle CSInstanceHandle,
    unsigned int SubComputePassIndex, uvec3_tgfx DispatchSize);
static void ChangeDrawPass_RTSlotSet(drawpass_tgfx_handle DrawPassHandle, rtslotset_tgfx_handle RTSlotSetHandle);


void Create_Renderer() {
    renderer = new renderer_public;
    hidden = new renderer_private;
}