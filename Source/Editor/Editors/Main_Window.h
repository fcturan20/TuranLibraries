#pragma once
#include "Editor/Editor_Includes.h"
#include "GFX/IMGUI/IMGUI_WINDOW.h"

namespace TuranEditor {
	class Camera {
	public:
		Camera(vec3 Position = vec3(0, 0, 0), vec3 target = vec3(0, 0, 1));
		void Update(float yaw, float pitch);
		vec3 Position, Front_Vector, Right_Vector, Up_Vector, target;
		mat4 view_matrix;
		float cameraSpeed_Base = 1.0f;
	};


	class Main_Window : public GFX_API::IMGUI_WINDOW {
		Camera RTCamera;
		GFX_API::GFXHandle COPYTP, StagingBuffer, RTGlobalBuffer, DisplayTexture;
		vec3 LightDir, LightColor;
		unsigned int DisplayWidth = 0, DisplayHeight = 0;
		void* RTLightsData = nullptr, *RTCameraData = nullptr; 
		unsigned int RTLightsDataSize = 0, RTCameraDataSize = 0;
	public:
		Main_Window(GFX_API::GFXHandle CopyTP_Handle, GFX_API::GFXHandle StagingBufferHandle, GFX_API::GFXHandle RTGlobalBufferHandle, GFX_API::GFXHandle RT_GlobalTexture);
		virtual void Run_Window();
	};

}