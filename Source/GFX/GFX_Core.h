#pragma once
#include "GFX_Includes.h"
#include "GFX_Display.h"
#include "Renderer/GFX_Renderer.h"
#include "Renderer/GPU_ContentManager.h"
#include "IMGUI/IMGUI_Core.h"


namespace GFX_API {
	class GFXAPI GFX_Core {
	protected:
		unsigned int FOCUSED_WINDOW_index;
		vector<GPU*> DEVICE_GPUs;
		GFX_Core();

		virtual void Initialization() = 0;
		virtual void Check_Computer_Specs() = 0;
		virtual void Save_Monitors() = 0;
		virtual void Create_Renderer() = 0;

		MONITOR* Create_MonitorOBJ(void* monitor, const char* name);
	public:
		virtual ~GFX_Core();

		static GFX_Core* SELF;
		Renderer* RENDERER;
		GPU_ContentManager* ContentManager;
		IMGUI_Core* IMGUI_o;
		GPU* GPU_TO_RENDER;

		WINDOW* Main_Window;
		vector<MONITOR> CONNECTED_Monitors;

		//Window Operations

		virtual void Change_Window_Resolution(unsigned int width, unsigned int height) = 0;
		virtual void Swapbuffers_ofMainWindow() = 0;
		virtual void Show_RenderTarget_onWindow(unsigned int RenderTarget_GFXID) = 0;



		virtual void Take_Inputs() = 0;
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