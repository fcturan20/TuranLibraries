#pragma once
#include "GFX_Includes.h"
#include "Renderer/GFX_Renderer.h"
#include "Renderer/GPU_ContentManager.h"
#include "IMGUI/IMGUI_Core.h"
#include "TuranAPI/ThreadedJobCore.h"


namespace GFX_API {
	//If you want to close GFX API systems, you should call Destroy GFX
	//Because "delete GFX;" may cause issues!
	class GFXAPI GFX_Core {
	protected:
		unsigned int FOCUSED_WINDOW_index;
		vector<GFXHandle> DEVICE_GPUs;
		GFXHandle GPU_TO_RENDER;
		vector<GFXHandle> WINDOWs;
		vector<GFXHandle> CONNECTED_Monitors;
		
		//This is to remind that each GFX API should return a monitor list at initialization
		GFX_Core(vector<MonitorDescription>& Monitors, vector<GPUDescription>& GPUs, TuranAPI::Threading::JobSystem* JobSys);
	public:
		virtual ~GFX_Core();

		static GFX_Core* SELF;
		Renderer* RENDERER;
		GPU_ContentManager* ContentManager;
		IMGUI_Core* IMGUI_o;
		TuranAPI::Threading::JobSystem* JobSys;

		//Window Operations

		//SwapchainTextureHandles should point to an array of 2 elements! Triple Buffering is not supported for now.
		virtual GFXHandle CreateWindow(const GFX_API::WindowDescription& Desc, GFX_API::GFXHandle* SwapchainTextureHandles, GFX_API::Texture_Properties& SwapchainTextureProperties) = 0;
		virtual void Change_Window_Resolution(GFXHandle WindowHandle, unsigned int width, unsigned int height) = 0;


		virtual void Take_Inputs() = 0;
		//Destroy GFX API systems to create again (with a different Graphics API maybe?) or close application
		//This will close all of the systems with "GFX" prefix and you shouldn't call them either
		virtual void Destroy_GFX_Resources() = 0;
		virtual void Check_Errors() = 0;
	};
}

#define GFX GFX_API::GFX_Core::SELF
#define GFXRENDERER GFX_API::GFX_Core::SELF->RENDERER
#define GFXContentManager GFX_API::GFX_Core::SELF->ContentManager
#define IMGUI GFX_API::GFX_Core::SELF->IMGUI_o

	//Window Manager functionality!
#define IMGUI_REGISTERWINDOW(WINDOW) GFX_API::GFX_Core::SELF->IMGUI->WindowManager->Register_WINDOW(WINDOW);
#define IMGUI_DELETEWINDOW(WINDOW) GFX_API::GFX_Core::SELF->IMGUI->WindowManager->Delete_WINDOW(WINDOW);
#define IMGUI_RUNWINDOWS() GFX_API::GFX_Core::SELF->IMGUI->WindowManager->Run_IMGUI_WINDOWs()