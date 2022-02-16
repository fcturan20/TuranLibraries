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


#include <stdint.h>
#include <vector>


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
	aos->virtual_allocator->change_string(first_array, 0, "Naber?");
	printf("%s", aos->virtual_allocator->read_string(first_array, 0));
	aos->virtual_allocator->change_string(first_array, 0, "FUCK");
	printf("%s", aos->virtual_allocator->read_string(first_array, 0));
	unittest->run_tests(0xFFFFFFFF, NULL, NULL);

	auto threadingsysdll = DLIB_LOAD_TAPI("tapi_threadedjobsys.dll");
	load_plugin_func threadingloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(threadingsysdll, "load_plugin");
	threadingsys = (THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE)threadingloader(sys, 0);

	auto filesysdll = DLIB_LOAD_TAPI("tapi_filesys.dll");
	load_plugin_func filesysloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(filesysdll, "load_plugin");
	FILESYS_TAPI_PLUGIN_LOAD_TYPE filesys = (FILESYS_TAPI_PLUGIN_LOAD_TYPE)filesysloader(sys, 0);
	filesys->funcs->write_textfile("naber?", "first.txt", false);

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

	gpu_tgfx_listhandle gpulist;
	tgfxsys->api->getGPUlist(&gpulist);
	TGFXLISTCOUNT(tgfxsys->api, gpulist, gpulist_count);
	printf("GPU COUNT: %u\n", gpulist_count);
	const char* gpuname;
	const memory_description_tgfx* memtypelist; unsigned int memtypelistcount;
	tgfxsys->api->helpers->GetGPUInfo_General(gpulist[0], &gpuname, nullptr, nullptr, nullptr, &memtypelist, &memtypelistcount, nullptr, nullptr, nullptr);
	std::cout << "Memory Type Count: " << memtypelistcount << " and GPU Name: " << gpuname << "\n";
	unsigned int devicelocalmemtype_id = UINT32_MAX, fastvisiblememtype_id = UINT32_MAX;
	for (unsigned int i = 0; i < memtypelistcount; i++) {
		if (memtypelist[i].allocationtype == memoryallocationtype_DEVICELOCAL) {
			tgfxsys->api->helpers->SetMemoryTypeInfo(memtypelist[i].memorytype_id, 40 * 1024 * 1024, nullptr);
			devicelocalmemtype_id = memtypelist[i].memorytype_id;
		}
		if (memtypelist[i].allocationtype == memoryallocationtype_FASTHOSTVISIBLE) {
			tgfxsys->api->helpers->SetMemoryTypeInfo(memtypelist[i].memorytype_id, 20 * 1024 * 1024, nullptr);
			fastvisiblememtype_id = memtypelist[i].memorytype_id;
		}
	}
	initializationsecondstageinfo_tgfx_handle secondinfo = tgfxsys->api->helpers->Create_GFXInitializationSecondStageInfo(gpulist[0], 10, 100, 100, 100, 100, 100, 100, 100, 100, false, true, true, (extension_tgfx_listhandle)tgfxsys->api->INVALIDHANDLE);
	tgfxsys->api->initialize_secondstage(secondinfo);
	monitor_tgfx_listhandle monitorlist;
	tgfxsys->api->getmonitorlist(&monitorlist);
	TGFXLISTCOUNT(tgfxsys->api, monitorlist, monitorcount);
	printf("Monitor Count: %u", monitorcount);
	textureusageflag_tgfx_handle usageflag = tgfxsys->api->helpers->CreateTextureUsageFlag(false, true, true, true, false);
	texture_tgfx_handle swapchain_textures[2];
	window_tgfx_handle firstwindow;
	tgfxsys->api->create_window(1280, 720, monitorlist[0], windowmode_tgfx_WINDOWED, "RegistrySys Example", usageflag, nullptr, nullptr, swapchain_textures, &firstwindow);
	tgfxsys->api->renderer->Start_RenderGraphConstruction();


	//First Copy TP Creation
	transferpass_tgfx_handle first_copytp;
	tgfxsys->api->renderer->Create_TransferPass((passwaitdescription_tgfx_listhandle)&tgfxsys->api->INVALIDHANDLE, transferpasstype_tgfx_COPY, "FirstCopyTP", &first_copytp);

	//First Barrier TP Creation
	passwaitdescription_tgfx_handle wait_for_copytp[2] = { tgfxsys->api->helpers->CreatePassWait_TransferPass(&first_copytp, transferpasstype_tgfx_COPY, tgfxsys->api->helpers->CreateWaitSignal_Transfer(true, true), false), (passwaitdescription_tgfx_handle)tgfxsys->api->INVALIDHANDLE };
	transferpass_tgfx_handle firstbarriertp;
	tgfxsys->api->renderer->Create_TransferPass(wait_for_copytp, transferpasstype_tgfx_BARRIER, "FirstBarrierTP", &firstbarriertp);

	//Draw Pass Creation

	vec4_tgfx white; white.x = 255; white.y = 255; white.z = 255; white.w = 0;
	rtslotdescription_tgfx_handle rtslot_descs[2] = {
		tgfxsys->api->helpers->CreateRTSlotDescription_Color(swapchain_textures[0], swapchain_textures[1], operationtype_tgfx_READ_AND_WRITE, drawpassload_tgfx_CLEAR, true, 0, white), (rtslotdescription_tgfx_handle)tgfxsys->api->INVALIDHANDLE };
	rtslotset_tgfx_handle rtslotset_handle;
	tgfxsys->api->contentmanager->Create_RTSlotset(rtslot_descs, &rtslotset_handle);
	rtslotusage_tgfx_handle rtslotusages[2] = { tgfxsys->api->helpers->CreateRTSlotUsage_Color(rtslot_descs[0], operationtype_tgfx_READ_AND_WRITE, drawpassload_tgfx_CLEAR), (rtslotusage_tgfx_handle)tgfxsys->api->INVALIDHANDLE };
	inheritedrtslotset_tgfx_handle irtslotset;
	tgfxsys->api->contentmanager->Inherite_RTSlotSet(rtslotusages, rtslotset_handle, &irtslotset);
	subdrawpassdescription_tgfx_handle subdp_descs[2] = { tgfxsys->api->helpers->CreateSubDrawPassDescription(irtslotset, subdrawpassaccess_tgfx_ALLCOMMANDS, subdrawpassaccess_tgfx_ALLCOMMANDS), (subdrawpassdescription_tgfx_handle)tgfxsys->api->INVALIDHANDLE };
	passwaitdescription_tgfx_handle dp_waits[2] = { tgfxsys->api->helpers->CreatePassWait_TransferPass(&firstbarriertp, transferpasstype_tgfx_BARRIER,
		tgfxsys->api->helpers->CreateWaitSignal_Transfer(true, true), false), (passwaitdescription_tgfx_handle)tgfxsys->api->INVALIDHANDLE };
	subdrawpass_tgfx_listhandle sub_dps;
	drawpass_tgfx_handle dp;
	tgfxsys->api->renderer->Create_DrawPass(subdp_descs, rtslotset_handle, dp_waits, "First DP", &sub_dps, &dp);

	//Final Barrier TP Creation
	waitsignaldescription_tgfx_handle waitsignaldescs[2] = { tgfxsys->api->helpers->CreateWaitSignal_FragmentShader(true, true, true), (waitsignaldescription_tgfx_handle)tgfxsys->api->INVALIDHANDLE };
	passwaitdescription_tgfx_handle final_tp_waits[2] = { tgfxsys->api->helpers->CreatePassWait_DrawPass(&dp, 0, waitsignaldescs, false), (passwaitdescription_tgfx_handle)tgfxsys->api->INVALIDHANDLE };
	transferpass_tgfx_handle final_tp;
	tgfxsys->api->renderer->Create_TransferPass(final_tp_waits, transferpasstype_tgfx_BARRIER, "Final TP", &final_tp);

	windowpass_tgfx_handle first_wp;
	waitsignaldescription_tgfx_handle wait_for_finaltp_signal = tgfxsys->api->helpers->CreateWaitSignal_Transfer(true, true);
	passwaitdescription_tgfx_handle wait_for_dp[2] = { tgfxsys->api->helpers->CreatePassWait_TransferPass(&final_tp, transferpasstype_tgfx_BARRIER, wait_for_finaltp_signal, false), (passwaitdescription_tgfx_handle)tgfxsys->api->INVALIDHANDLE };
	tgfxsys->api->renderer->Create_WindowPass(wait_for_dp, "First WP", &first_wp);
	tgfxsys->api->renderer->Finish_RenderGraphConstruction(sub_dps[0]);


	datatype_tgfx vertexattribs_datatypes[3]{ datatype_tgfx_VAR_VEC2, datatype_tgfx_VAR_VEC2, datatype_tgfx_UNDEFINED };
	vertexattributelayout_tgfx_handle vertexattriblayout;
	tgfxsys->api->contentmanager->Create_VertexAttributeLayout(vertexattribs_datatypes, vertexlisttypes_tgfx_TRIANGLELIST, &vertexattriblayout);

	//Create vertex buffer, staging buffer to upload to and upload data

	float vertexbufferdata[]{ -1.0, -1.0, 0.0, 0.0,
		-1.0, 1.0, 0.0, 1.0, 
		1.0, -1.0, 1.0, 0.0,
		1.0, -1.0, 1.0, 0.0,
		-1.0, 1.0, 0.0, 1.0,
		1.0, 1.0, 1.0, 1.0};
	buffer_tgfx_handle firstvertexbuffer, firststagingbuffer;
	tgfxsys->api->contentmanager->Create_VertexBuffer(vertexattriblayout, 6, devicelocalmemtype_id, &firstvertexbuffer);
	tgfxsys->api->contentmanager->Create_StagingBuffer(6 * 20, fastvisiblememtype_id, &firststagingbuffer);	//Allocate a little bit much
	tgfxsys->api->contentmanager->Upload_toBuffer(firststagingbuffer, buffertype_tgfx_STAGING, vertexbufferdata, 6 * 4 * 4, 0);
	tgfxsys->api->renderer->CopyBuffer_toBuffer(first_copytp, firststagingbuffer, buffertype_tgfx_STAGING, firstvertexbuffer, buffertype_tgfx_VERTEX, 0, 0, 6 * 4 * 4);

	//Create texture, staging buffer upload data to and upload data

	texture_tgfx_handle first_texture; buffer_tgfx_handle texturestagingbuffer;
	textureusageflag_tgfx_handle usageflag_tgfx = tgfxsys->api->helpers->CreateTextureUsageFlag(false, true, false, true, false);
	tgfxsys->api->contentmanager->Create_Texture(texture_dimensions_tgfx_2D, 1280, 720, texture_channels_tgfx_RGBA32F, 1, usageflag, texture_order_tgfx_SWIZZLE, devicelocalmemtype_id, &first_texture);
	vec4_tgfx color; color.x = 1.0f; color.y = 0.5f; color.z = 0.2f; color.w = 1.0f;
	std::vector<vec4_tgfx> texture_data(1280 * 720, color);
	tgfxsys->api->contentmanager->Create_StagingBuffer(1280 * 720 * 20, fastvisiblememtype_id, &texturestagingbuffer);
	tgfxsys->api->contentmanager->Upload_toBuffer(texturestagingbuffer, buffertype_tgfx_STAGING, texture_data.data(), texture_data.size() * 16, 0);


	unsigned long vertsize = 0, fragsize = 0;
	void* vert_binary = filesys->funcs->read_binaryfile(CMAKE_SOURCE_FOLDER"/Example/FirstVert.spv", &vertsize); if (!vert_binary) { printf("Vertex Shader SPIR-V read has failed!"); }
	void* frag_binary = filesys->funcs->read_binaryfile(CMAKE_SOURCE_FOLDER"/Example/FirstFrag.spv", &fragsize); if (!frag_binary) { printf("Fragment Shader SPIR-V read has failed!"); }
	shadersource_tgfx_handle compiled_sources[3]{(shadersource_tgfx_handle)tgfxsys->api->INVALIDHANDLE, (shadersource_tgfx_handle)tgfxsys->api->INVALIDHANDLE, (shadersource_tgfx_handle)tgfxsys->api->INVALIDHANDLE };
	tgfxsys->api->contentmanager->Compile_ShaderSource(shaderlanguages_tgfx_SPIRV, shaderstage_tgfx_VERTEXSHADER, vert_binary, vertsize, &compiled_sources[0]);
	tgfxsys->api->contentmanager->Compile_ShaderSource(shaderlanguages_tgfx_SPIRV, shaderstage_tgfx_FRAGMENTSHADER, frag_binary, fragsize, &compiled_sources[1]);
	rasterpipelinetype_tgfx_handle first_material;
	tgfxsys->api->contentmanager->Link_MaterialType(compiled_sources, (shaderinputdescription_tgfx_handle*)&tgfxsys->api->INVALIDHANDLE, vertexattriblayout, sub_dps[0], cullmode_tgfx_OFF, polygonmode_tgfx_FILL, nullptr, nullptr, nullptr,
		(blendinginfo_tgfx_listhandle)&tgfxsys->api->INVALIDHANDLE, &first_material);
	rasterpipelineinstance_tgfx_handle first_matinst;
	tgfxsys->api->contentmanager->Create_MaterialInst(first_material, &first_matinst);
	samplingtype_tgfx_handle first_samplingtype;
	tgfxsys->api->contentmanager->Create_SamplingType(0, 1, texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP, texture_mipmapfilter_tgfx_LINEAR_FROM_1MIP, texture_wrapping_tgfx_REPEAT, texture_wrapping_tgfx_REPEAT, texture_wrapping_tgfx_REPEAT, &first_samplingtype);
	tgfxsys->api->contentmanager->SetGlobalShaderInput_Texture(true, 0, false, first_texture, first_samplingtype, image_access_tgfx_SHADER_SAMPLEONLY);

	//Swapchain images should be converted from no_access layout in the first frame
	tgfxsys->api->renderer->ImageBarrier(firstbarriertp, first_texture, image_access_tgfx_NO_ACCESS, image_access_tgfx_TRANSFER_DIST, 0, cubeface_tgfx_FRONT);
	tgfxsys->api->renderer->ImageBarrier(firstbarriertp, swapchain_textures[0], image_access_tgfx_NO_ACCESS, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
	tgfxsys->api->renderer->ImageBarrier(firstbarriertp, swapchain_textures[1], image_access_tgfx_NO_ACCESS, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
	tgfxsys->api->renderer->ImageBarrier(final_tp, swapchain_textures[0], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
	tgfxsys->api->renderer->ImageBarrier(final_tp, swapchain_textures[1], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);

	tgfxsys->api->renderer->SwapBuffers(firstwindow, first_wp);
	tgfxsys->api->renderer->Run();




	while (true) {
		profiledscope_handle_tapi scope;	unsigned long long duration;

		boxregion_tgfx region;
		region.WIDTH = 1280; region.HEIGHT = 720; region.XOffset = 0; region.YOffset = 0;
		tgfxsys->api->renderer->CopyBuffer_toImage(first_copytp, texturestagingbuffer, first_texture, 0, region, buffertype_tgfx_STAGING, 0, cubeface_tgfx_FRONT);
		tgfxsys->api->renderer->ImageBarrier(firstbarriertp, first_texture, image_access_tgfx_TRANSFER_DIST, image_access_tgfx_SHADER_SAMPLEONLY, 0, cubeface_tgfx_FRONT);
		tgfxsys->api->renderer->ImageBarrier(final_tp, first_texture, image_access_tgfx_SHADER_SAMPLEONLY, image_access_tgfx_TRANSFER_DIST, 0, cubeface_tgfx_FRONT);

		profilersys->funcs->start_profiling(&scope, "Run Loop", &duration, 1);
		tgfxsys->api->renderer->ImageBarrier(firstbarriertp, swapchain_textures[0], image_access_tgfx_SWAPCHAIN_DISPLAY, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
		tgfxsys->api->renderer->ImageBarrier(firstbarriertp, swapchain_textures[1], image_access_tgfx_SWAPCHAIN_DISPLAY, image_access_tgfx_RTCOLOR_READWRITE, 0, cubeface_tgfx_FRONT);
		tgfxsys->api->renderer->ImageBarrier(final_tp, swapchain_textures[0], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
		tgfxsys->api->renderer->ImageBarrier(final_tp, swapchain_textures[1], image_access_tgfx_RTCOLOR_READWRITE, image_access_tgfx_SWAPCHAIN_DISPLAY, 0, cubeface_tgfx_FRONT);
		tgfxsys->api->renderer->SwapBuffers(firstwindow, first_wp);
		tgfxsys->api->renderer->CopyBuffer_toBuffer(first_copytp, firststagingbuffer, buffertype_tgfx_STAGING, firstvertexbuffer, buffertype_tgfx_VERTEX, 0, 0, 6 * 4 * 4);
		tgfxsys->api->renderer->Run();
		tgfxsys->api->renderer->DrawDirect(firstvertexbuffer, nullptr, 6, 0, 0, 1, 0, first_matinst, sub_dps[0]);
		profilersys->funcs->finish_profiling(&scope, 1);
	}
	printf("Application is finished!");
	
	return 1;
}