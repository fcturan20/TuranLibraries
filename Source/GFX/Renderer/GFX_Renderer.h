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
		unsigned char Get_LastFrameIndex();
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
		virtual GFXHandle Create_DrawPass(const vector<SubDrawPass_Description>& SubDrawPasses, GFXHandle RTSLOTSET_ID, const vector<GFX_API::PassWait_Description>& WAITs, GFXHandle* SubDrawPassIDs, const char* NAME) = 0;
		virtual GFXHandle Create_TransferPass(const TransferPass_Description& TransferPassDescription) = 0;
		//PassRunCondition is used to control the RenderGraph's rendering path
		//For example: You can disable all the world rendering related passes and just render UI
		//But you should give this handle to every pass that you want to disable (instead of the only one pass wait all other passes wait for)
		//Because for example: World Renderer Passes are using Swapchains so UI have to wait for World Renderer
		//When World Renderer is disabled, because UI depends on World Renderer (for Swapchain), GFX API would disable UI too!
		//If is_DEFAULTACTIVATED is true, passes depends on this condition will run by default
		//When you deactivate a PassRunCondition, it is deactivated until you activate it again!
		//virtual GFXHandle Create_PassRunCondition(bool is_DEFAULTACTIVATED) = 0;

		//Rendering Functions

		//Everything you call after this, will be proccessed for next frame!
		virtual void Run() = 0;
		virtual void Render_DrawCall(DrawCall_Description drawcall) = 0;
		//If should_ACTIVATE is true, condition is activated. Otherwise, it is deactivated.
		//virtual void Set_PassRunCondition(GFXHandle ConditionHandle, bool should_ACTIVATE) = 0;

		/*
			In a GL specific GFX API (Vulkan, OpenGL4 etc), you should specify functions like Find_DrawPass_byID to access draw passes on other systems
			You shouldn't define here, because that would any user to access these structures which isn't safe.
		*/
	};
}