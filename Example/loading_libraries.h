/*
1) You should search these if you want to learn how load a library
2) TGFX is the most complicated library and it's initialization takes 2 steps -unlike others that takes one step-.

*/
#pragma once
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

extern registrysys_tapi* regsys = nullptr;
extern virtualmemorysys_tapi* virmemsys = nullptr;
extern unittestsys_tapi* unittestsys = nullptr;
extern threadingsys_tapi* threadingsys = nullptr;
extern filesys_tapi* filesys = nullptr;
extern logger_tapi* loggersys = nullptr;
extern bitsetsys_tapi* bitsetsys = nullptr;
extern profiler_tapi* profilersys = nullptr;


#include "tgfx_core.h"
#include "tgfx_helper.h"
#include "tgfx_renderer.h"
#include "tgfx_gpucontentmanager.h"
#include "tgfx_imgui.h"

extern core_tgfx* TGFXCORE = nullptr;
extern const void* TGFXINVALID = nullptr;
extern helper_tgfx* TGFXHELPER = nullptr;
extern renderer_tgfx* TGFXRENDERER = nullptr;
extern gpudatamanager_tgfx* TGFXCONTENTMANAGER = nullptr;
extern dearimgui_tgfx* TGFXIMGUI = nullptr;


//Global variables
extern unsigned int devicelocalmemtype_id = UINT32_MAX, fastvisiblememtype_id = UINT32_MAX;
extern texture_tgfx_handle swapchain_textures[2] = {NULL, NULL}, depthbuffer_handle = NULL, orange_texture = NULL;
extern window_tgfx_handle firstwindow = NULL;
extern buffer_tgfx_handle shaderuniformsbuffer = NULL;
extern vertexattributelayout_tgfx_handle vertexattriblayout = NULL;


extern transferpass_tgfx_handle framebegin_TP = NULL;
extern subtransferpass_tgfx_handle framebegin_subTPs[2] = {NULL, NULL};

extern subcomputepass_tgfx_handle subCP = NULL;
extern computepass_tgfx_handle ComputebeforeRasterization_CP = NULL;

extern subdrawpass_tgfx_handle framerendering_subDP = NULL;
extern drawpass_tgfx_handle framerendering_DP = NULL;

extern windowpass_tgfx_handle swapchain_windowpass = NULL;


class TURANLIBRARY_LOADER {
public:
	static void printf_log_tgfx(result_tgfx result, const char* text) {
		printf("TGFX Result %u: %s\n", (unsigned int)result, text);
		if (result != result_tgfx_SUCCESS && result != result_tgfx_WARNING) {
			int i = 0;
			printf("TGFX RESULT ISN'T CODED HERE\n");
			//scanf("%u", &i);
		}
	}

	static void Load_AllLibraries() {
		auto registrydll = DLIB_LOAD_TAPI("tapi_registrysys.dll");
		if (registrydll == NULL) { printf("There is no tapi_registrysys.dll file!");}
		load_plugin_func loader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(registrydll, "load_plugin");
		if (!loader) { printf("Registry System DLL doesn't contain load_plugin function, which it should. You are probably using a newer version that uses different load scheme!"); }
		regsys = ((registrysys_tapi_type*)loader(NULL, 0))->funcs;
		const char* first_pluginname = regsys->list(NULL)[0];
		VIRTUALMEMORY_TAPI_PLUGIN_TYPE virmem = (VIRTUALMEMORY_TAPI_PLUGIN_TYPE)regsys->get("tapi_virtualmemsys", MAKE_PLUGIN_VERSION_TAPI(0, 0, 0), 0);
		if (!virmem) { printf("Registry System should have a virtual memory system, but it doesn't. You are probably using a newer version that uses different memory allocation strategy!"); }
		else { virmemsys = virmem->funcs; }
		//Create an array of strings in a 1GB of reserved virtual address space
		void* virmem_loc = virmemsys->virtual_reserve(1 << 30);

		//Generally, I recommend you to load unit test system after registry system
		//Because other systems may implement unit tests but if unittestsys isn't loaded, they can't implement them.
		auto unittestdll = DLIB_LOAD_TAPI("tapi_unittest.dll");
		if (!unittestdll) { printf("There is no tapi_unittest.dll file!"); }
		load_plugin_func unittestloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(unittestdll, "load_plugin");
		if (!unittestloader) { printf("UnitTest System DLL doesn't contain load_plugin function, which it should. You are probably using a newer version that uses different load scheme!"); }
		UNITTEST_TAPI_LOAD_TYPE unittest = (UNITTEST_TAPI_LOAD_TYPE)unittestloader(regsys, 0);
		if (!unittest) { printf("UnitTest System loading has failed!"); }
		else { unittestsys = unittest->funcs; }

		auto aossysdll = DLIB_LOAD_TAPI("tapi_array_of_strings_sys.dll");
		if (!aossysdll) { printf("There is no tapi_array_of_strings_sys.dll file!"); }
		load_plugin_func aosloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(aossysdll, "load_plugin");
		if(!aosloader){ printf("Array of Strings System DLL doesn't contain load_plugin function, which it should. You are probably using a newer version that uses different load scheme!"); }
		ARRAY_OF_STRINGS_TAPI_LOAD_TYPE aos = (ARRAY_OF_STRINGS_TAPI_LOAD_TYPE)aosloader(regsys, 0);
		if (!aos) { printf("Array of Strings System loading has failed!"); }

		auto threadingsysdll = DLIB_LOAD_TAPI("tapi_threadedjobsys.dll");
		load_plugin_func threadingloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(threadingsysdll, "load_plugin");
		if (!unittestloader) { printf("ThreadingJob System DLL doesn't contain load_plugin function, which it should. You are probably using a newer version that uses different load scheme!"); }
		THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE threader = (THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE)threadingloader(regsys, 0);
		if (!threader) { printf("ThreadingJob System loading has failed!"); }
		else { threadingsys = threader->funcs; }

		auto filesysdll = DLIB_LOAD_TAPI("tapi_filesys.dll");
		load_plugin_func filesysloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(filesysdll, "load_plugin");
		if (!unittestloader) { printf("File System DLL doesn't contain load_plugin function, which it should. You are probably using a newer version that uses different load scheme!"); }
		FILESYS_TAPI_PLUGIN_LOAD_TYPE filesystype = (FILESYS_TAPI_PLUGIN_LOAD_TYPE)filesysloader(regsys, 0);
		if (!filesystype) { printf("File System loading has failed!"); }
		else { filesys = filesystype->funcs; }

		auto loggerdll = DLIB_LOAD_TAPI("tapi_logger.dll");
		if (!loggerdll) { printf("There is no tapi_logger.dll file!"); }
		load_plugin_func loggerloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(loggerdll, "load_plugin");
		if(!loggerloader){ printf("Logger System DLL doesn't contain load_plugin function, which it should. You are probably using a newer version that uses different load scheme!"); }
		LOGGER_TAPI_PLUGIN_LOAD_TYPE loggersystype = (LOGGER_TAPI_PLUGIN_LOAD_TYPE)loggerloader(regsys, 0);
		if (!loggersystype) { printf("Logger System loading has failed!"); }
		else { loggersys = loggersystype->funcs; }

		auto bitsetsysdll = DLIB_LOAD_TAPI("tapi_bitset.dll");
		if (!bitsetsysdll) { printf("There is no tapi_bitset.dll file!"); }
		load_plugin_func bitsetloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(bitsetsysdll, "load_plugin");
		if(!bitsetloader){printf("Bitset System DLL doesn't contain load_plugin function, which it should. You are probably using a newer version that uses different load scheme!");}
		BITSET_TAPI_PLUGIN_LOAD_TYPE bitsetsystype = (BITSET_TAPI_PLUGIN_LOAD_TYPE)bitsetloader(regsys, 0);
		if (!bitsetsystype) { printf("Bitset System loading has failed!"); }
		else { bitsetsys = bitsetsystype->funcs; }

		auto profilersysdll = DLIB_LOAD_TAPI("tapi_profiler.dll");
		if (!profilersysdll) { printf("There is no tapi_profiler.dll"); }
		load_plugin_func profilersysloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(profilersysdll, "load_plugin");
		if (!profilersysloader) { printf("Profiler System DLL doesn't contain load_plugin function, which it should. You are probably using a newer version that uses different load scheme!"); }
		PROFILER_TAPI_PLUGIN_LOAD_TYPE profilersystype = (PROFILER_TAPI_PLUGIN_LOAD_TYPE)profilersysloader(regsys, 0);
		if (!profilersystype) { printf("Profiler System loading has failed!"); }
		else { profilersys = profilersystype->funcs; }


		auto tgfxdll = DLIB_LOAD_TAPI("tgfx_core.dll");
		if (!tgfxdll) { printf("There is no tgfx_core.dll file!"); }
		load_plugin_func tgfxloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(tgfxdll, "load_plugin");
		if (!tgfxloader) { printf("TGFX System DLL doesn't contain load_plugin function, which it should. You are probably using a newer version that uses different load scheme!"); }
		TGFX_PLUGIN_LOAD_TYPE tgfxsys = (TGFX_PLUGIN_LOAD_TYPE)tgfxloader(regsys, 0);
		if(!tgfxsys) { printf("TGFX System loading has failed!"); }
		else { TGFXCORE = tgfxsys->api; }
		//You have to load a backend to use the TGFX system.
		//There is no data in the base TGFX system if you don't load any backend.
		result_tgfx result = tgfxsys->api->load_backend(backends_tgfx_VULKAN, printf_log_tgfx);
		printf("%c", result);

		TGFXCORE = tgfxsys->api;
		TGFXINVALID = tgfxsys->api->INVALIDHANDLE;
		TGFXHELPER = tgfxsys->api->helpers;
		TGFXRENDERER = tgfxsys->api->renderer;
		TGFXCONTENTMANAGER = tgfxsys->api->contentmanager;
		TGFXIMGUI = tgfxsys->api->imgui;
	}

	static void InitializeTGFX_CreateWindow(unsigned int devicelocal_memallocsize, unsigned int fastvisible_memallocsize, uvec2_tgfx window_resolution) {
		gpu_tgfx_listhandle gpulist;
		TGFXCORE->getGPUlist(&gpulist);
		TGFXLISTCOUNT(TGFXCORE, gpulist, gpulist_count);
		printf("GPU COUNT: %u\n", gpulist_count);
		const char* gpuname;
		const memory_description_tgfx* memtypelist; unsigned int memtypelistcount;
		TGFXHELPER->GetGPUInfo_General(gpulist[0], &gpuname, nullptr, nullptr, nullptr, &memtypelist, &memtypelistcount, nullptr, nullptr, nullptr);
		std::cout << "Memory Type Count: " << memtypelistcount << " and GPU Name: " << gpuname << "\n";
		for (unsigned int i = 0; i < memtypelistcount; i++) {
			if (memtypelist[i].allocationtype == memoryallocationtype_DEVICELOCAL) {
				TGFXHELPER->SetMemoryTypeInfo(memtypelist[i].memorytype_id, devicelocal_memallocsize , nullptr);
				devicelocalmemtype_id = memtypelist[i].memorytype_id;
			}
			if (memtypelist[i].allocationtype == memoryallocationtype_FASTHOSTVISIBLE) {
				TGFXHELPER->SetMemoryTypeInfo(memtypelist[i].memorytype_id, fastvisible_memallocsize, nullptr);
				fastvisiblememtype_id = memtypelist[i].memorytype_id;
			}
		}
		initializationsecondstageinfo_tgfx_handle secondinfo = TGFXHELPER->Create_GFXInitializationSecondStageInfo(gpulist[0], 100, 1000, 1000, 1000, 1000, true, (extension_tgfx_listhandle)(&TGFXCORE->INVALIDHANDLE));
		TGFXCORE->initialize_secondstage(secondinfo);
		monitor_tgfx_listhandle monitorlist;
		TGFXCORE->getmonitorlist(&monitorlist);
		TGFXLISTCOUNT(TGFXCORE, monitorlist, monitorcount);
		TURANLIBRARY_LOADER::printf_log_tgfx(result_tgfx_SUCCESS, (std::string("Monitor Count: ") + std::to_string(monitorcount)).c_str());
		textureusageflag_tgfx_handle swapchaintexture_usageflag = TGFXHELPER->CreateTextureUsageFlag(false, true, true, true, false);
		TGFXCORE->create_window(window_resolution.x, window_resolution.y, monitorlist[0], windowmode_tgfx_WINDOWED, "RegistrySys Example", nullptr, nullptr, &firstwindow);
		TGFXCORE->create_swapchain(firstwindow, windowpresentation_tgfx_FIFO, windowcomposition_tgfx_OPAQUE, colorspace_tgfx_sRGB_NONLINEAR, texture_channels_tgfx_BGRA8UNORM, swapchaintexture_usageflag, swapchain_textures);
	}

	static void ReconstructRenderGraph_Simple(){
		TGFXRENDERER->Start_RenderGraphConstruction();

		//3 of these passes are merged: FRAMEBEGIN - FIRSTCP - FRAMERENDERING

		//FrameBegin TP Creation
		passwaitdescription_tgfx_handle wait_for_copytp[2] = { TGFXHELPER->CreatePassWait_SubTransferPass(&framebegin_TP, 0, TGFXHELPER->CreateWaitSignal_Transfer(true, true), false), (passwaitdescription_tgfx_handle)TGFXCORE->INVALIDHANDLE };
		passwaitdescription_tgfx_handle wait_for_subcp[2] = { TGFXHELPER->CreatePassWait_SubComputePass(&ComputebeforeRasterization_CP, 0, TGFXHELPER->CreateWaitSignal_ComputeShader(true, true, true), false), (passwaitdescription_tgfx_handle)TGFXCORE->INVALIDHANDLE };
		waitsignaldescription_tgfx_handle framerendering_waitsignals[2] = { TGFXHELPER->CreateWaitSignal_FragmentShader(true, true, true), (waitsignaldescription_tgfx_handle)TGFXCORE->INVALIDHANDLE };
		passwaitdescription_tgfx_handle wait_for_raster[2] = { TGFXHELPER->CreatePassWait_SubDrawPass(&framerendering_DP, 0, framerendering_waitsignals, false), (passwaitdescription_tgfx_handle)TGFXCORE->INVALIDHANDLE };
		subtransferpassdescription_tgfx_handle subtpdescs[5] = { 
			TGFXHELPER->CreateSubTransferPassDescription((passwaitdescription_tgfx_listhandle)(&TGFXCORE->INVALIDHANDLE), transferpasstype_tgfx_COPY),
			TGFXHELPER->CreateSubTransferPassDescription(wait_for_copytp, transferpasstype_tgfx_BARRIER),
			TGFXHELPER->CreateSubTransferPassDescription(wait_for_subcp, transferpasstype_tgfx_BARRIER),
			TGFXHELPER->CreateSubTransferPassDescription(wait_for_raster, transferpasstype_tgfx_BARRIER),
			(subtransferpassdescription_tgfx_handle)TGFXCORE->INVALIDHANDLE };
		TGFXRENDERER->Create_TransferPass(subtpdescs, framebegin_subTPs, &framebegin_TP);


		//First Compute Pass Creation
		passwaitdescription_tgfx_handle subcp_waits[2] = { TGFXHELPER->CreatePassWait_SubTransferPass(&framebegin_TP, 1, TGFXHELPER->CreateWaitSignal_Transfer(true, true), false), (passwaitdescription_tgfx_handle)TGFXINVALID };
		subcomputepassdescription_tgfx_handle subcpdescs[2] = { TGFXHELPER->CreateSubComputePassDescription(subcp_waits), (subcomputepassdescription_tgfx_handle)TGFXCORE->INVALIDHANDLE };
		TGFXRENDERER->Create_ComputePass(subcpdescs, &subCP, &ComputebeforeRasterization_CP);


		//Draw Pass Creation
		vec4_tgfx white; white.x = 255; white.y = 255; white.z = 255; white.w = 0;
		rtslotdescription_tgfx_handle rtslot_descs[3] = {
			TGFXHELPER->CreateRTSlotDescription_Color(swapchain_textures[0], swapchain_textures[1], operationtype_tgfx_READ_AND_WRITE, drawpassload_tgfx_CLEAR, true, 0, white),
			TGFXHELPER->CreateRTSlotDescription_DepthStencil(depthbuffer_handle, depthbuffer_handle, operationtype_tgfx_READ_AND_WRITE, drawpassload_tgfx_CLEAR, operationtype_tgfx_UNUSED, drawpassload_tgfx_CLEAR, 1.0, 255), (rtslotdescription_tgfx_handle)TGFXINVALID };
		rtslotset_tgfx_handle rtslotset_handle;
		TGFXCONTENTMANAGER->Create_RTSlotset(rtslot_descs, &rtslotset_handle);
		rtslotusage_tgfx_handle rtslotusages[3] = { TGFXHELPER->CreateRTSlotUsage_Color(rtslot_descs[0], operationtype_tgfx_READ_AND_WRITE, drawpassload_tgfx_CLEAR),
			TGFXHELPER->CreateRTSlotUsage_Depth(rtslot_descs[1], operationtype_tgfx_READ_AND_WRITE, drawpassload_tgfx_CLEAR, operationtype_tgfx_UNUSED, drawpassload_tgfx_CLEAR), (rtslotusage_tgfx_handle)TGFXINVALID };
		inheritedrtslotset_tgfx_handle irtslotset;
		TGFXCONTENTMANAGER->Inherite_RTSlotSet(rtslotusages, rtslotset_handle, &irtslotset);
		passwaitdescription_tgfx_handle subdp_waits[2] = { TGFXHELPER->CreatePassWait_SubTransferPass(&framebegin_TP, 2, TGFXHELPER->CreateWaitSignal_Transfer(true, true), false), (passwaitdescription_tgfx_handle)TGFXCORE->INVALIDHANDLE };
		subdrawpassdescription_tgfx_handle subdp_descs[2] = { TGFXHELPER->CreateSubDrawPassDescription(subdp_waits, irtslotset, subdrawpassaccess_tgfx_ALLCOMMANDS, subdrawpassaccess_tgfx_ALLCOMMANDS), (subdrawpassdescription_tgfx_handle)TGFXINVALID };
		TGFXRENDERER->Create_DrawPass(subdp_descs, rtslotset_handle, &framerendering_subDP, &framerendering_DP);


		//Window Pass Creation
		passwaitdescription_tgfx_handle wait_for_dp[2] = { TGFXHELPER->CreatePassWait_SubTransferPass(&framebegin_TP, 3, TGFXHELPER->CreateWaitSignal_Transfer(true, true), false), (passwaitdescription_tgfx_handle)TGFXINVALID };
		TGFXRENDERER->Create_WindowPass(wait_for_dp, &swapchain_windowpass);
		TGFXRENDERER->Finish_RenderGraphConstruction(framerendering_subDP);
	}
};