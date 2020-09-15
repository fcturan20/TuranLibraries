#pragma once
#include "Editor/Editor_Includes.h"
#include "Editor/Editors/Main_Window.h"
#include "GFX/IMGUI/IMGUI_Core.h"
#include "GFX/GFX_Core.h"
#include "Editor/FileSystem/EditorFileSystem_Core.h"

#include "Editor/FileSystem/ResourceImporters/Model_Loader.h"
#include "Editor/RenderContext/Editor_DataManager.h"
#include "TuranAPI/Profiler_Core.h"

namespace TuranEditor {

	class Editor_System {
		TuranAPI::Logging::Logger LOGGING;
		Editor_FileSystem FileSystem;
		TuranAPI::Profiler_System Profiler;
		static bool Should_Close;
	public:
		Editor_System();
		~Editor_System();
		static void Take_Inputs();
		static bool Should_EditorClose();
	};


}