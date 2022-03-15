#pragma once
#include "loading_libraries.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
	float fov_angle = 45.0f, aspect_ratio = 16.0/9.0, near_plane = 0.01f, far_plane = 1000.0f;
	glm::vec3 camerapos = { 0.0, 0.0, 10.0 }, camera_dir = { 0.0, 0.0, -1.0 };
public:
	glm::mat4x4 getPerspectiveMat4x4() { return glm::perspective(glm::radians(fov_angle), aspect_ratio, near_plane, far_plane); }
	glm::mat4x4 getWorld_to_ViewMat4x4(){ return glm::lookAt(camerapos, camerapos + glm::normalize(camera_dir), glm::vec3(0, 1, 0)); }
	vec3_tgfx getWorldPosition() { return vec3_tgfx{ camerapos.x, camerapos.y, camerapos.z }; }
	vec3_tgfx getWorldDirection() { return vec3_tgfx{ camera_dir.x, camera_dir.y, camera_dir.z }; }
	void imguiCameraEditor() {
		vec3_tgfx tgfx_camerapos = { camerapos.x, camerapos.y, camerapos.z }, tgfx_cameradir = { camera_dir.x, camera_dir.y, camera_dir.z };
		TGFXIMGUI->Slider_Vec3("Camera Position", &tgfx_camerapos, -10.0f, 10.0f);
		TGFXIMGUI->Slider_Vec3("Camera Direction", &tgfx_cameradir, -1.0, 1.0f);
		TGFXIMGUI->Slider_Float("FOV Angle", &fov_angle, 10.0, 150.0);
		camerapos.x = tgfx_camerapos.x; camerapos.y = tgfx_camerapos.y; camerapos.z = tgfx_camerapos.z;
		camera_dir.x = tgfx_cameradir.x; camera_dir.y = tgfx_cameradir.y; camera_dir.z = tgfx_cameradir.z;
		TGFXIMGUI->Slider_Float("Near Plane", &near_plane, FLT_MIN, 1.0f);
		TGFXIMGUI->Slider_Float("Far Plane", &far_plane, near_plane * 100.0, near_plane * 100000.0f);
	}

};