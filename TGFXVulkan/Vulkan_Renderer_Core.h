#pragma once
#include "Vulkan_Includes.h"
#include "Vulkan_Resource.h"

class Renderer {
public:
	std::vector<VK_DrawPass*> DrawPasses;
	std::vector<VK_TransferPass*> TransferPasses;
	std::vector<VK_WindowPass*> WindowPasses;
	std::vector<VK_ComputePass*> ComputePasses;
	VK_FrameGraph FrameGraphs[2];
	RenderGraphStatus RG_Status = RenderGraphStatus::Invalid;
	//These are RenderGraph algorithm only structures, so don't need any type or something
	void* Branches = nullptr, * Submits = nullptr, * Semaphores = nullptr;


	VkDescriptorPool IMGUIPOOL = VK_NULL_HANDLE;
	unsigned char FrameIndex;


	inline unsigned char Get_FrameIndex(bool is_LastFrame);

	//INHERITANCE
	Renderer();
	~Renderer();

	//RenderGraph operations
	static unsigned char GetCurrentFrameIndex();
	static void Start_RenderGraphConstruction();
	static unsigned char Finish_RenderGraphConstruction(tgfx_subdrawpass IMGUI_Subpass);
	//This function is defined in the FGAlgorithm.cpp
	static void Destroy_RenderGraph();
	static tgfx_result Create_DrawPass(tgfx_subdrawpassdescription_list SubDrawPasses, tgfx_rtslotset RTSLOTSET_ID, tgfx_passwaitdescription_list DrawPassWAITs,
		const char* NAME, tgfx_subdrawpass* SubDrawPassHandles, tgfx_drawpass* DPHandle);
	static tgfx_result Create_TransferPass(tgfx_passwaitdescription_list WaitDescriptions, tgfx_transferpass_type TP_TYPE, const char* NAME,
		tgfx_transferpass* TPHandle);
	static tgfx_result Create_ComputePass(tgfx_passwaitdescription_list WaitDescriptions, unsigned int SubComputePassCount, const char* NAME, 
		tgfx_computepass* CPHandle);
	static tgfx_result Create_WindowPass(tgfx_passwaitdescription_list WaitDescriptions, const char* NAME, tgfx_windowpass* WindowPassHandle);
	//RenderGraph has draw-compute-transfer calls to execute and display calls to display on windows
	//So this function handles everything related to these calls
	//If returns false, this means nothing is rendered this frame
	static bool Execute_RenderGraph();



	//Rendering operations
	static void Run();
	static void DrawDirect(tgfx_buffer VertexBuffer_ID, tgfx_buffer IndexBuffer_ID, unsigned int Count, unsigned int VertexOffset,
		unsigned int FirstIndex, unsigned int InstanceCount, unsigned int FirstInstance, tgfx_rasterpipelineinstance MaterialInstance_ID, tgfx_subdrawpass SubDrawPass_ID);
	static void SwapBuffers(tgfx_window WindowHandle, tgfx_windowpass WindowPassHandle);
	//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
	//Target Buffer should be created with DEVICELOCAL
	static void CopyBuffer_toBuffer(tgfx_transferpass TransferPassHandle, tgfx_buffer SourceBuffer_Handle, tgfx_buffertype SourceBufferTYPE,
		tgfx_buffer TargetBuffer_Handle, tgfx_buffertype TargetBufferTYPE, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size);
	//Source Buffer should be created with HOSTVISIBLE or FASTHOSTVISIBLE
	static void CopyBuffer_toImage(tgfx_transferpass TransferPassHandle, tgfx_buffer SourceBuffer_Handle, tgfx_texture TextureHandle,
		unsigned int SourceBuffer_offset, tgfx_BoxRegion TargetTextureRegion, tgfx_buffertype SourceBufferTYPE, unsigned int TargetMipLevel,
		tgfx_cubeface TargetCubeMapFace);
	static void CopyImage_toImage(tgfx_transferpass TransferPassHandle, tgfx_texture SourceTextureHandle, tgfx_texture TargetTextureHandle,
		tgfx_uvec3 SourceTextureOffset, tgfx_uvec3 CopySize, tgfx_uvec3 TargetTextureOffset, unsigned int SourceMipLevel, unsigned int TargetMipLevel,
		tgfx_cubeface SourceCubeMapFace, tgfx_cubeface TargetCubeMapFace);
	static void ImageBarrier(tgfx_transferpass BarrierTPHandle, tgfx_texture TextureHandle, tgfx_imageaccess LAST_ACCESS, 
		tgfx_imageaccess NEXT_ACCESS, unsigned int TargetMipLevel, tgfx_cubeface TargetCubeMapFace);
	static void Dispatch_Compute(tgfx_computepass ComputePassHandle, tgfx_computeshaderinstance CSInstanceHandle,
		unsigned int SubComputePassIndex, tgfx_uvec3 DispatchSize);
	static void ChangeDrawPass_RTSlotSet(tgfx_drawpass DrawPassHandle, tgfx_rtslotset RTSlotSetHandle);

};