#include <iostream>
#include <string>
#include <windows.h>

#include "registrysys_tapi.h"
#include "virtualmemorysys_tapi.h"
#include "unittestsys_tapi.h"
#include "array_of_strings_tapi.h"
#include "threadingsys_tapi.h"
#include "filesys_tapi.h"
#include "logger_tapi.h"
#include "bitset_tapi.h"
#include "profiler_tapi.h"
#include "allocator_tapi.h"


#include "tgfx_forwarddeclarations.h"
#include "tgfx_helper.h"
#include "tgfx_core.h"
#include "tgfx_renderer.h"
#include "tgfx_gpucontentmanager.h"
#include "tgfx_imgui.h"

#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE threadingsys = NULL;
char stop_char;

unsigned char unittest_func(const char** output_string, void* data){
	*output_string = "Unit test in registry_sys_example has run!";
	return 0;
}

void thread_print(){
	while(stop_char != '1'){
		printf("This thread index: %u\n", threadingsys->funcs->this_thread_index());
	}
	Sleep(2000);
	printf("Finished thread index: %u\n", threadingsys->funcs->this_thread_index());
}
void printf_log_tgfx(result_tgfx result, const char* text) {
	printf("TGFX Result %u: %s\n", (unsigned int)result, text);
	if (result == result_tgfx_NOTCODED || result == result_tgfx_FAIL) {
		int i = 0;
		printf("TGFX RESULT ISN'T CODED HERE, PLEASE ENTER IF YOU WANT TO CONTINUE: ");
		scanf("%u", &i);
	}
}

core_tgfx* TGFXCORE = nullptr;
const void* TGFXINVALID = nullptr;
helper_tgfx* TGFXHELPER = nullptr;
renderer_tgfx* TGFXRENDERER = nullptr;
gpudatamanager_tgfx* TGFXCONTENTMANAGER = nullptr;
dearimgui_tgfx* TGFXIMGUI = nullptr;

void FirstWindow_Run() {
	static bool shouldShowText = false;
	if (TGFXIMGUI->Button("First Button")) { shouldShowText = !shouldShowText; }
	if (shouldShowText) { TGFXIMGUI->Text("This is the first text!"); }
}
int main() {
	auto registrydll = DLIB_LOAD_TAPI("tapi_registrysys.dll");
	if (registrydll == NULL) {
		printf("Registry System DLL load failed!");
	}
	load_plugin_func loader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(registrydll, "load_plugin");
	registrysys_tapi* sys = ((registrysys_tapi_type*)loader(NULL, 0))->funcs;
	const char* first_pluginname = sys->list(NULL)[0];
	virtualmemorysys_tapi* virmemsys = ((VIRTUALMEMORY_TAPI_PLUGIN_TYPE)sys->get("tapi_virtualmemsys", MAKE_PLUGIN_VERSION_TAPI(0, 0, 0), 0))->funcs;

	//Generally, I recommend you to load unit test system after registry system
	//Because other systems may implement unit tests but if unittestsys isn't loaded, they can't implement them.
	auto unittestdll = DLIB_LOAD_TAPI("tapi_unittest.dll");
	load_plugin_func unittestloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(unittestdll, "load_plugin");
	UNITTEST_TAPI_LOAD_TYPE unittest = (UNITTEST_TAPI_LOAD_TYPE)unittestloader(sys, 0);
	unittest_interface_tapi x;
	x.test = &unittest_func;
	unittest->add_unittest("naber", 0, x);

	//Create an array of strings in a 1GB of reserved virtual address space
	void* virmem_loc = virmemsys->virtual_reserve(1 << 30);
	auto aos_sys = DLIB_LOAD_TAPI("tapi_array_of_strings_sys.dll");
	load_plugin_func aosloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(aos_sys, "load_plugin");
	ARRAY_OF_STRINGS_TAPI_LOAD_TYPE aos = (ARRAY_OF_STRINGS_TAPI_LOAD_TYPE)aosloader(sys, 0);
	array_of_strings_tapi* first_array = aos->virtual_allocator->create_array_of_string(virmem_loc, 1 << 30, 0);
	aos->virtual_allocator->change_string(first_array, 0, "wassup?");
	printf("%s", aos->virtual_allocator->read_string(first_array, 0));
	unittest->run_tests(0xFFFFFFFF, NULL, NULL);

	auto threadingsysdll = DLIB_LOAD_TAPI("tapi_threadedjobsys.dll");
	load_plugin_func threadingloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(threadingsysdll, "load_plugin");
	threadingsys = (THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE)threadingloader(sys, 0);

	auto filesysdll = DLIB_LOAD_TAPI("tapi_filesys.dll");
	load_plugin_func filesysloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(filesysdll, "load_plugin");
	FILESYS_TAPI_PLUGIN_LOAD_TYPE filesys = (FILESYS_TAPI_PLUGIN_LOAD_TYPE)filesysloader(sys, 0);
	filesys->funcs->write_textfile("wassup?", "first.txt", false);

	auto loggerdll = DLIB_LOAD_TAPI("tapi_logger.dll");
	load_plugin_func loggerloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(loggerdll, "load_plugin");
	LOGGER_TAPI_PLUGIN_LOAD_TYPE loggersys = (LOGGER_TAPI_PLUGIN_LOAD_TYPE)loggerloader(sys, 0);
	loggersys->funcs->log_status("First log!");

	auto bitsetsysdll = DLIB_LOAD_TAPI("tapi_bitset.dll");
	load_plugin_func bitsetloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(bitsetsysdll, "load_plugin");
	BITSET_TAPI_PLUGIN_LOAD_TYPE bitsetsys = (BITSET_TAPI_PLUGIN_LOAD_TYPE)bitsetloader(sys, 0);
	bitset_tapi* firstbitset = bitsetsys->funcs->create_bitset(100);
	bitsetsys->funcs->setbit_true(firstbitset, 5);
	printf("First true bit: %u and bitset size: %u\n", bitsetsys->funcs->getindex_firsttrue(firstbitset), bitsetsys->funcs->getbyte_length(firstbitset));

	auto profilersysdll = DLIB_LOAD_TAPI("tapi_profiler.dll");
	load_plugin_func profilersysloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(profilersysdll, "load_plugin");
	PROFILER_TAPI_PLUGIN_LOAD_TYPE profilersys = (PROFILER_TAPI_PLUGIN_LOAD_TYPE)profilersysloader(sys, 0);


	auto tgfxdll = DLIB_LOAD_TAPI("tgfx_core.dll");
	load_plugin_func tgfxloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(tgfxdll, "load_plugin");
	TGFX_PLUGIN_LOAD_TYPE tgfxsys = (TGFX_PLUGIN_LOAD_TYPE)tgfxloader(sys, 0);
	result_tgfx result = tgfxsys->api->load_backend(backends_tgfx_VULKAN, printf_log_tgfx);
	printf("%c", result);

	TGFXCORE = tgfxsys->api;
	TGFXINVALID = tgfxsys->api->INVALIDHANDLE;
	TGFXHELPER = tgfxsys->api->helpers;
	TGFXRENDERER = tgfxsys->api->renderer;
	TGFXCONTENTMANAGER = tgfxsys->api->contentmanager;
	TGFXIMGUI = tgfxsys->api->imgui;

	gpu_tgfx_listhandle gpulist;
	tgfxsys->api->getGPUlist(&gpulist);
	TGFXLISTCOUNT(tgfxsys->api, gpulist, gpulist_count);
	printf("GPU COUNT: %u\n", gpulist_count);
	const char* gpuname;
	const memory_description_tgfx* memtypelist; unsigned int memtypelistcount;
	TGFXHELPER->GetGPUInfo_General(gpulist[0], &gpuname, nullptr, nullptr, nullptr, &memtypelist, &memtypelistcount, nullptr, nullptr, nullptr);
	std::cout << "Memory Type Count: " << memtypelistcount << " and GPU Name: " << gpuname << "\n";
	unsigned int devicelocalmemtype_id = UINT32_MAX, fastvisiblememtype_id = UINT32_MAX;
	for (unsigned int i = 0; i < memtypelistcount; i++) {
		if (memtypelist[i].allocationtype == memoryallocationtype_DEVICELOCAL) {
			TGFXHELPER->SetMemoryTypeInfo(memtypelist[i].memorytype_id, 40 * 1024 * 1024, nullptr);
			devicelocalmemtype_id = memtypelist[i].memorytype_id;
		}
		if (memtypelist[i].allocationtype == memoryallocationtype_FASTHOSTVISIBLE) {
			TGFXHELPER->SetMemoryTypeInfo(memtypelist[i].memorytype_id, 20 * 1024 * 1024, nullptr);
			fastvisiblememtype_id = memtypelist[i].memorytype_id;
		}
	}
	initializationsecondstageinfo_tgfx_handle secondinfo = TGFXHELPER->Create_GFXInitializationSecondStageInfo(gpulist[0], 10, 100, 100, 100, 100, 100, 100, 100, 100, false, true, true, (extension_tgfx_listhandle)TGFXINVALID);
	tgfxsys->api->initialize_secondstage(secondinfo);
	monitor_tgfx_listhandle monitorlist;
	tgfxsys->api->getmonitorlist(&monitorlist);
	TGFXLISTCOUNT(tgfxsys->api, monitorlist, monitorcount);
	printf_log_tgfx(result_tgfx_SUCCESS, (std::string("Monitor Count: ") + std::to_string(monitorcount)).c_str());
	textureusageflag_tgfx_handle swapchaintexture_usageflag = TGFXHELPER->CreateTextureUsageFlag(false, true, true, true, false);
	texture_tgfx_handle swapchain_textures[2];
	window_tgfx_handle firstwindow;
	tgfxsys->api->create_window(1280, 720, monitorlist[0], windowmode_tgfx_WINDOWED, "RegistrySys Example", swapchaintexture_usageflag, nullptr, nullptr, swapchain_textures, &firstwindow);


	//Create a global buffer for shader inputs
	buffer_tgfx_handle shaderuniformbuffers_handle = nullptr;
	TGFXCONTENTMANAGER->Create_GlobalBuffer("UniformBuffers", 1024, true, fastvisiblememtype_id, &shaderuniformbuffers_handle);

	texture_tgfx_handle first_depthbuffer; buffer_tgfx_handle texturestagingbuffer;
	textureusageflag_tgfx_handle depthbuffer_usageflag = TGFXHELPER->CreateTextureUsageFlag(false, false, true, false, false);
	TGFXCONTENTMANAGER->Create_Texture(texture_dimensions_tgfx_2D, 1280, 720, texture_channels_tgfx_D32, 1, depthbuffer_usageflag, texture_order_tgfx_SWIZZLE, devicelocalmemtype_id, &first_depthbuffer);

	TGFXRENDERER->Start_RenderGraphConstruction();


	//First Copy TP Creation
	transferpass_tgfx_handle first_copytp;
	TGFXRENDERER->Create_TransferPass((passwaitdescription_tgfx_listhandle)&TGFXINVALID, transferpasstype_tgfx_COPY, "FirstCopyTP", &first_copytp);


	//First Barrier TP Creation
	passwaitdescription_tgfx_handle wait_for_copytp[2] = { TGFXHELPER->CreatePassWait_TransferPass(&first_copytp, transferpasstype_tgfx_COPY, TGFXHELPER->CreateWaitSignal_Transfer(true, true), false), (passwaitdescription_tgfx_handle)TGFXINVALID };
	transferpass_tgfx_handle firstbarriertp;
	TGFXRENDERER->Create_TransferPass(wait_for_copytp, transferpasstype_tgfx_BARRIER, "FirstBarrierTP", &firstbarriertp);

	//First Compute Pass Creation
	computepass_tgfx_handle first_computepass;
	passwaitdescription_tgfx_handle dp_waits[2] = { TGFXHELPER->CreatePassWait_TransferPass(&firstbarriertp, transferpasstype_tgfx_BARRIER,
		TGFXHELPER->CreateWaitSignal_Transfer(true, true), false), (passwaitdescription_tgfx_handle)TGFXINVALID };
	TGFXRENDERER->Create_ComputePass(dp_waits, 1, "FirstCP", &first_computepass);

	//Draw Pass Creation

	vec4_tgfx white; white.x = 255; white.y = 255; white.z = 255; white.w = 0;
	rtslotdescription_tgfx_handle rtslot_descs[3] = {
		TGFXHELPER->CreateRTSlotDescription_Color(swapchain_textures[0], swapchain_textures[1], operationtype_tgfx_READ_AND_WRITE, drawpassload_tgfx_CLEAR, true, 0, white), 
		TGFXHELPER->CreateRTSlotDescription_DepthStencil(first_depthbuffer, first_depthbuffer, operationtype_tgfx_READ_AND_WRITE, drawpassload_tgfx_CLEAR, operationtype_tgfx_UNUSED, drawpassload_tgfx_CLEAR, 1.0, 255), (rtslotdescription_tgfx_handle)TGFXINVALID};
	rtslotset_tgfx_handle rtslotset_handle;
	TGFXCONTENTMANAGER->Create_RTSlotset(rtslot_descs, &rtslotset_handle);
	rtslotusage_tgfx_handle rtslotusages[3] = { TGFXHELPER->CreateRTSlotUsage_Color(rtslot_descs[0], operationtype_tgfx_READ_AND_WRITE, drawpassload_tgfx_CLEAR), 
		TGFXHELPER->CreateRTSlotUsage_Depth(rtslot_descs[1], operationtype_tgfx_READ_AND_WRITE, drawpassload_tgfx_CLEAR, operationtype_tgfx_UNUSED, drawpassload_tgfx_CLEAR), (rtslotusage_tgfx_handle)TGFXINVALID};
	inheritedrtslotset_tgfx_handle irtslotset;
	TGFXCONTENTMANAGER->Inherite_RTSlotSet(rtslotusages, rtslotset_handle, &irtslotset);
	subdrawpassdescription_tgfx_handle subdp_descs[2] = { TGFXHELPER->CreateSubDrawPassDescription(irtslotset, subdrawpassaccess_tgfx_ALLCOMMANDS, subdrawpassaccess_tgfx_ALLCOMMANDS), (subdrawpassdescription_tgfx_handle)TGFXINVALID };
	passwaitdescription_tgfx_handle cp_waits[2] = { TGFXHELPER->CreatePassWait_ComputePass(&first_computepass, 0,
		TGFXHELPER->CreateWaitSignal_Transfer(true, true), false), (passwaitdescription_tgfx_handle)TGFXINVALID };
	subdrawpass_tgfx_listhandle sub_dps;
	drawpass_tgfx_handle dp;
	TGFXRENDERER->Create_DrawPass(subdp_descs, rtslotset_handle, cp_waits, "First DP", &sub_dps, &dp);

	//Final Barrier TP Creation
	waitsignaldescription_tgfx_handle waitsignaldescs[2] = { TGFXHELPER->CreateWaitSignal_FragmentShader(true, true, true), (waitsignaldescription_tgfx_handle)TGFXINVALID };
	passwaitdescription_tgfx_handle final_tp_waits[2] = { TGFXHELPER->CreatePassWait_DrawPass(&dp, 0, waitsignaldescs, false), (passwaitdescription_tgfx_handle)TGFXINVALID };
	transferpass_tgfx_handle final_tp;
	TGFXRENDERER->Create_TransferPass(final_tp_waits, transferpasstype_tgfx_BARRIER, "Final TP", &final_tp);

	//Window Pass Creation
	windowpass_tgfx_handle first_wp;
	waitsignaldescription_tgfx_handle wait_for_finaltp_signal = TGFXHELPER->CreateWaitSignal_Transfer(true, true);
	passwaitdescription_tgfx_handle wait_for_dp[2] = { TGFXHELPER->CreatePassWait_TransferPass(&final_tp, transferpasstype_tgfx_BARRIER, wait_for_finaltp_signal, false), (passwaitdescription_tgfx_handle)TGFXINVALID };
	TGFXRENDERER->Create_WindowPass(wait_for_dp, "First WP", &first_wp);
	TGFXRENDERER->Finish_RenderGraphConstruction(sub_dps[0]);


	datatype_tgfx vertexattribs_datatypes[3]{ datatype_tgfx_VAR_VEC3, datatype_tgfx_VAR_VEC2, datatype_tgfx_UNDEFINED };
	vertexattributelayout_tgfx_handle vertexattriblayout;
	TGFXCONTENTMANAGER->Create_VertexAttributeLayout(vertexattribs_datatypes, vertexlisttypes_tgfx_TRIANGLELIST, &vertexattriblayout);

	//Create vertex buffer, staging buffer to upload to and upload data

	float vertexbufferdata[]{ -1.0, -1.0, 0.0, 0.0, 0.0,
		1.0, -1.0, 0.0, 0.0, 1.0,
		-1.0, 1.0, 0.0, 1.0, 0.0,
		1.0, 1.0, 0.0, 1.0, 1.0,
		-1.0, -1.0, -1.0, 0.0, 0.0,
		1.0, -1.0, -1.0, 0.0, 1.0,
		-1.0, 1.0, -1.0, 1.0, 0.0,
		1.0, 1.0, -1.0, 1.0, 1.0};
	unsigned int indexbufferdata[]{
		//Top
		2, 6, 7,
		2, 3, 7,

		//Bottom
		0, 4, 5,
		0, 1, 5,

		//Left
		0, 2, 6,
		0, 4, 6,

		//Right
		1, 3, 7,
		1, 5, 7,

		//Front
		0, 2, 3,
		0, 1, 3,

		//Back
		4, 6, 7,
		4, 5, 7 };
	buffer_tgfx_handle firstvertexbuffer, firstindexbuffer, firststagingbuffer;
	TGFXCONTENTMANAGER->Create_VertexBuffer(vertexattriblayout, 4, devicelocalmemtype_id, &firstvertexbuffer);
	TGFXCONTENTMANAGER->Create_IndexBuffer(datatype_tgfx_VAR_UINT32, 6, devicelocalmemtype_id, &firstindexbuffer);
	TGFXCONTENTMANAGER->Create_StagingBuffer(6 * 100, fastvisiblememtype_id, &firststagingbuffer);	//Allocate a little bit much
	TGFXCONTENTMANAGER->Upload_toBuffer(firststagingbuffer, buffertype_tgfx_STAGING, vertexbufferdata, 8 * 5 * 4, 0);
	TGFXCONTENTMANAGER->Upload_toBuffer(firststagingbuffer, buffertype_tgfx_STAGING, indexbufferdata, 6 * 6 * 4, 8 * 5 * 4);
	TGFXRENDERER->CopyBuffer_toBuffer(first_copytp, firststagingbuffer, buffertype_tgfx_STAGING, firstvertexbuffer, buffertype_tgfx_VERTEX, 0, 0, 8 * 5 * 4);
	TGFXRENDERER->CopyBuffer_toBuffer(first_copytp, firststagingbuffer, buffertype_tgfx_STAGING, firstindexbuffer, buffertype_tgfx_INDEX, 8 * 5 * 4, 0, 6 * 6 * 4);

	//Create texture, staging buffer upload data to and upload data
	texture_tgfx_handle first_texture; 
	textureusageflag_tgfx_handle first_textureusage = TGFXHELPER->CreateTextureUsageFlag(true, true, true, true, true);
	TGFXCONTENTMANAGER->Create_Texture(texture_dimensions_tgfx_2D, 1280, 720, texture_channels_tgfx_RGBA32F, 1, first_textureusage, texture_order_tgfx_SWIZZLE, devicelocalmemtype_id, &first_texture);
	vec4_tgfx color; color.x = 1.0f; color.y = 0.5f; color.z = 0.2f; color.w = 1.0f;
	std::vector<vec4_tgfx> texture_data(1280 * 720, color);
	TGFXCONTENTMANAGER->Create_StagingBuffer(1280 * 720 * 20, fastvisiblememtype_id, &texturestagingbuffer);
	TGFXCONTENTMANAGER->Upload_toBuffer(texturestagingbuffer, buffertype_tgfx_STAGING, texture_data.data(), texture_data.size() * 16, 0);

	//Upload light data
	glm::vec4 directionallightdata[2] = { normalize(glm::vec4(0.3, 0.4, 0.5, 1.0)), glm::vec4(0.7, 0.3, 0.5, 1.0) };
	TGFXCONTENTMANAGER->Upload_toBuffer(shaderuniformbuffers_handle, buffertype_tgfx_GLOBAL, directionallightdata, 32, 0);
	struct raytrace_cameradata {
		glm::vec4 camera_worldpos = glm::vec4(-10.0, 0.0, 0.0, 1.0);
		glm::mat4 camera_to_world;
	};
	raytrace_cameradata rt_cameradata; rt_cameradata.camera_to_world = glm::perspective(glm::radians(90.0), double(16.0 / 9.0f), 0.1, 100.0);
	TGFXCONTENTMANAGER->Upload_toBuffer(shaderuniformbuffers_handle, buffertype_tgfx_GLOBAL, &rt_cameradata, sizeof(raytrace_cameradata), 64);


	void* vert_text = filesys->funcs->read_textfile(CMAKE_SOURCE_FOLDER"/Example/FirstShader.vert"); if (!vert_text) { printf("Vertex Shader read has failed!"); }
	void* frag_text = filesys->funcs->read_textfile(CMAKE_SOURCE_FOLDER"/Example/FirstShader.frag"); if (!frag_text) { printf("Fragment Shader read has failed!"); }
	shadersource_tgfx_handle compiled_sources[3];
	TGFXCONTENTMANAGER->Compile_ShaderSource(shaderlanguages_tgfx_GLSL, shaderstage_tgfx_VERTEXSHADER, vert_text, 0, &compiled_sources[0]);
	TGFXCONTENTMANAGER->Compile_ShaderSource(shaderlanguages_tgfx_GLSL, shaderstage_tgfx_FRAGMENTSHADER, frag_text, 0, &compiled_sources[1]);
	compiled_sources[2] = (shadersource_tgfx_handle)TGFXINVALID;
	rasterpipelinetype_tgfx_handle first_material;
	shaderinputdescription_tgfx_handle inputdescs[2] = { TGFXHELPER->CreateShaderInputDescription(shaderinputtype_tgfx_SAMPLER_G, 0, 1, 
		TGFXHELPER->CreateShaderStageFlag(1, shaderstage_tgfx_FRAGMENTSHADER)), (shaderinputdescription_tgfx_handle)TGFXINVALID };
	extension_tgfx_handle depthbounds_ext[2] = { TGFXHELPER->EXT_DepthBoundsInfo(0.0, 1.0), (extension_tgfx_handle)TGFXINVALID };
	TGFXHELPER->CreateDepthConfiguration(true, depthtest_tgfx_ALWAYS, depthbounds_ext);
	TGFXCONTENTMANAGER->Link_MaterialType(compiled_sources, inputdescs, vertexattriblayout, sub_dps[0], cullmode_tgfx_OFF, 
		polygonmode_tgfx_FILL, nullptr, nullptr, nullptr, (blendinginfo_tgfx_listhandle)&TGFXINVALID, &first_material);
	rasterpipelineinstance_tgfx_handle first_matinst;
	TGFXCONTENTMANAGER->Create_MaterialInst(first_material, &first_matinst);
	samplingtype_tgfx_handle first_samplingtype;
	TGFXCONTENTMANAGER->Create_SamplingType(0, 1, texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP, texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP, texture_wrapping_tgfx_REPEAT, texture_wrapping_tgfx_REPEAT, texture_wrapping_tgfx_REPEAT, &first_samplingtype);
	TGFXCONTENTMANAGER->SetGlobalShaderInput_Texture(true, 0, false, first_texture, first_samplingtype, image_access_tgfx_SHADER_SAMPLEONLY);

	const char* comp_text = filesys->funcs->read_textfile(CMAKE_SOURCE_FOLDER"/Example/FirstComputeShader.comp");
	shadersource_tgfx_handle compiled_computeshadersource = nullptr;
	TGFXCONTENTMANAGER->Compile_ShaderSource(shaderlanguages_tgfx_GLSL, shaderstage_tgfx_COMPUTESHADER, (void*)comp_text, 0, &compiled_computeshadersource);
	computeshadertype_tgfx_handle first_computetype = nullptr;
	shaderinputdescription_tgfx_handle first_compute_inputs[4] = { TGFXHELPER->CreateShaderInputDescription(shaderinputtype_tgfx_UBUFFER_G, 0, 1, TGFXHELPER->CreateShaderStageFlag(1, shaderstage_tgfx_COMPUTESHADER)), 
		TGFXHELPER->CreateShaderInputDescription(shaderinputtype_tgfx_UBUFFER_G, 1, 1, TGFXHELPER->CreateShaderStageFlag(1, shaderstage_tgfx_COMPUTESHADER)), 
		TGFXHELPER->CreateShaderInputDescription(shaderinputtype_tgfx_IMAGE_G, 2, 1, TGFXHELPER->CreateShaderStageFlag(1, shaderstage_tgfx_COMPUTESHADER)), (shaderinputdescription_tgfx_handle)TGFXINVALID};
	TGFXCONTENTMANAGER->Create_ComputeType(compiled_computeshadersource, first_compute_inputs, &first_computetype);
	computeshaderinstance_tgfx_handle first_computeinstance = nullptr;
	TGFXCONTENTMANAGER->Create_ComputeInstance(first_computetype, &first_computeinstance);
	TGFXCONTENTMANAGER->SetComputeType_Buffer(first_computetype, false, 0, shaderuniformbuffers_handle, buffertype_tgfx_GLOBAL, true, 0, 0, 32);
	TGFXCONTENTMANAGER->SetComputeType_Buffer(first_computetype, false, 1, shaderuniformbuffers_handle, buffertype_tgfx_GLOBAL, true, 0, 64, 142);
	TGFXCONTENTMANAGER->SetComputeType_Texture(first_computetype, false, 2, first_texture, false, 0, nullptr, image_access_tgfx_SHADER_WRITEONLY);

	static imguiwindow_tgfx window_struct;
	window_struct.isWindowOpen = true;
	window_struct.WindowName = "First IMGUIWindow";
	window_struct.RunWindow = &FirstWindow_Run;
	TGFXIMGUI->Register_WINDOW(&window_struct);

	//Swapchain images should be converted from no_access layout in the first frame
	//Frame 0
	TGFXRENDERER->ImageBarrier(firstbarriertp, first_depthbuffer, image_access_tgfx_NO_ACCESS, image_access_tgfx_DEPTHSTENCIL_READWRITE, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(firstbarriertp, first_texture, image_access_tgfx_NO_ACCESS, image_access_tgfx_TRANSFER_DIST, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(firstbarriertp, swapchain_textures[0], image_access_tgfx_NO_ACCESS, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(firstbarriertp, swapchain_textures[1], image_access_tgfx_NO_ACCESS, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(final_tp, swapchain_textures[0], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(final_tp, swapchain_textures[1], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
	TGFXCONTENTMANAGER->SetMaterialType_Texture(first_material, false, 0, first_texture, true, 0, first_samplingtype, image_access_tgfx_SHADER_SAMPLEONLY);
	TGFXRENDERER->SwapBuffers(firstwindow, first_wp);
	TGFXRENDERER->Run();

	//Frame 1

	boxregion_tgfx region;
	region.WIDTH = 1280; region.HEIGHT = 720; region.XOffset = 0; region.YOffset = 0;
	TGFXRENDERER->CopyBuffer_toImage(first_copytp, texturestagingbuffer, first_texture, 0, region, buffertype_tgfx_STAGING, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->CopyBuffer_toBuffer(first_copytp, firststagingbuffer, buffertype_tgfx_STAGING, firstvertexbuffer, buffertype_tgfx_VERTEX, 0, 0, 6 * 4 * 4);
	TGFXRENDERER->ImageBarrier(firstbarriertp, first_texture, image_access_tgfx_TRANSFER_DIST, image_access_tgfx_SHADER_WRITEONLY, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(final_tp, first_texture, image_access_tgfx_SHADER_WRITEONLY, image_access_tgfx_SHADER_SAMPLEONLY, 0, cubeface_tgfx_FRONT);
	uvec3_tgfx compute_dispatch; compute_dispatch.x = 512; compute_dispatch.y = 512; compute_dispatch.z = 1;
	TGFXRENDERER->Dispatch_Compute(first_computepass, first_computeinstance, 0, compute_dispatch);

	//Swapchain operations
	TGFXRENDERER->ImageBarrier(firstbarriertp, swapchain_textures[TGFXRENDERER->GetCurrentFrameIndex()], image_access_tgfx_SWAPCHAIN_DISPLAY, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->ImageBarrier(final_tp, swapchain_textures[TGFXRENDERER->GetCurrentFrameIndex()], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
	TGFXRENDERER->SwapBuffers(firstwindow, first_wp);
	TGFXRENDERER->Run();

	while (true) {
		profiledscope_handle_tapi scope;	unsigned long long duration;
		profilersys->funcs->start_profiling(&scope, "Run Loop", &duration, 1);

		TGFXIMGUI->Run_IMGUI_WINDOWs();
		//Rendering operations
		boxregion_tgfx region;
		region.WIDTH = 1280; region.HEIGHT = 720; region.XOffset = 0; region.YOffset = 0;
		TGFXRENDERER->DrawDirect(firstvertexbuffer, firstindexbuffer, 6 * 6, 0, 0, 1, 0, first_matinst, sub_dps[0]);

		//Swapchain operations
		TGFXRENDERER->ImageBarrier(firstbarriertp, swapchain_textures[TGFXRENDERER->GetCurrentFrameIndex()], image_access_tgfx_SWAPCHAIN_DISPLAY, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
		TGFXRENDERER->ImageBarrier(final_tp, swapchain_textures[TGFXRENDERER->GetCurrentFrameIndex()], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
		TGFXRENDERER->SwapBuffers(firstwindow, first_wp);

		TGFXRENDERER->Run();
		TGFXCORE->take_inputs();
		profilersys->funcs->finish_profiling(&scope, 1);
	}
	printf("Application is finished!");
	
	return 1;
}