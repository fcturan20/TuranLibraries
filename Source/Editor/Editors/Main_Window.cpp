#include "Main_Window.h"

#include "GFX/GFX_Core.h"
#include "GFX/GFX_Core.h"
#include "Editor/RenderContext/Editor_DataManager.h"

#include <string>

using namespace TuranAPI;

namespace TuranEditor {

	Main_Window::Main_Window() : IMGUI_WINDOW("Main_Window") {
		IMGUI_REGISTERWINDOW(this);
	}

	//Create main menubar for the Editor's main window!
	void Main_Menubar_of_Editor();

	void Main_Window::Run_Window() {
		if (!Is_Window_Open) {
			IMGUI->Is_IMGUI_Open = false;
			IMGUI_DELETEWINDOW(this);
			return;
		}
		if (!IMGUI->Create_Window("Main Window", Is_Window_Open, true)) {
			IMGUI->End_Window();
			return;
		}
		//Successfully created the window!
		Main_Menubar_of_Editor();

		if (IMGUI->Begin_TabBar()) {
			if (IMGUI->Begin_TabItem("Lights")) {
				IMGUI->Slider_Vec3("Directional Light Color", (vec3*)&Editor_RendererDataManager::DIRECTIONALLIGHTs[0].COLOR, 0, 1);
				IMGUI->Slider_Vec3("Directional Light Direction", (vec3*)&Editor_RendererDataManager::DIRECTIONALLIGHTs[0].DIRECTION, -1, 1);
				IMGUI->End_TabItem();
			}
			if (IMGUI->Begin_TabItem("Draw Pass")) {
				IMGUI->Slider_Vec3("Camera Position", &Editor_RendererDataManager::CameraPos, -1000, 1000);
				IMGUI->Slider_Vec3("Front Vector", &Editor_RendererDataManager::FrontVec, -1, 1);
				IMGUI->Slider_Vec3("First Object World Position", &Editor_RendererDataManager::FirstObjectPosition, -10, 10);
				IMGUI->End_TabItem();
			}
			if (IMGUI->Begin_TabItem("Compute Pass")) {
				DisplayWidth = 512; DisplayHeight = 512;
				IMGUI->End_TabItem();
			}
			if (DisplayTexture) {
				IMGUI->Display_Texture(DisplayTexture, DisplayWidth, DisplayHeight, true);
			}
		}


		IMGUI->End_Window();
	}
	
	void Main_Menubar_of_Editor() {
		if (!IMGUI->Begin_Menubar()) {
			IMGUI->End_Menubar();
		}
		//Successfully created the main menu bar!
		
		if (IMGUI->Begin_Menu("View")) {

			IMGUI->End_Menu();
		}


		//End Main Menubar operations!
		IMGUI->End_Menubar();
	}
}