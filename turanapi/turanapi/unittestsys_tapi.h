#pragma once
#define UNITTEST_TAPI_PLUGIN_NAME "tapi_unittest"
#define UNITTEST_TAPI_LOAD_TYPE unittestsys_tapi*

//RegistrySys Documentation: Storing Data for Systems
typedef struct virtualmemorysys_tapi_d virtualmemorysys_tapi_d;

//UnitTests are pretty much new to me so this interface is very abstract
//output_string: Test results may be displayed differently according to how unittestsys_tapi is compiled, so return at least a string.
//data: If you want to call the unittest yourself, you can use this to pass information in&out.
typedef struct unittest_interface_tapi{
    unsigned char (*test)(const char** output_string, void* data);
} unittest_interface_tapi;

typedef struct unittestsys_tapi{
    //name: Name of the unit test
    //classification_flag: Classify your unit tests to seperate types of your tests
    void* (*add_unittest)(const char* name, unsigned int classification_flag, unittest_interface_tapi test_interface);

    void (*remove_unittest)(const char* name, unsigned int classification_flag);
    
    //Run tests with the specified classification_flag
    //classification_flag: Unit tests with classified flags are run. If value is zero, name is used.
    //name: If classification_flag is 0, unit tests with matching name are run.
    //testoutput_visualization: Implementation may use it to display test results...
    //  Each unittest is run after the previous one finished.
    void (*run_tests)(unsigned int classification_flag, const char* name, void (*testoutput_visualization)(unsigned char unittest_result, const char* output_string));
} unittestsys_tapi;

//Unit Test System
//System that allows reserve, commit and free operations across platforms
typedef struct unittestsys_tapi_type{
    //RegistrySys Documentation: Storing Functions for Systems
    struct unittestsys_tapi* funcs;
    //RegistrySys Documentation: Storing Data for Systems
    struct unittestsys_tapi_d* data;
} unittestsys_tapi_type;

