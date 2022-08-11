//This example shows how to render a 3D cube

#include <iostream>
#include <string>
#include <windows.h>

#include "loading_libraries.h"
#include "loading_shaders.h"
#include "camera.h"


#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


char stop_char;

unsigned char unittest_func(const char** output_string, void* data){
	*output_string = "Unit test in registry_sys_example has run!";
	return 0;
}

void thread_print(){
	while(stop_char != '1'){
		printf("This thread index: %u\n", threadingsys->this_thread_index());
	}
	Sleep(2000);
	printf("Finished thread index: %u\n", threadingsys->this_thread_index());
}

//Camera datas
static vec3_tgfx objpos;
void FirstWindow_Run(imguiWindow_tgfx* windowdata) {
	if (!windowdata->isWindowOpen) { TGFXIMGUI->Delete_WINDOW(windowdata); return; }
	if (!TGFXIMGUI->Create_Window(windowdata->WindowName, &windowdata->isWindowOpen, false)) { TGFXIMGUI->End_Window(); }
	TGFXIMGUI->Slider_Vec3("Object position", &objpos, -10.0, 10.0);
	((Camera*)windowdata->userdata)->imguiCameraEditor();	//Change camera position etc. with sliders

	TGFXIMGUI->End_Window();
}
int main() {
	TURANLIBRARY_LOADER::Load_AllLibraries();

	uvec2_tgfx window_resolution = { 1280, 720 };
	TURANLIBRARY_LOADER::InitializeTGFX_CreateWindow(40 * 1024 * 1024, 20 * 1024 * 1024, window_resolution);
	
	//Create a global buffer for shader inputs
	shaderuniformsbuffer = nullptr;
	TGFXCONTENTMANAGER->Create_GlobalBuffer("UniformBuffers", 4096, fastvisiblememtype_id, (extension_tgfxlsthnd)&TGFXCORE->INVALIDHANDLE, &shaderuniformsbuffer);
	shaderStageFlag_tgfxhnd onlycompute_shaderstage = TGFXHELPER->CreateShaderStageFlag(1, shaderstage_tgfx_COMPUTESHADER);
	bindingTable_tgfxhnd firstbindingtable = nullptr;
	TGFXCONTENTMANAGER->Create_BindingTable(shaderdescriptortype_tgfx_BUFFER, 1, onlycompute_shaderstage, nullptr, &firstbindingtable);
	TGFXCONTENTMANAGER->SetBindingTable_Buffer(firstbindingtable, 0, shaderuniformsbuffer, 0, 4096, nullptr);

	//Create a depth buffer
	buffer_tgfxhnd texturestagingbuffer;
	textureUsageFlag_tgfxhnd renderonly_usageflag = TGFXHELPER->CreateTextureUsageFlag(false, false, true, false, false);
	TGFXCONTENTMANAGER->Create_Texture(texture_dimensions_tgfx_2D, 1280, 720, texture_channels_tgfx_D32, 1, renderonly_usageflag, textureOrder_tgfx_SWIZZLE, devicelocalmemtype_id, &depthbuffer_handle);

	TURANLIBRARY_LOADER::ReconstructRenderGraph_Simple();

	Camera maincam;

	//Create vertex attributelayout and buffer, then staging buffer to upload to and upload data
	datatype_tgfx vertexattribs_datatypes[3]{ datatype_tgfx_VAR_VEC3, datatype_tgfx_VAR_VEC2, datatype_tgfx_UNDEFINED };
	TGFXCONTENTMANAGER->Create_VertexAttributeLayout(vertexattribs_datatypes, vertexlisttypes_tgfx_TRIANGLELIST, &vertexattriblayout);
	std::vector<float> vertexbufferdata{ -1.0, -1.0, 0.0, 0.0, 0.0,
		1.0, -1.0, 0.0, 0.0, 1.0, 
		-1.0, 1.0, 0.0, 1.0, 0.0,
		1.0, 1.0, 0.0, 1.0, 1.0,
		-1.0, -1.0, -1.0, 0.0, 0.0,
		1.0, -1.0, -1.0, 0.0, 1.0,
		-1.0, 1.0, -1.0, 1.0, 0.0,
		1.0, 1.0, -1.0, 1.0, 1.0};
	std::vector<unsigned int> indexbufferdata{
		2, 6, 7,
		2, 3, 7,
		0, 4, 5,
		0, 1, 5,
		0, 2, 6,
		0, 4, 6,
		1, 3, 7,
		1, 5, 7,
		0, 2, 3,
		0, 1, 3,
		4, 6, 7,
		4, 5, 7 };
	buffer_tgfxhnd firstvertexbuffer, firstindexbuffer, firststagingbuffer;
	TGFXCONTENTMANAGER->Create_VertexBuffer(vertexattriblayout, 4, devicelocalmemtype_id, &firstvertexbuffer);
	TGFXCONTENTMANAGER->Create_IndexBuffer(datatype_tgfx_VAR_UINT32, 6, devicelocalmemtype_id, &firstindexbuffer);
	TGFXCONTENTMANAGER->Create_GlobalBuffer("FirstStagingBuffer", 6 * 100, fastvisiblememtype_id, nullptr, &firststagingbuffer);	//Allocate a little bit much
	TGFXCONTENTMANAGER->Upload_toBuffer(firststagingbuffer, vertexbufferdata.data(), vertexbufferdata.size() * sizeof(float), 0);
	TGFXCONTENTMANAGER->Upload_toBuffer(firststagingbuffer, indexbufferdata.data(), indexbufferdata.size() * sizeof(unsigned int), vertexbufferdata.size() * sizeof(float));
	TGFXRENDERER->CopyBuffer_toBuffer(framebegin_subTPs[0], firststagingbuffer, firstvertexbuffer, 0, 0, vertexbufferdata.size() * sizeof(float));
	TGFXRENDERER->CopyBuffer_toBuffer(framebegin_subTPs[0], firststagingbuffer, firstindexbuffer, vertexbufferdata.size() * sizeof(float), 0, indexbufferdata.size() * sizeof(unsigned int));

	//Create texture, staging buffer upload data to and upload data
	textureUsageFlag_tgfxhnd orange_textureusage = TGFXHELPER->CreateTextureUsageFlag(true, true, true, true, true);
	TGFXCONTENTMANAGER->Create_Texture(texture_dimensions_tgfx_2D, 1280, 720, texture_channels_tgfx_RGBA32F, 1, orange_textureusage, textureOrder_tgfx_SWIZZLE, devicelocalmemtype_id, &orange_texture);
	vec4_tgfx color; color.x = 1.0f; color.y = 0.5f; color.z = 0.2f; color.w = 1.0f;
	std::vector<vec4_tgfx> texture_data(1280 * 720, color);
	TGFXCONTENTMANAGER->Create_GlobalBuffer("Texture Staging Buffer", 1280 * 720 * 20, fastvisiblememtype_id, nullptr, &texturestagingbuffer);
	TGFXCONTENTMANAGER->Upload_toBuffer(texturestagingbuffer, texture_data.data(), texture_data.size() * 16, 0);

	//Upload light data
	glm::vec4 directionallightdata[2] = { normalize(glm::vec4(0.3, 0.4, 0.5, 1.0)), glm::vec4(0.7, 0.3, 0.5, 1.0) };
	TGFXCONTENTMANAGER->Upload_toBuffer(shaderuniformsbuffer, directionallightdata, 32, 0);
	struct raytrace_cameradata {
		glm::vec4 camera_worldpos = glm::vec4(-10.0, 0.0, 0.0, 1.0);
		glm::mat4 camera_to_world;
	};
	raytrace_cameradata rt_cameradata; rt_cameradata.camera_to_world = glm::inverse(glm::perspective(glm::radians(90.0), double(16.0 / 9.0f), 0.1, 100.0));
	TGFXCONTENTMANAGER->Upload_toBuffer(shaderuniformsbuffer, &rt_cameradata, sizeof(raytrace_cameradata), 64);


	static imguiWindow_tgfx first_dearimgui_window;
	first_dearimgui_window.isWindowOpen = true;
	first_dearimgui_window.WindowName = "First IMGUIWindow";
	first_dearimgui_window.RunWindow = &FirstWindow_Run;
	first_dearimgui_window.userdata = &maincam;
	TGFXIMGUI->Register_WINDOW(&first_dearimgui_window);
	SHADERS::LoadShaders();

	/*
	//All textures should be converted from no_access layout to something in the first frame
	//Frame 0
	TGFXRENDERER->ImageBarrier(barrierbeforecompute_TP, depthbuffer_handle, image_access_tgfx_NO_ACCESS, image_access_tgfx_DEPTHSTENCIL_READWRITE, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(barrierbeforecompute_TP, orange_texture, image_access_tgfx_NO_ACCESS, image_access_tgfx_TRANSFER_DIST, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(barrierbeforecompute_TP, swapchain_textures[0], image_access_tgfx_NO_ACCESS, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(barrierbeforecompute_TP, swapchain_textures[1], image_access_tgfx_NO_ACCESS, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(barrierbeforeswapchain_TP, swapchain_textures[0], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(barrierbeforeswapchain_TP, swapchain_textures[1], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
	TGFXCONTENTMANAGER->SetMaterialType_Texture(base_3dshadertype, false, 0, orange_texture, true, 0, linear1mip_repeatsampler, image_access_tgfx_SHADER_SAMPLEONLY);
	TGFXRENDERER->SwapBuffers(firstwindow, swapchain_windowpass);
	TGFXRENDERER->Run();

	//Frame 1
	boxregion_tgfx region;
	region.WIDTH = 1280; region.HEIGHT = 720; region.XOffset = 0; region.YOffset = 0;
	TGFXRENDERER->CopyBuffer_toImage(framebegincopy_TP, texturestagingbuffer, orange_texture, 0, region, buffertype_tgfx_STAGING, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->CopyBuffer_toBuffer(framebegincopy_TP, firststagingbuffer, buffertype_tgfx_STAGING, firstvertexbuffer, buffertype_tgfx_VERTEX, 0, 0, 6 * 4 * 4);
	TGFXRENDERER->ImageBarrier(barrierbeforecompute_TP, orange_texture, image_access_tgfx_TRANSFER_DIST, image_access_tgfx_SHADER_WRITEONLY, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(barrierbeforeswapchain_TP, orange_texture, image_access_tgfx_SHADER_WRITEONLY, image_access_tgfx_SHADER_SAMPLEONLY, 0, cubeface_tgfx_FRONT);
	uvec3_tgfx compute_dispatch; compute_dispatch.x = 512; compute_dispatch.y = 512; compute_dispatch.z = 1;
	TGFXRENDERER->Dispatch_Compute(ComputebeforeRasterization_CP, RT_computeshaderinstance, 0, compute_dispatch);
	TGFXRENDERER->ImageBarrier(barrierbeforecompute_TP, swapchain_textures[TGFXRENDERER->GetCurrentFrameIndex()], image_access_tgfx_SWAPCHAIN_DISPLAY, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(barrierbeforeswapchain_TP, swapchain_textures[TGFXRENDERER->GetCurrentFrameIndex()], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->SwapBuffers(firstwindow, swapchain_windowpass);
	TGFXRENDERER->Run();

	while (first_dearimgui_window.isWindowOpen) {
		profiledscope_handle_tapi scope;	unsigned long long duration;
		profilersys->start_profiling(&scope, "Run Loop", &duration, 1);

		TGFXIMGUI->Run_IMGUI_WINDOWs();
		//Rendering operations
		boxregion_tgfx region;
		region.WIDTH = 1280; region.HEIGHT = 720; region.XOffset = 0; region.YOffset = 0;
		TGFXRENDERER->DrawDirect(firstvertexbuffer, firstindexbuffer, 6 * 6, 0, 0, 1, 0, base_3dshaderinst, framerendering_subDP);
		
		glm::vec3 glm_objpos(objpos.x, objpos.y, objpos.z); vec3_tgfx camera_pos = maincam.getWorldPosition();
		glm::mat4x4 obj_to_world(1.0f); obj_to_world = glm::translate(obj_to_world, glm_objpos);
		glm::mat4x4 world_to_camera = maincam.getWorld_to_ViewMat4x4(); 
		glm::mat4x4 cameraproj = maincam.getPerspectiveMat4x4();
		glm::mat4x4 cameraproj_to_world = glm::inverse(world_to_camera * cameraproj);
		TGFXCONTENTMANAGER->Upload_toBuffer(shaderuniformsbuffer, buffertype_tgfx_GLOBAL, &obj_to_world, 64, 256);
		TGFXCONTENTMANAGER->Upload_toBuffer(shaderuniformsbuffer, buffertype_tgfx_GLOBAL, &world_to_camera, 64, 320);
		TGFXCONTENTMANAGER->Upload_toBuffer(shaderuniformsbuffer, buffertype_tgfx_GLOBAL, &cameraproj, 64, 448);
		TGFXCONTENTMANAGER->Upload_toBuffer(shaderuniformsbuffer, buffertype_tgfx_GLOBAL, &cameraproj_to_world, 64, 512);
		TGFXCONTENTMANAGER->Upload_toBuffer(shaderuniformsbuffer, buffertype_tgfx_GLOBAL, &camera_pos, 16, 576);
		

		//Swapchain operations
		TGFXRENDERER->ImageBarrier(barrierbeforecompute_TP, swapchain_textures[TGFXRENDERER->GetCurrentFrameIndex()], image_access_tgfx_SWAPCHAIN_DISPLAY, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
		TGFXRENDERER->ImageBarrier(barrierbeforeswapchain_TP, swapchain_textures[TGFXRENDERER->GetCurrentFrameIndex()], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
		TGFXRENDERER->SwapBuffers(firstwindow, swapchain_windowpass);

		TGFXRENDERER->Run();
		profilersys->finish_profiling(&scope, 1);
	}*/
	printf("Application is finished!");
	
	return 1;
}