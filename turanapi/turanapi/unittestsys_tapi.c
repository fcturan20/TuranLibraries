#include "unittestsys_tapi.h"
#include "registrysys_tapi.h"
#include "array_of_strings_tapi.h"

unittestsys_tapi_type* utsys;

typedef struct unittestsys_tapi_d{
    unittest_interface_tapi* tests;
    array_of_strings_tapi* name_aray;
} unittestsys_tapi_d;


void* add_unittest(const char* name, unsigned int classification_flag, unittest_interface_tapi test_interface){
    const char* test_result;
    test_interface.test(&test_result, NULL);
    printf("Test Name: %s\n Result: %s", name, test_result);
}

void remove_unittest(const char* name, unsigned int classification_flag){

}

void run_tests(unsigned int classification_flag, const char* name, void (*testoutput_visualization)(unsigned char unittest_result, const char* output_string)){

}


FUNC_DLIB_EXPORT void* load_plugin(registrysys_tapi* reg_sys, unsigned char reload){
    utsys = (unittestsys_tapi_type*)malloc(sizeof(unittestsys_tapi_type));
    utsys->data = (unittestsys_tapi_d*)malloc(sizeof(unittestsys_tapi_d));
    utsys->funcs = (unittestsys_tapi*)malloc(sizeof(unittestsys_tapi));

    utsys->funcs->add_unittest = &add_unittest;
    utsys->funcs->remove_unittest = &remove_unittest;
    utsys->funcs->run_tests = &run_tests;

    reg_sys->add(UNITTEST_TAPI_PLUGIN_NAME, 0, utsys->funcs);

    return utsys->funcs;
}
FUNC_DLIB_EXPORT void unload_plugin(registrysys_tapi* reg_sys, unsigned char reload){
    free(utsys->data);
    free(utsys->funcs);
    free(utsys);
}
 

