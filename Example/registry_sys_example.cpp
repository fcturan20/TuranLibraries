#include <iostream>
#include <string>

#include "turanapi/registrysys_tapi.h"
#include "turanapi/virtualmemorysys_tapi.h"
#include "turanapi/unittestsys_tapi.h"
#include "turanapi/array_of_strings_tapi.h"
#include "turanapi/threadingsys_tapi.h"

THREADEDJOBSYS_TAPI_LOAD_TYPE threadingsys = NULL;
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
	threadingsys = (THREADEDJOBSYS_TAPI_LOAD_TYPE)threadingloader(sys, 0);
	printf("This thread index: %u\n", threadingsys->funcs->this_thread_index());
	for(unsigned int i = 0; i < threadingsys->funcs->thread_count(); i++){
		threadingsys->funcs->execute_withoutwait(&thread_print);
	}
	
	printf("Application is finished!");
	
	scanf("%c", &stop_char);
	threadingsys->funcs->wait_all_otherjobs();
	scanf("%c", &stop_char);

	return 1;
}