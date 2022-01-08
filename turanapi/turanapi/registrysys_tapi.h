#include "predefinitions.h"
#define REGISTRYSYS_TAPI_PLUGIN_TYPE registrysys_tapi_type*


typedef struct registrysys_tapi_d registrysys_tapi_d;

typedef struct registrysys_tapi {
    //name = "x": Plugin's name
    //version: Use MAKE_PLUGIN_VERSION_TAPI() and see its documentation for further information
    void (*add)(const char* name, unsigned int version, void* plugin_ptr);
    void (*remove)(void* plugin_ptr);
    //name = "x": First search for a plugin that's named "x", if not found search according to the search_flag
    //version: Use MAKE_PLUGIN_VERSION_TAPI() and see its documentation for further information
    //search_flag: Isn't used, added for future use.
    void* (*get)(const char* name, unsigned int version, unsigned char search_flag);
    //name = null -> List all; name = "x" -> each plugin that contains letter "x"
    //Return: An invalid-terminated list of null-terminated strings
    char** (*list)(const char* name);
} registrysys_tapi;

//Registry System
typedef struct registrysys_tapi_type{
    registrysys_tapi* funcs;
    registrysys_tapi_d* data;
} registrysys_tapi_type;


typedef void* (*load_plugin_func)(registrysys_tapi* reg_sys, unsigned char reload);
typedef void (*unload_plugin_func)(registrysys_tapi* reg_sys, unsigned char reload);
