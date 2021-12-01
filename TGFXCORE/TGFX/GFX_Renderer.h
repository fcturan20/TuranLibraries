#pragma once
#include "GFX_Includes.h"


#if defined(__cplusplus)
extern "C" {
#endif

	typedef struct tgfx_renderer {
		//RenderGraph Functions

		//Returns Swapchain's Handle
		void (*Start_RenderGraphConstruction)();
		unsigned char (*Finish_RenderGraphConstruction)(tgfx_subdrawpass IMGUI_Subpass);
		//When you call this function, you have to create your material types and instances from scratch
		void (*Destroy_RenderGraph)();

		//SubDrawPassIDs is used to return created SubDrawPasses IDs and DrawPass ID
		//SubDrawPassIDs argument array's order is in the order of passed SubDrawPass_Description vector
		tgfx_result(*Create_DrawPass)(tgfx_subdrawpassdescription_list SubDrawPasses,
			tgfx_rtslotset RTSLOTSET_ID, tgfx_passwaitdescription_list DrawPassWAITs,
			const char* NAME, tgfx_subdrawpass* SubDrawPassHandles, tgfx_drawpass* DPHandle);
		/*
		* There are 4 types of TransferPasses and each type only support its type of GFXContentManager command: Barrier, Copy, Upload, Download
		* Barrier means passes waits for the given resources (also image usage changes may happen that may change layout of the image)
		And Barrier commands are also useful for creating resources that don't depend on CPU side data and GPU operations creates the data
		That means you generally should use Barrier TP to create RTs
		* Upload cmd uploads resource from CPU to GPU
		* Download cmd downloads resource from GPU to CPU
		* Copy cmd copies resources between each other on GPU

		Upload TP:
		1) If your texture data comes from the CPU, you should create the texture in the pass that suits your upload timing
		For example: If you first create a Texture (without uploading any data) in UTP-1, you can not upload texture data in UTP-2.

		Download TP:
		1) Don't forget that you should carefully set IMUSAGEs of the Download cmds!
		2) If you're gonna delete the texture (Save the texture to CPU and delete), you can set LATER_IMUSAGE same with PREVIOUS_IMUSAGE so no barrier will be created

		Copy TP:
		1) Don't forget that you should carefully set IMUSAGEs of the Copy cmds!

		*/
		tgfx_result (*Create_TransferPass)(tgfx_passwaitdescription_list WaitDescriptions,
			tgfx_transferpass_type TP_TYPE, const char* NAME, tgfx_transferpass * TPHandle);
		tgfx_result (*Create_WindowPass)(tgfx_passwaitdescription_list WaitDescriptions, const char* NAME, tgfx_windowpass* WindowPassHandle);

		tgfx_result (*Create_ComputePass)(tgfx_passwaitdescription_list WaitDescriptions, unsigned int SubComputePassCount, const char* NAME,
			tgfx_computepass * CPHandle);

		//Rendering Functions

		//If VertexBuffer_ID is nullptr, you have to fetch vertex data in shader (with uniform/storage buffers)
		//If IndexBuffer_ID is nullptr, draw is a NonIndexedDraw. If IndexBuffer_ID isn't nullptr, this is an IndexedDraw.
		//For IndexedDraws, Count is IndexCount to render. For NonIndexedDraws, Count is VertexCount to render.
		//You can set Count as 0. For IndexedDraws, this means all of index buffer will be used. For NonIndexedDraws, all of the vertex buffer will be used.
		//For IndexedDraws, VertexOffset is the added to the value gotten from index buffer. For NonIndexedDraws, it is the place to start reading from Vertex Buffer.
		//For IndexedDraws, IndexOffset is the place to start reading from Index Buffer
		void (*DrawDirect)(tgfx_buffer VertexBuffer_ID, tgfx_buffer IndexBuffer_ID, unsigned int Count, unsigned int VertexOffset,
			unsigned int FirstIndex, unsigned int InstanceCount, unsigned int FirstInstance, tgfx_rasterpipelineinstance MaterialInstance_ID,
			tgfx_subdrawpass SubDrawPass_ID);
		void (*SwapBuffers)(tgfx_window WindowHandle, tgfx_windowpass WindowPassHandle);
		void (*CopyBuffer_toBuffer)(tgfx_transferpass TransferPassHandle, tgfx_buffer SourceBuffer_Handle,
			tgfx_buffertype SourceBufferTYPE,
			tgfx_buffer TargetBuffer_Handle, tgfx_buffertype TargetBufferTYPE, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size);
		//If TargetTexture_CopyXXX is 0, it's converted to size of the texture in that dimension
		//If your texture is a cubemap, you should use TargetCubeMapFace to specify which face you're copying to
		void (*CopyBuffer_toImage)(tgfx_transferpass TransferPassHandle, tgfx_buffer SourceBuffer_Handle, tgfx_texture TextureHandle,
			unsigned int SourceBuffer_offset, tgfx_BoxRegion TargetTextureRegion, tgfx_buffertype SourceBufferTYPE, unsigned int TargetMipLevel,
			tgfx_cubeface TargetCubeMapFace);
		//This function copies CopySize amount of data from Source to Target
		//Which means texture channels and layouts should match for a bugless copy
		//If either (or both) of your texture is a cubemap, you should use xxxCubeMapFace argument to specify the face.
		void (*CopyImage_toImage)(tgfx_transferpass TransferPassHandle, tgfx_texture SourceTextureHandle, tgfx_texture TargetTextureHandle,
			tgfx_uvec3 SourceTextureOffset, tgfx_uvec3 CopySize, tgfx_uvec3 TargetTextureOffset, unsigned int SourceMipLevel, unsigned int TargetMipLevel,
			tgfx_cubeface SourceCubeMapFace, tgfx_cubeface TargetCubeMapFace);
		//If you don't want to target to a specific mip level (you want all mips to transition), you don't have to specify TargetMipLevel
		//If your texture is a cubemap, you should use TargetCubeMapFace to specify the texture you're gonna use barrier for.
		void (*ImageBarrier)(tgfx_transferpass BarrierTPHandle, tgfx_texture TextureHandle, tgfx_imageaccess LAST_ACCESS,
			tgfx_imageaccess NEXT_ACCESS, unsigned int TargetMipLevel, tgfx_cubeface TargetCubeMapFace);
		void (*ChangeDrawPass_RTSlotSet)(tgfx_drawpass DrawPassHandle, tgfx_rtslotset RTSlotSetHandle);
		void (*Dispatch_Compute)(tgfx_computepass ComputePassHandle, tgfx_computeshaderinstance CSInstanceHandle,
			unsigned int SubComputePassIndex, tgfx_uvec3 DispatchSize);


		unsigned char (*GetCurrentFrameIndex)();
		//Everything you call after this, will be proccessed for next frame!
		void (*Run)();
	} tgfx_renderer;


#if defined(__cplusplus)
}
#endif