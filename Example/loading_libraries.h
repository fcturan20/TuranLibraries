/*
1) You should search these if you want to learn how load a library
2) TGFX is the most complicated library and it's initialization takes 2 steps -unlike others that takes one step-.

*/
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

extern core_tgfx* TGFXCORE = nullptr;
extern const void* TGFXINVALID = nullptr;
extern helper_tgfx* TGFXHELPER = nullptr;
extern renderer_tgfx* TGFXRENDERER = nullptr;
extern gpudatamanager_tgfx* TGFXCONTENTMANAGER = nullptr;
extern dearimgui_tgfx* TGFXIMGUI = nullptr;


class TURANLIBRARY_LOADER {
public:
	static void printf_log_tgfx(result_tgfx result, const char* text) {
		printf("TGFX Result %u: %s\n", (unsigned int)result, text);
		if (result == result_tgfx_NOTCODED || result == result_tgfx_FAIL) {
			int i = 0;
			printf("TGFX RESULT ISN'T CODED HERE, PLEASE ENTER IF YOU WANT TO CONTINUE: ");
			scanf("%u", &i);
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
		array_of_strings_tapi* first_array = aos->virtual_allocator->create_array_of_string(virmem_loc, 1 << 30, 0);
		aos->virtual_allocator->change_string(first_array, 0, "wassup?");
		printf("%s", aos->virtual_allocator->read_string(first_array, 0));

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
		loggersys->log_status("First log!");

		auto bitsetsysdll = DLIB_LOAD_TAPI("tapi_bitset.dll");
		if (!bitsetsysdll) { printf("There is no tapi_bitset.dll file!"); }
		load_plugin_func bitsetloader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(bitsetsysdll, "load_plugin");
		if(!bitsetloader){printf("Bitset System DLL doesn't contain load_plugin function, which it should. You are probably using a newer version that uses different load scheme!");}
		BITSET_TAPI_PLUGIN_LOAD_TYPE bitsetsystype = (BITSET_TAPI_PLUGIN_LOAD_TYPE)bitsetloader(regsys, 0);
		if (!bitsetsystype) { printf("Bitset System loading has failed!"); }
		else { bitsetsys = bitsetsystype->funcs; }
		bitset_tapi* firstbitset = bitsetsys->create_bitset(100);
		bitsetsys->setbit_true(firstbitset, 5);
		printf("First true bit: %u and bitset size: %u\n", bitsetsys->getindex_firsttrue(firstbitset), bitsetsys->getbyte_length(firstbitset));

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
};