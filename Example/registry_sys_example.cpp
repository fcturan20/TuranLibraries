#include <iostream>
#include <string>
#include <windows.h>

#include "turanapi/registrysys_tapi.h"
#include "turanapi/virtualmemorysys_tapi.h"
#include "turanapi/unittestsys_tapi.h"
#include "turanapi/array_of_strings_tapi.h"
#include "turanapi/threadingsys_tapi.h"
#include "turanapi/filesys_tapi.h"
#include "turanapi/logger_tapi.h"
#include "turanapi/bitset_tapi.h"
#include "turanapi/profiler_tapi.h"

THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE threadingsys = NULL;
char stop_char;

unsigned char unittest_func(const char** output_string, void* data){
	*output_string = "evet unittest'teyiz!";
	return 0;
}

void unittest_resultvisualize(unsigned char result, const char* output_string){

}

void thread_print(){
	while(stop_char != '1'){
		printf("This thread index: %u\n", threadingsys->funcs->this_thread_index());
	}
	Sleep(2000);
	printf("Finished thread index: %u\n", threadingsys->funcs->this_thread_index());
}

int main(){
	auto registrydll = DLIB_LOAD_TAPI("tapi_registrysys.dll");
	if(registrydll == NULL){
		printf("Registry System DLL load failed!");
	}
	load_plugin_func loader = (load_plugin_func)DLIB_FUNC_LOAD_TAPI(registrydll, "load_plugin");
	registrysys_tapi* sys = ((registrysys_tapi_type*)loader(NULL, 0))->funcs;
	const char* first_pluginname = sys->list(NULL)[0];
	virtualmemorysys_tapi* virmemsys = ((VIRTUALMEMORY_TAPI_PLUGIN_TYPE)sys->get("tapi_virtualmemsys", MAKE_PLUGIN_VERSION_TAPI(0,0,0), 0))->funcs;

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
	unittest->run_tests(0xFFFFFFFF, NULL, &unittest_resultvisualize);

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
	profiledscope_handle_tapi firstprofiling_handle;
	unsigned long long firstprofiling_duration = 0;
	profilersys->funcs->start_profiling(&firstprofiling_handle, "First Profiling", &firstprofiling_duration, 2);


	scanf("%c", &stop_char);
	threadingsys->funcs->wait_all_otherjobs();
	Sleep(100);
	char new_char;
	scanf(" %c", &new_char);
	profilersys->funcs->finish_profiling(&firstprofiling_handle, 1);
	printf("Profiling Duration: %llu\n", firstprofiling_duration);
	printf("Application is finished!");

	return 1;
}