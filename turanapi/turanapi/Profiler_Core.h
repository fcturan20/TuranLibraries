#pragma once
#include "API_includes.h"


/*
	Create a Profiling system that starts profiling in Begin_Profiling, profiles functions with Profile_Function, profiles scopes with Profile_Scope(name) and ends all profiling systems with End_Profiling!
*/



#ifdef __cplusplus
extern "C"{
#endif
TAPIHANDLE(ProfiledScope)

//Timing Indexes: 0->Nanoseconds, 1->Microseconds, 2->Milliseconds, 3->Seconds
void tapi_Profile_Start(tapi_ProfiledScope* handle, const char* NAME, unsigned long long* duration, unsigned char TimingTypeIndex);
void tapi_Profile_End(unsigned char ShouldPrint);



#define TURAN_PROFILE_SCOPE_NAS(name, duration_ptr) {tapi_ProfiledScope PROFILE##__LINE__ ; tapi_Profile_Start(&PROFILE##__LINE__, name, duration_ptr, 0); {
#define TURAN_PROFILE_SCOPE_MCS(name, duration_ptr) {tapi_ProfiledScope PROFILE##__LINE__ ; tapi_Profile_Start(&PROFILE##__LINE__, name, duration_ptr, 1); {
#define TURAN_PROFILE_SCOPE_MLS(name, duration_ptr) {tapi_ProfiledScope PROFILE##__LINE__ ; tapi_Profile_Start(&PROFILE##__LINE__, name, duration_ptr, 2); {
#define TURAN_PROFILE_SCOPE_SEC(name, duration_ptr) {tapi_ProfiledScope PROFILE##__LINE__ ; tapi_Profile_Start(&PROFILE##__LINE__, name, duration_ptr, 3); {
#define STOP_PROFILE_PRINTLESS_TAPI() } tapi_Profile_End(0);}
#define STOP_PROFILE_PRINTFUL_TAPI() } tapi_Profile_End(1);}



#ifdef __cplusplus
}
#endif

