#pragma once
#include "Editor/Editor_Includes.h"
#include "GFX/IMGUI/IMGUI_WINDOW.h"

namespace TuranEditor {
	class Main_Window : public GFX_API::IMGUI_WINDOW {
		unsigned int DisplayTexture = 0, DisplayWidth = 0, DisplayHeight = 0, Scene_View_Texture = 0;
	public:
		Main_Window();
		virtual void Run_Window();
	};

}