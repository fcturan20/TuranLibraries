#include "profiler_tapi.h"
#include "threadingsys_tapi.h"
#include "ecs_tapi.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>



typedef struct profiler_tapi_d{
    profiler_tapi_type* type;
    profiledscope_handle_tapi* last_handles;
    unsigned int threadcount;
    threadingsys_tapi* threadsys;
} profiler_tapi_d;

struct Profiled_Scope {
public:
	bool Is_Recording : 1;
	unsigned char TimingType : 2;
	unsigned long long START_POINT;
	unsigned long long* DURATION;
	std::string NAME;
};

profiler_tapi_d* profiler_data = nullptr;

void start_profiling(profiledscope_handle_tapi* handle, const char* NAME, unsigned long long* duration, unsigned char TimingTypeIndex) {
    unsigned int threadindex = (profiler_data->threadcount == 1) ? (0) : (profiler_data->threadsys->this_thread_index());
    Profiled_Scope* profil = new Profiled_Scope;
    while(TimingTypeIndex > 3){printf("Profile started with index bigger than 3! Should be between 0-3!\n"); scanf(" %hhu", &TimingTypeIndex);}
    switch (TimingTypeIndex) {
    case 0:
        profil->START_POINT = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
        break;
    case 1:
        profil->START_POINT = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
        break;
    case 2:
        profil->START_POINT = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
        break;
    case 3:
        profil->START_POINT = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
        break;
    }
    profil->Is_Recording = true;
    profil->DURATION = duration;
    profil->TimingType = TimingTypeIndex;
    profil->NAME = NAME;
    *handle = (profiledscope_handle_tapi)profil;
    profiler_data->last_handles[threadindex]  = *handle;
}

void finish_profiling(profiledscope_handle_tapi* handle, unsigned char ShouldPrint) {
    Profiled_Scope* profil = (Profiled_Scope*)*handle;
    switch (profil->TimingType) {
    case 0:
        *profil->DURATION = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - profil->START_POINT;
        if (ShouldPrint) {printf((profil->NAME + " took: " + std::to_string(*profil->DURATION) + " nanoseconds!\n").c_str());}
        return;
    case 1:
        *profil->DURATION = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - profil->START_POINT;
        if (ShouldPrint) {printf((profil->NAME + " took: " + std::to_string(*profil->DURATION) + " microseconds!\n").c_str());}
        return;
    case 2:
        *profil->DURATION = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - profil->START_POINT;
        if (ShouldPrint) {printf((profil->NAME + " took: " + std::to_string(*profil->DURATION) + " milliseconds!\n").c_str());}
        return;
    case 3:
        *profil->DURATION = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
        if (ShouldPrint) {printf((profil->NAME + " took: " + std::to_string(*profil->DURATION) + " seconds!\n").c_str());}
        return;
    }
    delete profil;
}
void threadlocal_finish_last_profiling(unsigned char ShouldPrint) {
    unsigned int threadindex = (profiler_data->threadcount == 1) ? (0) : (profiler_data->threadsys->this_thread_index());
    Profiled_Scope* profil = (Profiled_Scope*)profiler_data->last_handles[threadindex];
    switch (profil->TimingType) {
    case 0:
        *profil->DURATION = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - profil->START_POINT;
        if (ShouldPrint) {printf((profil->NAME + " took: " + std::to_string(*profil->DURATION) + " nanoseconds!\n").c_str());}
        return;
    case 1:
        *profil->DURATION = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - profil->START_POINT;
        if (ShouldPrint) {printf((profil->NAME + " took: " + std::to_string(*profil->DURATION) + " microseconds!\n").c_str());}
        return;
    case 2:
        *profil->DURATION = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - profil->START_POINT;
        if (ShouldPrint) {printf((profil->NAME + " took: " + std::to_string(*profil->DURATION) + " milliseconds!\n").c_str());}
        return;
    case 3:
        *profil->DURATION = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
        if (ShouldPrint) {printf((profil->NAME + " took: " + std::to_string(*profil->DURATION) + " seconds!\n").c_str());}
        return;
    }
    delete profil;
}

ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
    unsigned int threadcount = 1;
    THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE threadsystype = (THREADINGSYS_TAPI_PLUGIN_LOAD_TYPE)ecssys->getSystem(THREADINGSYS_TAPI_PLUGIN_NAME);
    if(threadsystype){ threadcount = threadsystype->funcs->thread_count(); }

    profiler_tapi_type* type = (profiler_tapi_type*)malloc(sizeof(profiler_tapi_type));
    type->data = (profiler_tapi_d*)malloc(sizeof(profiler_tapi_d));
    type->funcs = (profiler_tapi*)malloc(sizeof(profiler_tapi));
    profiler_data = type->data;

    ecssys->addSystem(PROFILER_TAPI_PLUGIN_NAME, PROFILER_TAPI_PLUGIN_VERSION, type);

    type->funcs->start_profiling = &start_profiling;
    type->funcs->finish_profiling = &finish_profiling;
    type->funcs->threadlocal_finish_last_profiling = &threadlocal_finish_last_profiling;

    type->data->threadsys = threadsystype->funcs;
    type->data->threadcount = threadcount;
    type->data->last_handles = (profiledscope_handle_tapi*)malloc(sizeof(profiledscope_handle_tapi) * threadcount);
    memset(type->data->last_handles, 0, sizeof(profiledscope_handle_tapi) * threadcount);
}

ECSPLUGIN_EXIT(ecssys, reloadFlag) {

}