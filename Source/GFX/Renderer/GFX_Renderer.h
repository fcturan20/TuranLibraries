#pragma once
#include "GFX/GFX_Includes.h"
#include "GFX_RenderNodes.h"
#include "GFX/GFX_FileSystem/Resource_Type/Material_Type_Resource.h"

namespace GFX_API {
	class GFXAPI Renderer {
	protected:
		//RenderGraph States
		bool Record_RenderGraphConstruction = false;
		unsigned char FrameIndex = 0, FrameCount = 2;
		void Set_NextFrameIndex();

		//Renderer States
		float LINEWIDTH = 1;

		//Render Node
	public:
		friend class GFX_Core;

		Renderer();
		~Renderer();
		Renderer* SELF;


		//RenderGraph Functions

		//Returns Swapchain's Handle
		virtual void Start_RenderGraphConstruction() = 0;
		virtual void Finish_RenderGraphConstruction() = 0;
		//SubDrawPassIDs is used to return created SubDrawPasses IDs and DrawPass ID
		//SubDrawPassIDs argument array's order is in the order of passed SubDrawPass_Description vector
		virtual TAPIResult Create_DrawPass(const vector<SubDrawPass_Description>& SubDrawPasses, GFXHandle RTSLOTSET_ID, const vector<GFX_API::PassWait_Description>& WAITs, const char* NAME, vector<GFXHandle>& SubDrawPassIDs, GFXHandle& DPHandle) = 0;
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
		virtual TAPIResult Create_TransferPass(const vector<PassWait_Description>& WaitDescriptions, const TRANFERPASS_TYPE& TP_TYPE, const string& NAME, GFX_API::GFXHandle& TPHandle) = 0;
		virtual TAPIResult Create_WindowPass(const vector<GFX_API::PassWait_Description>& WaitDescriptions, const string& NAME, GFX_API::GFXHandle& WindowPassHandle) = 0;

		//Rendering Functions

		//If VertexBuffer_ID is nullptr, you have to fetch vertex data in shader (with uniform/storage buffers)
		//If IndexBuffer_ID is nullptr, draw is a NonIndexedDraw. If IndexBuffer_ID isn't nullptr, this is an IndexedDraw.
		//For IndexedDraws, Count is IndexCount to render. For NonIndexedDraws, Count is VertexCount to render.
		//You can set Count as 0. For IndexedDraws, this means all of index buffer will be used. For NonIndexedDraws, all of the vertex buffer will be used.
		//For IndexedDraws, VertexOffset is the added to the value gotten from index buffer. For NonIndexedDraws, it is the place to start reading from Vertex Buffer.
		//For IndexedDraws, IndexOffset is the place to start reading from Index Buffer
		virtual void DrawNonInstancedDirect(GFX_API::GFXHandle VertexBuffer_ID, GFX_API::GFXHandle IndexBuffer_ID, unsigned int Count, unsigned int VertexOffset, 
			unsigned int FirstIndex, GFX_API::GFXHandle MaterialInstance_ID, GFX_API::GFXHandle SubDrawPass_ID) = 0;
		virtual void SwapBuffers(GFX_API::GFXHandle WindowHandle, GFX_API::GFXHandle WindowPassHandle) = 0;
		virtual void CopyBuffer_toBuffer(GFX_API::GFXHandle TransferPassHandle, GFX_API::GFXHandle SourceBuffer_Handle, GFX_API::BUFFER_TYPE SourceBufferTYPE, 
			GFX_API::GFXHandle TargetBuffer_Handle, GFX_API::BUFFER_TYPE TargetBufferTYPE, unsigned int SourceBuffer_Offset, unsigned int TargetBuffer_Offset, unsigned int Size) = 0;
		//If TargetTexture_CopyXXX is 0, it's converted to size of the texture in that dimension
		virtual void CopyBuffer_toImage(GFX_API::GFXHandle TransferPassHandle, GFX_API::GFXHandle SourceBuffer_Handle, GFX_API::BUFFER_TYPE SourceBufferTYPE,
			GFX_API::GFXHandle TextureHandle, unsigned int SourceBuffer_offset, GFX_API::BoxRegion TargetTextureRegion) = 0;
		//This function copies CopySize amount of data from Source to Target
		//Which means texture channels and layouts should match for a bugless copy
		virtual void CopyImage_toImage(GFX_API::GFXHandle TransferPassHandle, GFX_API::GFXHandle SourceTextureHandle, GFX_API::GFXHandle TargetTextureHandle,
			uvec3 SourceTextureOffset, uvec3 CopySize, uvec3 TargetTextureOffset) = 0;
		virtual void ImageBarrier(GFX_API::GFXHandle TextureHandle, const GFX_API::IMAGE_ACCESS& LAST_ACCESS
			, const GFX_API::IMAGE_ACCESS& NEXT_ACCESS, GFX_API::GFXHandle BarrierTPHandle) = 0;


		unsigned char GetCurrentFrameIndex();
		//Everything you call after this, will be proccessed for next frame!
		virtual void Run() = 0;
	};
}