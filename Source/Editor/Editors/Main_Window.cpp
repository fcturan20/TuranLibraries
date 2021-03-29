#include "Main_Window.h"

#include "GFX/GFX_Core.h"
#include "GFX/GFX_Core.h"

#include <string>

using namespace TuranAPI;

namespace TuranEditor {
	Camera::Camera(vec3 Position, vec3 target) : Position(Position), target(target) {
		Front_Vector = normalize(target - Position);
	}

	void Camera::Update(float yaw, float pitch) {
		//Camera Rotation
		vec3 world_up(0, 1, 0);

		//Camera Direction Update
		if (yaw != 0.0f || pitch != 0.0f) {
			target.x = cos(radians(pitch)) * cos(radians(yaw));
			target.y = sin(radians(pitch));
			target.z = cos(radians(pitch)) * sin(radians(yaw));
		}



		Front_Vector = normalize(target);
		Right_Vector = normalize(cross(world_up, Front_Vector));
		Up_Vector = normalize(cross(Front_Vector, Right_Vector));

		/*
		//Camera Position Update
		float camera_speed = 33.0 * cameraSpeed_Base;
		if (GFX->IsKey_Pressed(GFX_API::KEYBOARD_KEYs::KEYBOARD_W)) {
			Position += Front_Vector * camera_speed;
		}
		else if (GFX->IsKey_Pressed(GFX_API::KEYBOARD_KEYs::KEYBOARD_S)) {
			Position -= Front_Vector * camera_speed;
		}

		if (GFX->IsKey_Pressed(GFX_API::KEYBOARD_KEYs::KEYBOARD_D)) {
			Position -= Right_Vector * camera_speed;
		}
		else if (GFX->IsKey_Pressed(GFX_API::KEYBOARD_KEYs::KEYBOARD_A)) {
			Position += Right_Vector * camera_speed;
		}

		if (GFX->IsKey_Pressed(GFX_API::KEYBOARD_KEYs::KEYBOARD_NP_8)) {
			Position += Up_Vector * camera_speed;
		}
		else if (GFX->IsKey_Pressed(GFX_API::KEYBOARD_KEYs::KEYBOARD_NP_2)) {
			Position -= Up_Vector * camera_speed;
		}
		*/



		//View matrix construction
		view_matrix = lookAt(Position, Front_Vector + Position, world_up);
	}

	Main_Window::Main_Window(GFX_API::GFXHandle CopyTP_Handle, GFX_API::GFXHandle StagingBufferHandle, GFX_API::GFXHandle RTGlobalBufferHandle, GFX_API::GFXHandle RT_GlobalTexture) : IMGUI_WINDOW("Main_Window"),
		COPYTP(CopyTP_Handle), DisplayTexture(RT_GlobalTexture), StagingBuffer(StagingBufferHandle), RTGlobalBuffer(RTGlobalBufferHandle) {
		RTLightsData = new bool[32];	RTLightsDataSize = 32;
		RTCameraData = new bool[80];	RTCameraDataSize = 80;
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

		IMGUI->Slider_Vec3("Light Direction", &LightDir, -1.0f, 1.0f);
		IMGUI->Slider_Vec3("Light Color", &LightColor, -1.0f, 1.0f);
		vec2 MousePos = IMGUI->GetMouseWindowPos() - IMGUI->GetItemWindowPos();
		IMGUI->Display_Texture(DisplayTexture, 500, 500, false);
		/*
		if (MousePos.x >= 0.0f && MousePos.y >= 0.0f && 500 - MousePos.x > 0.0f && 500 - MousePos.y > 0.0f &&
			GFX->IsMouse_Clicked(GFX_API::MOUSE_BUTTONs::MOUSE_RIGHT_CLICK)) {
			if (!isMoving) {
				LastMousePos = MousePos;
				isMoving = true;
			}

			vec2 MouseOffset = MousePos - LastMousePos;
			MouseOffset.y = -MouseOffset.y;
			//Sensitivity
			MouseOffset *= 0.1f;
			Yaw_Pitch += MouseOffset;
			if (Yaw_Pitch.y > 89.0f) {
				Yaw_Pitch.y = 89.0f;
			}
			else if (Yaw_Pitch.y < -89.0f) {
				Yaw_Pitch.y = -89.0f;
			}

			LastMousePos = MousePos;
		}
		else {
			isMoving = false;
		}
		DrawPassCamera.Update(Yaw_Pitch.x, Yaw_Pitch.y);*/
		//Successfully created the window!
		Main_Menubar_of_Editor();

		//Upload GPU data
		{
			mat4 camera_to_world = inverse(RTCamera.view_matrix);
			memcpy((char*)RTCameraData + 16, &camera_to_world, 64);
			memcpy((char*)RTCameraData, &RTCamera.Position, 16);
			memcpy((char*)RTLightsData, &LightDir, 12);
			memcpy((char*)RTLightsData + 16, &LightColor, 12);
			if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, RTLightsData, RTLightsDataSize, 104) != TAPI_SUCCESS) {
				LOG_CRASHING_TAPI("RTLightsData Upload to buffer has failed!");
			}
			if (GFXContentManager->Upload_toBuffer(StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, RTCameraData, RTCameraDataSize, RTLightsDataSize + 104) != TAPI_SUCCESS) {
				LOG_CRASHING_TAPI("RTCameraData Upload to buffer has failed!");
			}

			GFXRENDERER->CopyBuffer_toBuffer(COPYTP, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, RTGlobalBuffer, GFX_API::BUFFER_TYPE::GLOBAL, 104, 0, RTLightsDataSize);
			GFXRENDERER->CopyBuffer_toBuffer(COPYTP, StagingBuffer, GFX_API::BUFFER_TYPE::STAGING, RTGlobalBuffer, GFX_API::BUFFER_TYPE::GLOBAL, 
				104 + RTLightsDataSize, RTLightsDataSize, RTCameraDataSize);
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