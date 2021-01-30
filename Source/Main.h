#pragma once
#include "Editor/Editor_Includes.h"
#include "Editor/Editors/Main_Window.h"
#include "GFX/IMGUI/IMGUI_Core.h"
#include "GFX/GFX_Core.h"
#include "Editor/FileSystem/EditorFileSystem_Core.h"
#include "TuranAPI/ThreadedJobCore.h"

#include "Editor/FileSystem/ResourceImporters/Model_Loader.h"
#include "TuranAPI/Profiler_Core.h"

//These functions are defined in Editor_System.cpp in Editor folder
namespace TuranEditor {

	class Editor_System {
		TuranAPI::Logging::Logger LOGGING;
		Editor_FileSystem FileSystem;
		TuranAPI::Profiler_System Profiler;
		static bool Should_Close;
	public:
		vector<GFX_API::GPUDescription> GPUs;
		vector<GFX_API::MonitorDescription> Monitors;

		Editor_System(TuranAPI::Threading::JobSystem* JobSystem);
		~Editor_System();
		static void Take_Inputs();
		static bool Should_EditorClose();
	};

	void RenderGraphConstruction_BasicUT();
	void RenderGraphConstruction_LastFrameUT();
	void RenderGraphConstruction_DrawPassed(GFX_API::GFXHandle SWPCHT0, GFX_API::GFXHandle SWPCHT1, GFX_API::GFXHandle& SubpassID, GFX_API::GFXHandle& IRTSlotSetID, GFX_API::GFXHandle& WindowPassHandle
		, GFX_API::GFXHandle& FirstBarrierTPHandle, GFX_API::GFXHandle& UploadTPHandle, GFX_API::GFXHandle& FinalBarrierTPHandle);
}