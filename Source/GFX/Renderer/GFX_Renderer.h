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

		virtual void Render_DrawCall(GFX_API::GFXHandle VertexBuffer_ID, GFX_API::GFXHandle IndexBuffer_ID, GFX_API::GFXHandle MaterialInstance_ID, GFX_API::GFXHandle SubDrawPass_ID) = 0;
		virtual void SwapBuffers(GFX_API::GFXHandle WindowHandle, GFX_API::GFXHandle WindowPassHandle, const GFX_API::IMAGEUSAGE& PREVIOUS_IMUSAGE, const GFX_API::SHADERSTAGEs_FLAG& PREVIOUS_SHADERSTAGE) = 0;

		
		//Everything you call after this, will be proccessed for next frame!
		virtual void Run() = 0;
	};
}